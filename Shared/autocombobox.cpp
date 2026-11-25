// autocombobox.cpp : implementation file
//

#include "stdafx.h"
#include "autocombobox.h"
#include "holdredraw.h"
#include "dialoghelper.h"
#include "autoflag.h"
#include "misc.h"
#include "enstring.h"
#include "graphicsmisc.h"
#include <vector>
#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////

const LPCTSTR STR_CLOSEBTN	= _T("r");
const int SIZE_CLOSEBTN		= 8;
const COLORREF RED			= RGB(200, 90, 90);
const COLORREF WHITE		= RGB(240, 240, 240);

/////////////////////////////////////////////////////////////////////////////

void AFXAPI DDX_AutoCBString(CDataExchange* pDX, int nIDC, CString& value)
{
	HWND hWndCtrl = pDX->PrepareCtrl(nIDC);

	if (pDX->m_bSaveAndValidate)
	{
		::DDX_CBString(pDX, nIDC, value);
	}
	else
	{
		// try exact first
		int nIndex = (int)::SendMessage(hWndCtrl, CB_FINDSTRINGEXACT, (WPARAM)-1,
										(LPARAM)(LPCTSTR)value);

		// then partial
		if (nIndex == CB_ERR)
		{
			nIndex = ::SendMessage(hWndCtrl, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)(LPCTSTR)value);

			// if there's a match, check its not partial
			if (nIndex != CB_ERR)
			{
				CString sItem;
				int nLen = ::SendMessage(hWndCtrl, CB_GETLBTEXTLEN, nIndex, 0);
				::SendMessage(hWndCtrl, CB_GETLBTEXT, nIndex, (LPARAM)(LPCTSTR)sItem.GetBuffer(nLen + 1));
				sItem.ReleaseBuffer();

				if (value.CompareNoCase(sItem) != 0)
					nIndex = CB_ERR;
			}
		}

		if (nIndex == CB_ERR)
		{
			// add it and then select it if not empty
			if (!value.IsEmpty())
				nIndex = ::SendMessage(hWndCtrl, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)value);

		}

		::SendMessage(hWndCtrl, CB_SETCURSEL, nIndex, 0L);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CAutoComboBox

CAutoComboBox::CAutoComboBox(DWORD dwFlags) 
	: 
	m_dwFlags(dwFlags), 
	m_bClosingUp(FALSE), 
	m_bEditChange(FALSE), 
	m_nDeleteItem(LB_ERR),
	m_nMaxTextWidth(-1),
	m_nKnownItemsCount(-1)
{
}

CAutoComboBox::~CAutoComboBox()
{
}


BEGIN_MESSAGE_MAP(CAutoComboBox, COwnerdrawComboBoxBase)
	//{{AFX_MSG_MAP(CAutoComboBox)
	ON_WM_SIZE()
	ON_WM_CTLCOLOR()
	//}}AFX_MSG_MAP
	ON_CONTROL_REFLECT_EX(CBN_SELENDCANCEL, OnSelEndCancel)
	ON_CONTROL_REFLECT_EX(CBN_SELENDOK, OnSelEndOK)
	ON_CONTROL_REFLECT_EX(CBN_SELCHANGE, OnSelChange)
	ON_CONTROL_REFLECT_EX(CBN_CLOSEUP, OnCloseUp)
	ON_CONTROL_REFLECT_EX(CBN_DROPDOWN, OnDropDown)
	ON_CONTROL_REFLECT_EX(CBN_EDITCHANGE, OnEditChange)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAutoComboBox message handlers

namespace
{
	struct CAutoComboBoxItemLess
	{
		bool operator()(const CString& s1, const CString& s2) const
		{
			return (s1.CompareNoCase(s2) < 0);
		}
	};
}

int CAutoComboBox::SetStrings(const CStringArray& aItems) 
{ 
	CComboBox::SetRedraw(FALSE);
	ResetContent(); 

	int nAdded = 0;
	const int nNumItems = aItems.GetSize();
	BOOL bSort = ((nNumItems > 64) && (GetStyle() & CBS_SORT));
	CStringArray aFeed;

	// A sorted combo positions every insert with an O(n) ordered insert and
	// shifts the index of all following items, which also invalidates our index
	// map. Feeding it in ascending order removes the shift, and dropping CBS_SORT
	// for the duration of the batch turns the ordered insert into a plain append.
	// The combo holds the same sorted items either way.
	if (bSort)
	{
		std::vector<CString> vItems;
		vItems.reserve(nNumItems);

		for (int nItem = 0; nItem < nNumItems; nItem++)
			vItems.push_back(Misc::GetItem(aItems, nItem));

		std::sort(vItems.begin(), vItems.end(), CAutoComboBoxItemLess());

		aFeed.SetSize(nNumItems);

		// ascending feed: measured best of the two orders (descending was slower)
		for (int nItem = 0; nItem < nNumItems; nItem++)
			aFeed[nItem] = vItems[nItem];
	}
	else
	{
		aFeed.Copy(aItems);
	}

	nAdded = AddUniqueItems(aFeed);

	// the cached index map cannot survive the re-ordering
	m_nKnownItemsCount = -1;

	CComboBox::SetRedraw(TRUE);

	return nAdded; 
}

BOOL CAutoComboBox::OnCloseUp()
{
	CAutoFlag af(m_bClosingUp, TRUE);

	// notify parent of (possible) selection change
	if (m_bEditChange)
		ParentCBNotify(CBN_SELCHANGE);
	else
		ParentCBNotify(CBN_SELENDCANCEL);
	
	return FALSE; // pass to parent
}

BOOL CAutoComboBox::OnDropDown()
{
	return FALSE; // pass to parent
}

BOOL CAutoComboBox::OnEditChange()
{
	m_bEditChange = TRUE;

	return FALSE; // pass to parent
}

BOOL CAutoComboBox::OnSelEndOK()
{
	OnSelChange();
	HandleReturnKey();

	return FALSE; // continue routing
}

BOOL CAutoComboBox::OnSelEndCancel()
{
	// eat this unless we sent it explicitly
	return (!m_bClosingUp);
}

BOOL CAutoComboBox::OnSelChange()
{
	// make sure the edit control is up to date
	if (IsHooked())
	{
		int nSel = GetCurSel();

		if (nSel < GetCount() )
			SetWindowText(GetSelectedItem());
	}

	// eat notification if dropped down
	// ie we don't send selection notifications until the dropdown closes
	return GetDroppedState() && !IsType(CBS_SIMPLE);
}

CString CAutoComboBox::GetSelectedItem() const
{
	CString sSel;
	int nSel = GetCurSel();

	if (nSel != CB_ERR)
		GetLBText(nSel, sSel);
	
	return sSel;
}

int CAutoComboBox::FindStringExact(int nIndexStart, LPCTSTR lpszFind) const
{ 
	return FindStringExact(nIndexStart, lpszFind, FALSE); 
}

int CAutoComboBox::FindStringExact(int nIndexStart, const CString& sItem, BOOL bCaseSensitive) const
{
	int nFind = nIndexStart; // default
	
	if (!sItem.IsEmpty())
	{
		// because more than one item might exist if were doing a case-sensitive
		// search we can't just stop if the first find doesn't exactly match
		// because there still may be further matches
		BOOL bContinue = TRUE;
		
		while (bContinue)
        {
			int nPrevFind = nFind;
			nFind = COwnerdrawComboBoxBase::FindStringExact(nFind, sItem);
			
			// if no match then definitely done
			if (nFind <= nPrevFind && nFind != nIndexStart)
			{
				nFind = CB_ERR;
				bContinue = FALSE;
			}
			else if (!bCaseSensitive)
			{
				bContinue = FALSE;
			}
			else
			{
				// else if (bCaseSensitive)
				ASSERT (nFind != CB_ERR);
				ASSERT (bCaseSensitive);
				
				// test for real exactness because FindStringExact is not case sensitive
				CString sFind;
				GetLBText(nFind, sFind);
				
				bContinue = !(sItem == sFind); // differ in case
			}
        }
	}
	else // special case: look for empty item
	{
		nFind = GetCount();

		while (nFind--)
		{
			CString sLBItem;
			GetLBText(nFind, sLBItem);

			if (sLBItem.IsEmpty())
				break;
		}
	}
	
	return nFind;
}

int CAutoComboBox::DeleteString(LPCTSTR szItem, BOOL bCaseSensitive)
{
	int nItem = FindStringExact(-1, szItem, bCaseSensitive);

	if (nItem != CB_ERR)
		return DeleteString(nItem);

	// else
	return CB_ERR;
}

int CAutoComboBox::DeleteItems(const CStringArray& aItems, BOOL bCaseSensitive)
{
	int nDeleted = 0;
	int nItem = aItems.GetSize();

	while (nItem--)
	{
		const CString& sItem = Misc::GetItem(aItems, nItem);

		if (DeleteString(sItem, bCaseSensitive) != CB_ERR)
			nDeleted++;
	}

	return nDeleted;
}

BOOL CAutoComboBox::AddEmptyString()
{
	int nItem = FindStringExact(-1, _T(""));

	if (nItem == CB_ERR)
	{
		if (AddToStart())
			nItem = COwnerdrawComboBoxBase::InsertString(0, _T(""));
		else
			nItem = COwnerdrawComboBoxBase::AddString(_T(""));
	}

	return (nItem != CB_ERR);
}

int CAutoComboBox::AddUniqueItem(const CString& sItem)
{
	return AddUniqueItem(sItem, AddToStart());
}

int CAutoComboBox::AddUniqueItem(const CString& sItem, BOOL bAddToStart)
{
	return InsertUniqueItem(bAddToStart ? 0 : -1, sItem);
}

CString CAutoComboBox::MakeItemKey(const CString& sItem) const
{
	CString sKey(sItem);

	// FindStringExact 默认不区分大小写, 索引键需保持同样语义
	if (!CaseSensitive())
		sKey.MakeUpper();

	return sKey;
}

void CAutoComboBox::RebuildKnownItems()
{
	m_mapKnownItems.RemoveAll();

	int nCount = GetCount();

	if (nCount > 0)
	{
		m_mapKnownItems.InitHashTable(nCount * 2 + 17);

		for (int nItem = 0; nItem < nCount; nItem++)
		{
			CString sItem;
			GetLBText(nItem, sItem);

			if (!sItem.IsEmpty())
			{
				CString sKey = MakeItemKey(sItem);
				m_mapKnownItems[(LPCTSTR)sKey] = nItem;
			}
		}
	}

	m_nKnownItemsCount = nCount;
}

//李波这个函数慢了 这里慢
int CAutoComboBox::InsertUniqueItem(int nIndex, const CString& sNewItem)
{
	if (nIndex < 0 || nIndex > GetCount())
		nIndex = -1; // insert at end if not exists

	CString sItem(sNewItem);
	Misc::Trim(sItem);
	
	if (!sItem.IsEmpty())
	{
		int nFind = CB_ERR;
		int nCurCount = GetCount();
		CString sKey;

		// 条目较少时线性查找更快, 且无需维护索引
		if (nCurCount < 64)
		{
			nFind = FindStringExact(-1, sItem, CaseSensitive());
		}
		else
		{
			// 快速路径: 用哈希索引定位, 避免逐条线性扫描
			// 索引条目数与控件实际条目数不一致时重建
			if (m_nKnownItemsCount != nCurCount)
				RebuildKnownItems();

			int nKnown = -1;
			sKey = MakeItemKey(sItem);

			if (m_mapKnownItems.Lookup((LPCTSTR)sKey, nKnown)
				&& nKnown >= 0 && nKnown < nCurCount)
			{
				CString sLBItem;
				GetLBText(nKnown, sLBItem);

				if (sLBItem == sItem)
				{
					nFind = nKnown;
				}
				else
				{
					// 大小写/内容存在差异, 退回原查找以保持原有替换语义
					nFind = FindStringExact(-1, sItem, CaseSensitive());
				}
			}
		}

		if (nFind != CB_ERR) // items already exists
		{
			CString sLBItem;
			GetLBText(nFind, sLBItem);

			if (nIndex == -1)
				nIndex = nFind; // leave it in it's current position
			
			if (nIndex != nFind || sItem != sLBItem)
			{
				// save selection so we can restore it
				int nSel = GetCurSel();
				CString sSelItem;
				
				if (nSel != CB_ERR)
					GetLBText(nSel, sSelItem);

				// be sure to transfer item data
				DWORD dwItemData = GetItemData(nFind);
				
				DeleteString(nFind); // remove original

				m_nKnownItemsCount = -1; // 删除会移动后续索引, 使其失效待重建

				if (GetStyle() & CBS_SORT)
					nIndex = COwnerdrawComboBoxBase::AddString(sItem); // re-insert
				else
					nIndex = COwnerdrawComboBoxBase::InsertString(nIndex, sItem); // re-insert

				SetItemData(nIndex, dwItemData);
				
				//李波 20191121 去掉刷新
				//if (nIndex != CB_ERR)
				//	RefreshDropWidth();
				
				// restore selection
				if (nSel != CB_ERR)
					SelectString(-1, sSelItem);
				
				return nIndex;
			}
		}
		else
		{
			if (GetStyle() & CBS_SORT)
				nIndex = COwnerdrawComboBoxBase::AddString(sItem); // re-insert
			else
				nIndex = COwnerdrawComboBoxBase::InsertString(nIndex, sItem); // re-insert
			

			//李波 20191121 去掉刷新
			//if (nIndex != CB_ERR)
			//	RefreshDropWidth();

			// 维护哈希索引
			if (nIndex != CB_ERR && m_nKnownItemsCount >= 0 && !sKey.IsEmpty())
			{
				m_mapKnownItems[(LPCTSTR)sKey] = nIndex;
				m_nKnownItemsCount = GetCount();
			}
			
			return nIndex;
		}
	}
	
	return CB_ERR; // invalid item
}

void CAutoComboBox::Flush() 
{ 
	if (m_bEditChange) 
		HandleReturnKey(); 
}
static bool G_bDisableRefreshDropWidth = true;
void CAutoComboBox::DisableRefreshDropWidth()
{
	G_bDisableRefreshDropWidth = false;
}
void CAutoComboBox::EnableRefreshDropWidth()
{
	G_bDisableRefreshDropWidth = true;
}

void CAutoComboBox::RefreshDropWidth()
{
	//李波  这里影响速度
	if (G_bDisableRefreshDropWidth == true)
	RefreshDropWidth(TRUE);
}

void CAutoComboBox::RefreshDropWidth(BOOL bRecalc)
{

	int nWidth = 0;

	if (bRecalc)
	{
		m_nMaxTextWidth = nWidth = CDialogHelper::CalcMaxTextWidth(*this, 0, TRUE);
	}
	else if (m_nMaxTextWidth > 0)
	{
		nWidth = m_nMaxTextWidth;
	}
	else
	{
		CRect rWindow;
		GetWindowRect(rWindow);

		nWidth = rWindow.Width();
	}
	
	int nMaxWidth = GetMaxDropWidth();
	
	if (nMaxWidth > 0)
		nWidth = min(nWidth, nMaxWidth);
	
	SetDroppedWidth(nWidth + GetExtraListboxWidth());

}

int CAutoComboBox::GetExtraListboxWidth() const
{
	// space for close button
	int nIcon = (HasIcon() ? 18 : 0);
	int nClose = (IsType(CBS_DROPDOWNLIST) ? 0 : SIZE_CLOSEBTN);

	return (nIcon + nClose);
}

BOOL CAutoComboBox::GetListDeleteButtonRect(const CRect& rItem, CRect& rBtn) const
{
	if (IsType(CBS_DROPDOWNLIST))
		return FALSE;

	rBtn = rItem;

	rBtn.left = rBtn.right - SIZE_CLOSEBTN;
	rBtn.DeflateRect(0, ((rBtn.Height() - SIZE_CLOSEBTN) / 2));

	return TRUE;
}

void CAutoComboBox::DrawItemText(CDC& dc, const CRect& rect, int nItem, UINT nItemState,
								DWORD dwItemData, const CString& sItem, BOOL bList)
{
	COwnerdrawComboBoxBase::DrawItemText(dc, rect, nItem, nItemState, dwItemData, sItem, bList);

	// draw 'delete' mark for non-empty deletable list items
	if (bList && !IsType(CBS_DROPDOWNLIST) && !sItem.IsEmpty())
	{
		// create font first time
		if (m_fontClose.GetSafeHandle() == NULL)
			GraphicsMisc::CreateFont(m_fontClose, _T("Marlett"), 6);
		
		CFont* pOldFont = (CFont*)dc.SelectObject(&m_fontClose);
		
		CRect rBtn;
		VERIFY(GetListDeleteButtonRect(rect, rBtn));
		
		// set the color to white-on-red if selected
		if (nItemState & ODS_SELECTED)
		{
			dc.SetTextColor(WHITE);
			dc.FillSolidRect(rBtn, RED);
		}
		else
		{
			dc.SetTextColor(GetSysColor(COLOR_3DDKSHADOW));
		}

		dc.SetTextAlign(TA_TOP | TA_LEFT);
		dc.SetBkMode(TRANSPARENT);
		dc.TextOut(rBtn.left + 1, rBtn.top + 1, STR_CLOSEBTN);
		dc.SelectObject(pOldFont);
	}
}

void CAutoComboBox::OnSize(UINT nType, int cx, int cy) 
{
	CWnd* pEdit = GetDlgItem(1001);

	// if the edit control does not have the focus then hide the selection
	if (pEdit && pEdit != GetFocus())
	{
		CHoldRedraw hr(*pEdit);
		COwnerdrawComboBoxBase::OnSize(nType, cx, cy);
	
		pEdit->SendMessage(EM_SETSEL, (UINT)-1, 0);
	}
	else
	{
		COwnerdrawComboBoxBase::OnSize(nType, cx, cy);
	}

	RefreshDropWidth(FALSE);
}

int CAutoComboBox::AddUniqueItems(const CStringArray& aItems)
{
 
	
    int nItemCount = aItems.GetSize(), nCount = 0;

	// maintain order
    for (int nItem = 0; nItem < nItemCount; nItem++)
    {
		const CString& sItem = Misc::GetItem(aItems, nItem);

        if (AddUniqueItem(sItem, FALSE) != CB_ERR)
            nCount++;
    }

	if (nCount)
		RefreshDropWidth();

    return nCount;
}

int CAutoComboBox::AddUniqueItems(const CAutoComboBox& cbItems)
{
    CStringArray aItems;

    if (cbItems.GetItems(aItems))
        return AddUniqueItems(aItems);

    // else
    return 0;
}

int CAutoComboBox::GetItems(CStringArray& aItems) const
{
    int nItem = GetCount();

    aItems.RemoveAll();
    aItems.SetSize(nItem);

    while (nItem--)
    {
        CString sItem;
		GetLBText(nItem, sItem);

        aItems.SetAt(nItem, sItem); // maintain order
    }

    return aItems.GetSize();
}

// messages destined for the edit control
LRESULT CAutoComboBox::WindowProc(HWND /*hRealWnd*/, UINT msg, WPARAM wp, LPARAM lp)
{
	return OnEditboxMessage(msg, wp, lp);
}

// this handles messages destined for the dropped listbox
LRESULT CAutoComboBox::ScWindowProc(HWND /*hRealWnd*/, UINT msg, WPARAM wp, LPARAM lp)
{
	return OnListboxMessage(msg, wp, lp);
}

BOOL CAutoComboBox::DoDeleteListItem(const CPoint& ptClient)
{
	ASSERT(GetDroppedState());

	if (m_nDeleteItem != LB_ERR)
	{
		int nItem = m_nDeleteItem;
		m_nDeleteItem = LB_ERR;
		
		if (HitTestListDeleteBtn(ptClient) == nItem)
		{
			return DeleteLBItem(nItem);
		}
	}

	// else
	return FALSE;
}

LRESULT CAutoComboBox::OnListboxMessage(UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) 
	{
	case WM_LBUTTONDOWN:
		m_nDeleteItem = HitTestListDeleteBtn(lp);

		if (m_nDeleteItem != -1)
			return 1L;
		break;
		
	case WM_LBUTTONUP:
		if (DoDeleteListItem(lp))
		{
			return 1L; // prevent combo closing
		}
		break;
	}
	
	return CSubclasser::ScWindowProc(GetListbox(), msg, wp, lp);
}

LRESULT CAutoComboBox::OnEditboxMessage(UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
	case WM_KEYDOWN:
		// <Ctrl> + <Delete>
		if (AllowDelete() && (wp == VK_DELETE) && GetDroppedState() && Misc::IsKeyPressed(VK_CONTROL))
		{
			if (DeleteLBItem(ScSendMessage(LB_GETCURSEL)))
			{
				// eat message else it'll go to the edit window
				return 0L;
			}
		}
		// <Return>
		else if (wp == VK_RETURN)
		{
			if (GetDroppedState() && !IsType(CBS_SIMPLE))
				ShowDropDown(FALSE);

			HandleReturnKey();
			
			return 0L;
		}
		else if (wp == VK_DOWN && !GetDroppedState() && !IsType(CBS_SIMPLE) && GetCount())
		{
			ShowDropDown();
			return 0L; // eat
		}
		break;
		
	case WM_GETDLGCODE:
		// special handling of the return key
		if (lp)
		{
			LPMSG pMsg = (LPMSG)lp;

			if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
			{
				// can be simple or not dropped
				if (IsType(CBS_SIMPLE) || !GetDroppedState())
					return DLGC_WANTALLKEYS;
			}
		}
		break;

	case WM_KILLFOCUS:
		if (m_bEditChange)
			HandleReturnKey();
		break;
	}

	return CSubclassWnd::WindowProc(GetEdit(), msg, wp, lp);
}

int CAutoComboBox::HitTestListDeleteBtn(const CPoint& ptList) const
{
	if (!IsType(CBS_DROPDOWNLIST))
	{
		int nItem = HitTestList(ptList);
		
		if ((nItem != LB_ERR) && ::SendMessage(GetListbox(), LB_GETTEXTLEN, nItem, 0))
		{
			CRect rItem, rBtn;
			
			if (LB_ERR != ::SendMessage(GetListbox(), LB_GETITEMRECT, nItem, (LPARAM)(LPRECT)rItem))
			{
				rItem.left = rItem.right - SIZE_CLOSEBTN;

				if (rItem.PtInRect(ptList))
					return nItem;
			}
		}
	}

	// else
	return LB_ERR;
}

int CAutoComboBox::HitTestList(const CPoint& ptList) const
{
	DWORD dwHit = ::SendMessage(GetListbox(), LB_ITEMFROMPOINT, 0, MAKELPARAM(ptList.x, ptList.y));

	if (HIWORD(dwHit))
		return LB_ERR;

	// else
	return LOWORD(dwHit);
}

int CAutoComboBox::GetCurSel() const
{
	CAutoFlag af(m_bDrawing, FALSE);
	
	return CComboBox::GetCurSel();
}

void CAutoComboBox::HandleReturnKey()
{
	m_bEditChange = FALSE;

	if (IsHooked())
	{
		CString sEdit;
		GetCWnd()->GetWindowText(sEdit);
		
		int nAdd = AddUniqueItem(sEdit);
		
		if (nAdd != CB_ERR)
		{
			ParentACNotify(WM_ACBN_ITEMADDED, nAdd, sEdit);
		}

		if (!sEdit.IsEmpty())
			SelectString(-1, sEdit);
			
		// send a possible selection change
		ParentCBNotify(CBN_SELCHANGE);
	}
}

void CAutoComboBox::ParentACNotify(UINT nMsgNotify, int nIndex, LPCTSTR szItem)
{
	ASSERT((nIndex >= 0) && (!szItem || szItem[0]));

	if ((nIndex >= 0) && (!szItem || szItem[0]))
	{
		GetParent()->SendMessage(nMsgNotify, MAKEWPARAM(GetDlgCtrlID(), nIndex), (LPARAM)szItem);
	}
}

void CAutoComboBox::ParentCBNotify(UINT nIDNotify)
{
	CWnd* pParent = GetParent();

	if (pParent)
	{
		UINT nID = GetDlgCtrlID();
		pParent->SendMessage(WM_COMMAND, MAKEWPARAM(nID, nIDNotify), (LPARAM)m_hWnd);
	}
}

BOOL CAutoComboBox::IsType(UINT nComboType) const
{
	return ((GetStyle() & 0xf) == nComboType);
}

BOOL CAutoComboBox::AllowDelete() const 
{ 
	if (Misc::HasFlag(m_dwFlags, ACBS_ALLOWDELETE))
	{
		return ((GetStyle() & 0xf) != CBS_DROPDOWNLIST);
	}

	else return FALSE;
}

void CAutoComboBox::SetEditMask(LPCTSTR szMask, DWORD dwMaskFlags)
{
	m_eMask.SetMask(szMask, dwMaskFlags);
}

HBRUSH CAutoComboBox::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	HBRUSH hbr = COwnerdrawComboBoxBase::OnCtlColor(pDC, pWnd, nCtlColor);
	
	// hook list box before base class subclasses it
	if (nCtlColor == CTLCOLOR_LISTBOX && !ScIsHooked())
	{
		ScHookWindow(pWnd->GetSafeHwnd());
	}
	// and hook edit box
	else if (nCtlColor == CTLCOLOR_EDIT && !IsHooked())
	{
		HookWindow(pWnd->GetSafeHwnd());

		// mask
		if (m_eMask.IsMasked())
			m_eMask.SubclassWindow(pWnd->GetSafeHwnd());
	}
	
	return hbr;
}

BOOL CAutoComboBox::DeleteLBItem(int nItem)
{
	ASSERT(AllowDelete());
	ASSERT((nItem >= 0) && (nItem < GetCount()));

	if ((nItem >= 0) && (nItem < GetCount()))
	{
		CString sCurItem, sItem;
		int nCurSel = GetCurSel();
		
		// save existing selection
		if (nCurSel != nItem)
			GetWindowText(sCurItem);
		
		GetLBText(nItem, sItem); // need this for notifying parent
		::SendMessage(GetSafeHwnd(), CB_DELETESTRING, nItem, 0);
		
		// restore combo selection
		if (!sCurItem.IsEmpty())
		{
			nCurSel = FindStringExact(-1, sCurItem);
		}
		else if (GetCount() == 0)
		{
			nCurSel = CB_ERR; // we've deleted the last item
			SetWindowText(_T("")); // clear edit
			m_bEditChange = TRUE;
		}
		ScSendMessage(LB_SETCURSEL, nCurSel);
		
		// notify parent that we've been fiddling
		ParentACNotify(WM_ACBN_ITEMDELETED, nItem, sItem);

		// resize our window
		if (!IsType(CBS_SIMPLE))
		{
			if (GetCount())
			{
				CRect rList;
				ScGetCWnd()->GetWindowRect(rList);
				rList.bottom -= GetItemHeight(0);
				
				ScGetCWnd()->MoveWindow(rList, TRUE);
			}
			else
			{
				ShowDropDown(FALSE);
			}
		}

		return TRUE;
	}

	// else
	return FALSE;
}


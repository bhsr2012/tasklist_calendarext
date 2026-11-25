/* PERF__FREZ/PUB
 * ====================================================
 * CalendarCtrl.cpp : implementation file
 * Frederic Rezeau
 * 
 * Perf'Control Personal Edition calendar 
 *
 * If you like it, feel free to use it. 
 *
 * www.performlabs.com 
 * ==================================================== 
 */

#include "stdafx.h"
#include "CalendarCtrl.h"

#include "..\shared\datehelper.h"

#include <math.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//拆分字符串, CString 拆分为 vector<string>
void SplitString(const CString &SourceStr, const CString &SplitStr, vector<CString> & outstrvec)
{
	outstrvec.resize(0);
	outstrvec.reserve(6);

	//char *str; 
	int LenofSplitStr;				//SplitStr的长度
	int LenofSourceStr;				//SourceStr的长度
	int nstart = 0, noldstart = 0;	//寻找拆分字符串时的起始点
									//int nEndSplitstr;				//SourceStr末尾是SplitStr的位置
	int nBgnSplitstr = 0;			//SourceStr开头是SplitStr的位置
	int n = 0;						//拆分成几个

	LenofSplitStr = SplitStr.GetLength();
	LenofSourceStr = SourceStr.GetLength();

	CString szItem;
	while (nstart >= 0)
	{
		nstart = SourceStr.Find(SplitStr, nstart);
		if (nstart == 0) nBgnSplitstr = 1;
		if (nstart == -1) break;

		if ((nstart - noldstart > 0))
		{
			szItem = SourceStr.Mid(noldstart, nstart - noldstart);
			if (szItem && szItem.GetLength() > 0)
				outstrvec.push_back(szItem);
		}

		nstart = nstart + LenofSplitStr;
		noldstart = nstart;
		n = n + 1;
	}
	if (n == 0)
	{
		outstrvec.push_back(SourceStr);
		return;
	}

	szItem = SourceStr.Mid(noldstart, LenofSourceStr - noldstart);
	if (szItem.GetLength() > 0)
		outstrvec.push_back(szItem);
}

/////////////////////////////////////////////////////////////////////////////
// CCalendarCtrl
static int G_nCellHeight = 20;
CCalendarCtrl::CCalendarCtrl() : m_nVisibleWeeks(4), m_bShowWeekends(TRUE), m_bEnableMultiSel(FALSE), m_nLeftGutter(0)
{
	LOGFONT lf;
	::GetObject((HFONT)GetStockObject(DEFAULT_GUI_FONT),sizeof(lf),&lf);
	lstrcpy(lf.lfFaceName, _T("Arial"));
	lf.lfHeight = 13;
	m_DefaultFont.CreateFontIndirect(&lf);

	m_bMonthIsOdd		= false;
	m_bSelectionStarted = false;
	m_SelectionRange[0]	= 0;
	m_SelectionRange[1]	= 0;
	m_SelectionRange[2]	= 0;
	m_nVscrollMax		= 110;
	m_nVscrollPos		= m_nVscrollMax/2;
 
	m_BoundDown			= COleDateTime(1999,12,31,0,0,0); 
	m_BoundUp			= COleDateTime(2099,1,1,0,0,0);
	m_nMaxSel			= 42;
	m_nFirstWeekDay		= 1;
	m_pfnDataCallback	= NULL;
	m_crTheme			= GetSysColor(COLOR_3DFACE);

	CALENDAR_NUM_COLUMNS = 14;

	//以下变量在设备模式下起作用
	m_bEquipmentMode	= false;  
	m_nEquipmentVisableRows = 5;    //一页显示的设备数
	m_nEquipmentCount	= 0;
	m_nBeginDay			= 40000;	//日历的开始日期
	m_nEndDay			= 40060;	//日历的结束日期

	m_Equipments.resize(0);
	//m_Equipments[0].EquipmentID = _T("01");
	//m_Equipments[0].EquipmentName = _T("设备1");
 
	m_DragingEquipments.resize(0);
 
	if (m_bEquipmentMode == true)
	{
		m_nVscrollMax		=  m_nEquipmentCount ;
		m_nVscrollPos		=  0;

		m_nHscrollMax		= m_nEndDay - m_nBeginDay + 1;
		m_nHscrollPos		= 0;
		CALENDAR_NUM_COLUMNS = 14;
	}
 
}

CCalendarCtrl::~CCalendarCtrl()
{
}

int CCalendarCtrl::GetCellHeight()
{
	return G_nCellHeight;
}

BEGIN_MESSAGE_MAP(CCalendarCtrl, CWnd)
	//{{AFX_MSG_MAP(CCalendarCtrl)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_VSCROLL()
	ON_WM_HSCROLL()
	ON_WM_MOUSEWHEEL()
	ON_WM_LBUTTONDBLCLK()
	//}}AFX_MSG_MAP
	ON_WM_KEYDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_GETDLGCODE()
END_MESSAGE_MAP()


BOOL CCalendarCtrl::Create(DWORD dwStyle, const CRect &wndRect, CWnd *pParent, UINT nID, LPFN_CALENDAR_DATA_CALLBACK pfnDataCallback)
{
	BOOL bResult = CWnd::Create(NULL, NULL, dwStyle, wndRect, pParent, nID);
	m_pfnDataCallback = pfnDataCallback;

    SetScrollRange(SB_VERT, 0, m_nVscrollMax, FALSE);
	SetScrollRange(SB_HORZ, 0, m_nHscrollMax, FALSE);
 
	Reset();

	return bResult;
}



void CCalendarCtrl::SetEquipmentMode( bool bEquipmentMode)
{
	if (bEquipmentMode && m_nEquipmentCount > 0)
	{
		m_bEquipmentMode = bEquipmentMode;
	}
	else if (bEquipmentMode == false)
	{
		m_bEquipmentMode = bEquipmentMode;
	}
}
 
void CCalendarCtrl::SetBeginEndDay(int nBegin, int nEnd)
{
	m_nBeginDay = nBegin;
	m_nEndDay = nEnd;
}
 
BOOL CCalendarCtrl::SetVisibleWeeks(int nWeeks)
{
	ASSERT(nWeeks > 0 && nWeeks <= CALENDAR_MAX_ROWS);

	if (nWeeks < 1 || nWeeks > CALENDAR_MAX_ROWS)
		return FALSE;

	m_nVisibleWeeks = nWeeks;
	Invalidate();

	return TRUE;
}


BOOL CCalendarCtrl::SetVisibleColumns(int nColumns)
{
 
	if (nColumns < 1 || nColumns > CALENDAR_NUM_COLUMNS_MAX)
		return FALSE;
	
	CALENDAR_NUM_COLUMNS = nColumns;
  
	Goto(m_DateCurrent, false); 
	Invalidate();
 
	
	return TRUE;
}

BOOL CCalendarCtrl::SetVisibleEquipments(int nRows)
{
 
	if (nRows < 1 || nRows > CALENDAR_MAX_ROWS)
		return FALSE;
	
	m_nEquipmentVisableRows = nRows;
 
	Invalidate();
 
	return TRUE;
}

// equipment rows that fit the current client height without going below
// the minimum row height
int CCalendarCtrl::GetEffectiveEquipRows() const
{
	int nRows = m_nEquipmentVisableRows;
	if (nRows < 1)
		nRows = 1;

	CRect rc;
	GetGridRect(rc);

	int nAvail = rc.Height() - CALENDAR_HEADER_HEIGHT;
	int nMaxFit = (nAvail > 0) ? (nAvail / CALENDAR_MIN_EQUIP_ROW_HEIGHT) : 0;
	if (nMaxFit < 1)
		nMaxFit = 1;

	if (nRows > nMaxFit)
		nRows = nMaxFit;

	return nRows;
}

int CCalendarCtrl::GetRowHeight() const
{
	CRect rc;
	GetGridRect(rc);

	int nAvail = rc.Height() - CALENDAR_HEADER_HEIGHT;
	if (nAvail < 1)
		nAvail = 1;

	int nRows = m_bEquipmentMode ? GetEffectiveEquipRows() : GetVisibleWeeks();
	if (nRows < 1)
		nRows = 1;

	int nHeight = nAvail / nRows;

	if (m_bEquipmentMode && nHeight < CALENDAR_MIN_EQUIP_ROW_HEIGHT)
		nHeight = CALENDAR_MIN_EQUIP_ROW_HEIGHT;

	return max(1, nHeight);
}


void CCalendarCtrl::Reset()
{
	COleDateTime today(time(NULL));
	COleDateTime dtFirstDay(today.GetYear(), today.GetMonth(), 1,0,0,0); 
	
	if (dtFirstDay > m_BoundUp) 
		dtFirstDay = m_BoundUp;

	else if (dtFirstDay < m_BoundDown)	
		dtFirstDay = m_BoundDown;

	if (dtFirstDay.GetMonth()%2) 
		m_bMonthIsOdd = true;

	Goto(dtFirstDay);

	// Reset selection
	m_SelectionRange[1] = DateToSeconds(COleDateTime(today.GetYear(), today.GetMonth(), today.GetDay(),0,0,0));
	m_SelectionRange[0] = m_SelectionRange[1];

	// Reset scroll pos
	m_nVscrollPos = m_nVscrollMax/2;
	if (m_bEquipmentMode) m_nVscrollPos = 0;
	SetScrollPos(SB_VERT, m_nVscrollPos, TRUE);
}

void CCalendarCtrl::GotoToday(bool bSelect) 
{ 
	Goto(COleDateTime::GetCurrentTime(), bSelect); 
}
 
#include "..\ToDoList\tdcmsg.h"
void CCalendarCtrl::Goto(const COleDateTime& dtDate, bool bSelect)
{
	if ((dtDate < m_BoundUp) && 
		(dtDate > m_BoundDown))
	{
		m_DateCurrent = dtDate;	
		
		if (m_nFirstWeekDay<1 || m_nFirstWeekDay>7)
			m_nFirstWeekDay = 1;

		COleDateTime dtIte(m_DateCurrent.GetYear(), m_DateCurrent.GetMonth(), m_DateCurrent.GetDay(),0,0,0);
		int narr[7];

		for (int d=0; d<7; d++)	
			narr[((m_nFirstWeekDay-1)+d)%7] = d;

		int nCellStart = narr[dtIte.GetDayOfWeek()-1], i;

		for(i=0; i<nCellStart; i++)	
			dtIte -= COleDateTimeSpan(1,0,0,0);

		if (m_bEquipmentMode == false)
		{
			for(i=0; i<CALENDAR_MAX_ROWS ; i++)
			{
				for(int u=0; u<CALENDAR_NUM_COLUMNS; u++)
				{
					// Init the cell
					m_dayCells[i][u].nEquipmentIndex = -1;
					m_dayCells[i][u].date = dtIte;
					m_dayCells[i][u].bPartial = false;
					m_dayCells[i][u].bMark = false;
					m_dayCells[i][u].csaLines.RemoveAll();
					
					if (m_pfnDataCallback)
						m_pfnDataCallback(this, DateToSeconds(m_dayCells[i][u].date));
					
					dtIte += COleDateTimeSpan(1,0,0,0);
				}
			}
		}
		else
		//对当前显示的所有cell初始化，计算其应该显示的日期
		{
			for(i=0; i<CALENDAR_MAX_ROWS ; i++)
			{
				dtIte.m_dt =(int) m_DateCurrent.m_dt ; 
				for(int u=0; u<CALENDAR_NUM_COLUMNS; u++)
				{
					// Init the cell
					int nEquipmentIndex = i + m_nVscrollPos;
					if (nEquipmentIndex<0 || nEquipmentIndex>=m_Equipments.size()  )
					{
						nEquipmentIndex = -1;
					}
					m_dayCells[i][u].nEquipmentIndex = nEquipmentIndex;
					m_dayCells[i][u].date = dtIte;
					m_dayCells[i][u].bPartial = false;
					m_dayCells[i][u].bMark = false;
					m_dayCells[i][u].csaLines.RemoveAll();
					
					if (m_pfnDataCallback)
						m_pfnDataCallback(this, DateToSeconds(m_dayCells[i][u].date));
					
					dtIte += COleDateTimeSpan(1,0,0,0);
				}
			}

// 			m_nHscrollPos =  m_DateCurrent.m_dt - m_nBeginDay;
// 			SetScrollPos(SB_HORZ, m_nHscrollPos, TRUE);
		}
 

		if (bSelect)
		{
			m_SingleSelection.RemoveAll();

			m_SelectionRange[2] = 0;
			m_SelectionRange[1] = DateToSeconds(COleDateTime(m_DateCurrent.GetYear(), m_DateCurrent.GetMonth(), m_DateCurrent.GetDay(),0,0,0));
			m_SelectionRange[0] = m_SelectionRange[1];

			if (m_bEquipmentMode == false)
			{ 
				// Scrolling pos
				COleDateTime today(time(NULL));
				m_nVscrollPos = (m_nVscrollMax/2) + (m_DateCurrent-today).GetDays()/7;

				SetScrollPos(SB_VERT, m_nVscrollPos, TRUE);
			}
		}
	}
	//李波尝试加入重载消息
	//GetParent()->GetParent()->GetParent()->SendMessage(WM_TDCN_ROLOAD, TRUE, NULL);
	 


	Invalidate(true);
}

void CCalendarCtrl::UpdateCells()
{
	if (m_pfnDataCallback)
		Goto(m_DateCurrent, false);
}

void CCalendarCtrl::SetThemeColour(COLORREF crTheme)
{
	if (crTheme == CLR_NONE)
		m_crTheme = GetSysColor(COLOR_3DFACE);
	else
		m_crTheme = crTheme;
	
	if (GetSafeHwnd())
		Invalidate();
}

/////////////////////////////////////////////////////////////////////////////
// CCalendarCtrl message handlers

void CCalendarCtrl::OnPaint() 
{
	CPaintDC dc(this);
	CRect rc;
	GetClientRect(&rc);
 
	CDC MemDC;
	MemDC.CreateCompatibleDC(&dc);
	CBitmap MemBitmap;
	MemBitmap.CreateCompatibleBitmap(&dc, rc.Width(), rc.Height());
	CBitmap *pOldBitmap = MemDC.SelectObject(&MemBitmap);

	// Always repaint the whole calendar. A partial update rect can leave
	// stale pixels from the previously active view (task tree/list) behind.
	MemDC.FillSolidRect(rc, GetSysColor(COLOR_WINDOW));
	MemDC.SetBkMode(TRANSPARENT);

	// The left gutter holds the per-row scrollbars and is never covered by a
	// cell, so it would otherwise expose the system window colour. Paint it
	// with the cell background instead.
	{
		CRect rcGrid;
		GetGridRect(rcGrid);

		if (rcGrid.left > rc.left)
		{
			CRect rcGutter(rc.left, 0, rcGrid.left, rc.bottom);
			MemDC.FillSolidRect(rcGutter, CALENDAR_WHITE);
		}
	}
	
	if( m_bEquipmentMode == false)
	{
		ShowScrollBar(SB_HORZ, false);
	}
 
	// Draw calendar elements
				
	DrawCells(&MemDC);	
	DrawHeader(&MemDC);
	DrawGrid(&MemDC);
 

	// Render
	dc.BitBlt(0, 0, rc.Width(), rc.Height(), &MemDC, 0, 0, SRCCOPY);
	MemDC.SelectObject(pOldBitmap);
}

const CCalendarCell* CCalendarCtrl::GetCell(int nRow, int nCol) const
{
	ASSERT(nRow >= 0 && nRow < CALENDAR_MAX_ROWS);
	ASSERT(nCol >= 0 && nCol < CALENDAR_NUM_COLUMNS);


	return &(m_dayCells[nRow][nCol]);
}

CCalendarCell* CCalendarCtrl::GetCell(int nRow, int nCol)
{
	ASSERT(nRow >= 0 && nRow < CALENDAR_MAX_ROWS);
	ASSERT(nCol >= 0 && nCol < CALENDAR_NUM_COLUMNS);

	return &(m_dayCells[nRow][nCol]);
}

BOOL CCalendarCtrl::PtInHeader(const CPoint& ptScreen) const
{
	CPoint ptHeader(ptScreen);
	ScreenToClient(&ptHeader);

	CRect rHeader;
	GetGridRect(rHeader);
	rHeader.bottom = rHeader.top + CALENDAR_HEADER_HEIGHT;

	return rHeader.PtInRect(ptHeader);
}

int CCalendarCtrl::GetDayOfWeek(int nColumn) const
{
	return ((((m_nFirstWeekDay - 1) + nColumn) % 7) + 1);
}

void CCalendarCtrl::DrawHeader(CDC* pDC)
{
	CRect rc;
	GetGridRect(rc);
	rc.bottom = rc.top + CALENDAR_HEADER_HEIGHT;

	CRect rcLine(rc);
	rcLine.top = rcLine.bottom-1;

	int i;

	for(i=0; i<CALENDAR_HEADER_HEIGHT; i++)
	{
		pDC->FillSolidRect(rcLine, GetFadedThemeColour(i*10));
		rcLine.bottom--;
		rcLine.top = rcLine.bottom-1;
	}

	CFont* pOldFont = pDC->SelectObject(&m_DefaultFont);
	float nWidth = 1.0 * rc.Width()/CALENDAR_NUM_COLUMNS;
	if (m_bEquipmentMode && nWidth < CALENDAR_FIRST_COL_WIDTH )
	{
		nWidth = ( 1.0 * rc.Width() - CALENDAR_FIRST_COL_WIDTH ) / ( CALENDAR_NUM_COLUMNS - 1 );
	}


	bool bShort = false;
 
	CRect txtRect;
	 
	for(i=0; i<CALENDAR_NUM_COLUMNS; i++)
	{
		//CRect txtRect(i*nWidth, 0, (i+1)*nWidth, CALENDAR_HEADER_HEIGHT);

		if (m_bEquipmentMode && nWidth < CALENDAR_FIRST_COL_WIDTH)
		{
			txtRect = CRect(rc.left+(i-1)*nWidth+CALENDAR_FIRST_COL_WIDTH, 0, rc.left+(i)*nWidth, CALENDAR_HEADER_HEIGHT);
		}
		else
		{
			txtRect = CRect(rc.left+i*nWidth, 0, rc.left+(i+1)*nWidth, CALENDAR_HEADER_HEIGHT);
		}
		
		if (i  == (CALENDAR_NUM_COLUMNS - 1))
		{
			txtRect.right = rc.right;
		}
		else if (i == 0 && m_bEquipmentMode)
		{
			if ( nWidth < CALENDAR_FIRST_COL_WIDTH)
			{
				txtRect.left = rc.left;
				txtRect.right = txtRect.left + CALENDAR_FIRST_COL_WIDTH;
			}
		}
		else
		{
			txtRect.right = txtRect.left + nWidth;
		}
		


		
		int nDOW = GetDayOfWeek(i);
		
		CSize TitleSize(pDC->GetTextExtent(CDateHelper::GetWeekdayName(nDOW, FALSE)));
		
		if (TitleSize.cx > txtRect.Width())
		{
			bShort = true;
			break;
		}
	}
	
	for(i=0 ; i<CALENDAR_NUM_COLUMNS; i++)
	{
		//CRect txtRect(i*nWidth, 2, (i+1)*nWidth, CALENDAR_HEADER_HEIGHT);

		if (m_bEquipmentMode && nWidth < CALENDAR_FIRST_COL_WIDTH)
		{
			txtRect = CRect(rc.left+(i-1)*nWidth+CALENDAR_FIRST_COL_WIDTH, 0, rc.left+(i)*nWidth, CALENDAR_HEADER_HEIGHT);
		}
		else
		{
			txtRect = CRect(rc.left+i*nWidth, 0, rc.left+(i+1)*nWidth, CALENDAR_HEADER_HEIGHT);
		}
		
		if (i  == (CALENDAR_NUM_COLUMNS - 1))
		{
			txtRect.right = rc.right;
		}
		else if (m_bEquipmentMode && i == 0)
		{
			if ( nWidth < CALENDAR_FIRST_COL_WIDTH)
			{
				txtRect.left = rc.left;
				txtRect.right = txtRect.left + CALENDAR_FIRST_COL_WIDTH;
			}
		}
		else
		{
			txtRect.right = txtRect.left + nWidth;
		}

 	
		int nDOW = m_dayCells[0][i].date.GetDayOfWeek()  ; //GetDayOfWeek(i);

		CString strWeekDay = CDateHelper::GetWeekdayName(nDOW, bShort);
		if ( m_bEquipmentMode )
		{
		 
			int nMonth = m_dayCells[0][i].date.GetMonth();
			
			strWeekDay = CDateHelper::GetMonthName(nMonth, FALSE) + _T("  ") + strWeekDay;
		}
		
		pDC->DrawText(strWeekDay, txtRect, DT_CENTER|DT_VCENTER);
	}
 

	pDC->SelectObject(pOldFont);
}

void CCalendarCtrl::DrawGrid(CDC* pDC)
{
	CRect rc;
	GetGridRect(rc);

	int nRows = GetVisibleWeeks();

	if ( m_bEquipmentMode == true )
	{
		nRows = GetEffectiveEquipRows();
	}

	float nHeight = (float)GetRowHeight();

	float nWidth = 1.0 * rc.Width()/CALENDAR_NUM_COLUMNS;
	if (m_bEquipmentMode && nWidth < CALENDAR_FIRST_COL_WIDTH )
	{
		nWidth = ( 1.0 * rc.Width() - CALENDAR_FIRST_COL_WIDTH ) / ( CALENDAR_NUM_COLUMNS - 1 );
	}


	CPen thinPen(PS_SOLID, 1, RGB(125,175,255));
	CPen* pOldPen = pDC->SelectObject(&thinPen);

	int i;

	for(i=1; i<CALENDAR_NUM_COLUMNS; i++)
	{
		if (m_bEquipmentMode && nWidth < CALENDAR_FIRST_COL_WIDTH )
		{
			if ( i==1 )
			{
				pDC->MoveTo(rc.left + CALENDAR_FIRST_COL_WIDTH, CALENDAR_HEADER_HEIGHT);
				pDC->LineTo(rc.left + CALENDAR_FIRST_COL_WIDTH, rc.Height());
			}
			else
			{
				pDC->MoveTo(rc.left + CALENDAR_FIRST_COL_WIDTH + (i-1)*nWidth, CALENDAR_HEADER_HEIGHT);
				pDC->LineTo(rc.left + CALENDAR_FIRST_COL_WIDTH + (i-1)*nWidth, rc.Height());
			}

		}
		else
		{
			pDC->MoveTo(rc.left+i*nWidth-1, CALENDAR_HEADER_HEIGHT);
			pDC->LineTo(rc.left+i*nWidth-1, rc.Height());
		}
	}

	if ( m_bEquipmentMode == true )
	{
		if ( nRows > m_nEquipmentCount + 1 )
		{
			nRows = m_nEquipmentCount + 1;
		}
	}



	
	for(i=1; i<nRows; i++)
	{
		pDC->MoveTo(rc.left,		(i*nHeight)+CALENDAR_HEADER_HEIGHT);
		pDC->LineTo(rc.right,	(i*nHeight)+CALENDAR_HEADER_HEIGHT);
	}


	pDC->SelectObject(pOldPen);
}

void CCalendarCtrl::DrawCells(CDC* pDC)
{
	CRect rc;
	GetGridRect(rc);
// 	int ncHeight = (rc.Height()-CALENDAR_HEADER_HEIGHT)/GetVisibleWeeks();
// 	int ncWidth = rc.Width()/CALENDAR_NUM_COLUMNS;
	bool bResult = false;

	// only paint cells that intersect the update region: a selection
	// change invalidates two task bars, not the whole grid
	CRect rClipBox;
	pDC->GetClipBox(rClipBox);

	CFont* pOldFont = pDC->SelectObject(&m_DefaultFont);

	int nRows = GetVisibleWeeks();
	int nHeightRows = nRows;
	if (m_bEquipmentMode == true)
	{
		int nVisRows = GetEffectiveEquipRows();

		if ( nVisRows > m_nEquipmentCount  )
		{
			nRows = m_nEquipmentCount - m_nVscrollPos ;
		}
		else
		{
			if ( m_nVscrollPos + nVisRows > m_nEquipmentCount )
			{
				nRows = m_nEquipmentCount - m_nVscrollPos;
			}
			else
			{
				nRows = nVisRows; // - m_nVscrollPos;
			}
		}

		nHeightRows = nVisRows;
	}
 
	for(int i=0; i<nRows ; i++)
	{
		for(int u=0; u<CALENDAR_NUM_COLUMNS ; u++)
		{
			CRect rCell;

			if (GetCellRect(i, u, rCell))
			{
				CRect rHit;

				if (!rClipBox.IsRectEmpty() && !rHit.IntersectRect(rCell, rClipBox))
					continue;

				const CCalendarCell* pCell = GetCell(i, u);
  
				if (u == CALENDAR_NUM_COLUMNS-1) 
					rCell.right = rc.right;

				if (i == nHeightRows-1) 
					rCell.bottom = rc.bottom;

				// test for showing month
				BOOL bShowMonth = ((pCell->date.GetDay() == 1) || (i==0 && u==0));

				// test for today
				COleDateTime today(time(NULL));
				
				bool bToday = (today.GetDay() == pCell->date.GetDay() && 
								today.GetMonth() == pCell->date.GetMonth() && 
								today.GetYear() == pCell->date.GetYear());

				// test for selected
				bool bSelected = false;
				time_t tcur = DateToSeconds(pCell->date);	
				
				if (m_SingleSelection.GetCount())
				{	
					m_SingleSelection.Lookup(tcur, bSelected);
				}
				else
				{
					time_t tmax = max(m_SelectionRange[0], m_SelectionRange[1]);
					time_t tmin = min(m_SelectionRange[0], m_SelectionRange[1]);		
					
					bSelected = ((tmax >= tcur) && (tcur >= tmin));
				}

				bResult = DrawCell(pDC, pCell, rCell, bSelected, bToday, bShowMonth);

				if (bResult == false)
				{
					UpdateCells();
					DrawCell(pDC, pCell, rCell, bSelected, bToday, bShowMonth);
				}

				// focus rect
				if (m_SelectionRange[2] == tcur)
					DrawCellFocus(pDC, pCell, rCell);
			}
		}
	}

	pDC->SelectObject(pOldFont);
}

void CCalendarCtrl::DrawCellFocus(CDC* pDC, const CCalendarCell* /*pCell*/, const CRect& rCell)
{
	CRect rFocus(rCell);
	rFocus.DeflateRect(1, 1, 0, 0);
	
	pDC->DrawFocusRect(rFocus);
}

bool CCalendarCtrl::DrawCell(CDC* pDC, const CCalendarCell* pCell, const CRect& rCell, 
							 BOOL bSelected, BOOL bToday, BOOL bShowMonth)
{
	if ((m_bMonthIsOdd && !(pCell->date.GetMonth()%2)) || 
		(!m_bMonthIsOdd && (pCell->date.GetMonth()%2)))
	{
		pDC->FillSolidRect(rCell, CALENDAR_LIGHTGREY);
	}
	else
	{
		pDC->FillSolidRect(rCell, CALENDAR_WHITE);
	}

	bool bEquipmentHoliday = false;
	if (m_bEquipmentMode == true)
	{
		//AfxMessageBox(_T("m_bEquipmentMode"));
		int nEquipmentIndex = pCell->nEquipmentIndex;

 
		if (nEquipmentIndex >= 0 && nEquipmentIndex < m_Equipments.size())
		{
			vector<int> Holidays = m_Equipments[nEquipmentIndex].Holidays;

			int nCellDate = pCell->date.m_dt;
			for (int k = 0; k < Holidays.size(); k++)
			{
				if (nCellDate == Holidays[k])
				{
					bEquipmentHoliday = true;
					break;
				}
			}
		}
		else
		{
			return false;
		}
 

	}
	if (bEquipmentHoliday)
	{
		// Draw the frame 
		CRect rcLine(rCell);
		rcLine.bottom = rcLine.top + 15;
		rcLine.top = rcLine.bottom - 1;

		for (int c = 0; c<15; c++)
		{
			pDC->FillSolidRect(rcLine, RGB(255,220,220 ));
			rcLine.bottom--;
			rcLine.top = rcLine.bottom - 1;
		}

	}
	else if (bToday)
	{
		// Draw the frame 
		CRect rcLine(rCell);
		rcLine.bottom = rcLine.top+15;
		rcLine.top = rcLine.bottom-1;
		
		for(int c=0; c<15; c++)
		{
			pDC->FillSolidRect(rcLine, GetFadedThemeColour(c*6));
			rcLine.bottom--;
			rcLine.top = rcLine.bottom-1;
		}
	}
	
	// Draw the selection
	if (bSelected)
	{	
		CBrush br;

		br.CreateSolidBrush(GetFadedThemeColour(40));
		

		CRect selRect(rCell);

		if (bToday)	
			selRect.top += CALENDAR_DAY_HEADER_CY;

		pDC->FillRect(&selRect, &br);
	}
	
	// Out of range
	if ((pCell->date >= m_BoundUp) || 
		(pCell->date <= m_BoundDown))	
	{
		CRect selRect(rCell);
		CBrush br;

		br.CreateSolidBrush(RGB(255,225,225));
		pDC->FillRect(&selRect, &br);
	}
	
	if (pCell->bMark)
	{
		CBrush br;
		br.CreateSolidBrush(RGB(255,104,4));

		CRect rcMark(rCell);
		rcMark.DeflateRect(3,3);
		rcMark.right = rcMark.left +6;
		rcMark.bottom = rcMark.top +6;

		pDC->FillRect(&rcMark, &br);
	}
	
	// draw day/month numbers
	CString csDay;
	int nDay = pCell->date.GetDay();
	
	if (bShowMonth)
	{
		int nMonth = pCell->date.GetMonth();


		if ( m_bEquipmentMode && rCell.left == GetGridLeft() )
		{
			csDay.Format(_T("%d"), nDay);
		}
		else
		{
			csDay.Format(_T("%s %d"), CDateHelper::GetMonthName(nMonth, false), nDay);
  
			if (pDC->GetTextExtent(csDay).cx > rCell.Width())
			{
				csDay.Format(_T("%s %d"), CDateHelper::GetMonthName(nMonth, true), nDay);
				if (pDC->GetTextExtent(csDay).cx > rCell.Width())
				{
					csDay.Format(_T("%d"), nDay);
				}
			}
		}

	}
	else
		csDay.Format(_T("%d"), nDay);

 
	//如果是第一列,显示设备信息
	CString csEquipment;
	CString csEquipmentName;
	if ( rCell.left == GetGridLeft() && m_bEquipmentMode )
	{
		
		int nEquipmentIndex = pCell->nEquipmentIndex ; // pCell->nEquipmentIndex = ;
		if ( nEquipmentIndex>=0 && nEquipmentIndex<m_Equipments.size() )
		{
			csEquipment = m_Equipments[nEquipmentIndex].EquipmentID ;
			csEquipmentName +=	m_Equipments[nEquipmentIndex].EquipmentName;
		}
	}
	
	unsigned long nColor;
 
	
	//设置设备颜色

	if (bSelected && !bToday)
	{
		nColor = pDC->SetTextColor(RGB(255,0,255));
	}
	else
	{
		nColor = pDC->SetTextColor(RGB(0,0,255));
	}

	//如果是第一列,显示设备信息
	if ( rCell.left == GetGridLeft()  )
	{
		pDC->DrawText(csEquipment, (LPRECT)(LPCRECT)rCell, DT_LEFT|DT_TOP);
 
	
		LONG nTextWidth = pDC->GetTextExtent(csEquipmentName).cx;
		if (nTextWidth > rCell.Width())
		{
			CString csEquipmentName1, csEquipmentName2;
			int nAllLength = csEquipmentName.GetLength();
			int nIndex = nAllLength / 2;
			csEquipmentName1 = csEquipmentName.Left(nIndex);

			if (pDC->GetTextExtent(csEquipmentName1).cx > nTextWidth / 2)
			{
				while (pDC->GetTextExtent(csEquipmentName1).cx > nTextWidth / 2)
				{
					nIndex--;
					csEquipmentName1 = csEquipmentName.Left(nIndex);
				}
			}
			else if (pDC->GetTextExtent(csEquipmentName1).cx < nTextWidth / 2)
			{
				while (pDC->GetTextExtent(csEquipmentName1).cx < nTextWidth / 2)
				{
					nIndex++;
					csEquipmentName1 = csEquipmentName.Left(nIndex);
				}
			}
 
			csEquipmentName2 = csEquipmentName.Right(nAllLength - nIndex);

			CRect RectName1(rCell);
			CRect RectName2(rCell);
			RectName1.top = RectName1.bottom - 30;
			RectName1.bottom = RectName1.bottom - 15;
			RectName2.top = RectName2.bottom - 15;

			pDC->DrawText(csEquipmentName1, (LPRECT)(LPCRECT)&RectName1, DT_LEFT | DT_BOTTOM);
			pDC->DrawText(csEquipmentName2, (LPRECT)(LPCRECT)&RectName2, DT_LEFT | DT_BOTTOM);

		}
		else
		{
			CRect RectName(rCell);
			RectName.top = RectName.bottom - 15;

			pDC->DrawText(csEquipmentName, (LPRECT)(LPCRECT)&RectName, DT_LEFT | DT_BOTTOM);
		}
 
		
	}
 
 
	//设置日期颜色
	if (bSelected && !bToday)
	{
		pDC->SetTextColor(RGB(255,104,4));
 
	}
	else if (bShowMonth)
	{
		pDC->SetTextColor(RGB(0, 0, 255));
	}
	else
	{
		pDC->SetTextColor(RGB(0,0,0));
 
	}
	pDC->DrawText(csDay, (LPRECT)(LPCRECT)rCell, DT_RIGHT|DT_TOP);
 

	//还原颜色
	pDC->SetTextColor(nColor);

	// draw inside...
	CRect rContent(rCell);
	rContent.DeflateRect(1,CALENDAR_DAY_HEADER_CY, 0, 0);		
	
	//画任务
	DrawCellContent(pDC, pCell, rContent, bSelected, bToday);

	return true;
}

//画任务
void CCalendarCtrl::DrawCellContent(CDC* pDC, const CCalendarCell* pCell, const CRect& rCell, 
									BOOL /*bSelected*/, BOOL /*bToday*/)
{
	// Draw the cell content if possible
	if (rCell.Width() >= 15)
	{
		for (int j=0; j<pCell->csaLines.GetSize(); j++)
		{
			CRect txtRect(rCell);
			CRect dotRect(rCell);
			
			txtRect.left += 9; 
			txtRect.right-= 2;
			txtRect.top += (j+1)*CALENDAR_LINE_HEIGHT; 
			
			dotRect.top += (j+1)*CALENDAR_LINE_HEIGHT+(CALENDAR_LINE_HEIGHT/2-1); 
			dotRect.bottom = dotRect.top + 3;
			dotRect.left += 3;
			dotRect.right = dotRect.left +3;
			
			pCell->bPartial = false;
			
			if (!pCell->csaLines[j].IsEmpty() && txtRect.Height() > CALENDAR_LINE_HEIGHT)
			{
				pDC->SetTextColor(RGB(0,0,0));
				pDC->DrawText(pCell->csaLines[j], txtRect, DT_LEFT|DT_TOP);								
				CBrush br;
				br.CreateSolidBrush(RGB(125,175,255));
			 
				pDC->FillRect(&dotRect, &br);
			}
			else if (!pCell->csaLines[j].IsEmpty())
			{
				CPen dotPen(PS_SOLID, 1, RGB(170,170,170));
				pDC->SelectObject(&dotPen);
				
				// Draw a little arrow
				static int t[2][7] = {5,5,8,8,8,5,5,4,3,2,1,2,3,4};
				int n = 0;
				
				for(int r=7; r>0; r--)
				{
					pDC->MoveTo(rCell.right-9+r, rCell.bottom-t[0][n]);
					pDC->LineTo(rCell.right-9+r, rCell.bottom-t[1][n]);
					n++;
				}
				
				pCell->bPartial = true;
				break;
			}
		}
	}
}

// grid area = client area minus the left gutter
void CCalendarCtrl::GetGridRect(CRect& rc) const
{
	GetClientRect(&rc);

	if (m_nLeftGutter > 0 && rc.Width() > m_nLeftGutter)
		rc.left += m_nLeftGutter;
}

int CCalendarCtrl::GetGridLeft() const
{
	CRect rc;
	GetGridRect(rc);
	return rc.left;
}

bool CCalendarCtrl::GetCellRect(int nRow, int nCol, CRect& rect, BOOL bOmitHeader) const
{	
	int nMaxRows = GetVisibleWeeks();
	if ( m_bEquipmentMode == true )
	{
		nMaxRows = min( GetEffectiveEquipRows(), m_nEquipmentCount );
	}


	if (nRow >=0 && nRow<nMaxRows && 
		nCol>=0 && nCol<CALENDAR_NUM_COLUMNS)
	{
		CRect rc;
		GetGridRect(rc);

		int nHeight = GetRowHeight();

		G_nCellHeight = nHeight;

		float nWidth = 1.0 * rc.Width()/CALENDAR_NUM_COLUMNS;

		if ( m_bEquipmentMode )
		{
			if ( nWidth < CALENDAR_FIRST_COL_WIDTH )
			{
				nWidth = ( 1.0 * rc.Width() - CALENDAR_FIRST_COL_WIDTH ) / ( CALENDAR_NUM_COLUMNS - 1 );
			}
			
			if ( nWidth < CALENDAR_FIRST_COL_WIDTH)
			{
				rect.left = rc.left+(nCol-1)*nWidth+CALENDAR_FIRST_COL_WIDTH;
				rect.right = rect.left + nWidth;
				rect.top = CALENDAR_HEADER_HEIGHT + nRow*nHeight;
				rect.bottom = rect.top + nHeight;
				
			}
			else
			{
				rect.left = rc.left+nWidth*nCol;
				rect.right = rect.left + nWidth;
				rect.top = CALENDAR_HEADER_HEIGHT + nRow*nHeight;
				rect.bottom = rect.top + nHeight;
			}
		}

		else
		{
			rect.left = rc.left+nWidth*nCol;
			rect.right = rect.left + nWidth;
			rect.top = CALENDAR_HEADER_HEIGHT + nRow*nHeight;
			rect.bottom = rect.top + nHeight;
		}

		if (nCol == 0)
		{
			if ( m_bEquipmentMode && nWidth < CALENDAR_FIRST_COL_WIDTH)
			{
				rect.left = rc.left;
				rect.right = rect.left + CALENDAR_FIRST_COL_WIDTH;
			}
		}
		else if (nCol == (CALENDAR_NUM_COLUMNS-1))
		{
			// right-most column extends to client extent
			rect.right = rc.right;
		}


		if (bOmitHeader)
			rect.top += CALENDAR_DAY_HEADER_CY;

		return true;
	}
	return false;
}

CCalendarCell* CCalendarCtrl::GetCell(const CPoint& point)
{
	int nRow, nCol;

	if (GetGridCellFromPoint(point, nRow, nCol))
		return GetCell(nRow, nCol);

	return NULL;
}

const CCalendarCell* CCalendarCtrl::GetCell(const CPoint& point) const 
{
	int nRow, nCol;
	
	if (GetGridCellFromPoint(point, nRow, nCol))
		return GetCell(nRow, nCol);
	
	return NULL;
}

bool CCalendarCtrl::GetGridCellFromPoint(const CPoint& point, int &nRow, int &nCol) const
{
	if (point.y > CALENDAR_HEADER_HEIGHT) 
	{
		CRect rc;
		GetGridRect(rc);

		if (point.x < rc.left || point.x > rc.right || point.y < 0 || point.y > rc.bottom)
			return false;
		
		int nAllVisableRowCount = m_bEquipmentMode ? GetEffectiveEquipRows() : GetVisibleWeeks();
		int nHeight = GetRowHeight();
		float nWidth = 1.0 * rc.Width()/CALENDAR_NUM_COLUMNS;

		if ( nWidth < CALENDAR_FIRST_COL_WIDTH )
		{
			nWidth = ( 1.0 * rc.Width() - CALENDAR_FIRST_COL_WIDTH ) / ( CALENDAR_NUM_COLUMNS - 1 );
		}

		if (nHeight && nWidth)
		{
			nRow = (point.y-CALENDAR_HEADER_HEIGHT)/nHeight;

			if ( nWidth < CALENDAR_FIRST_COL_WIDTH )
			{
				if ( point.x < rc.left + CALENDAR_FIRST_COL_WIDTH )
				{
					nCol = 0;
				}
				else
				{
					nCol = 1 + (point.x - rc.left - CALENDAR_FIRST_COL_WIDTH) / nWidth;
				}
			}
			else
			{
				nCol = (point.x - rc.left)/nWidth;
			}
 
			// correct for last row and column
			nRow = min(nRow, nAllVisableRowCount-1);
			nCol = min(nCol, CALENDAR_NUM_COLUMNS-1);

			return true;
		}
	}
	return false;
}

const COleDateTime& CCalendarCtrl::GetMinDate() const 
{ 
	return m_dayCells[0][0].date; 
};

const COleDateTime& CCalendarCtrl::GetMaxDate() const 
{ 
	int nMaxRow = GetVisibleWeeks() -1 ;
	if ( m_bEquipmentMode == true)
	{
		nMaxRow = min( m_nEquipmentVisableRows, m_nEquipmentCount ) - 1 ;
	}
	return m_dayCells[nMaxRow][CALENDAR_NUM_COLUMNS-1].date; 
};

bool CCalendarCtrl::GetLastSelectedGridCell(int &nRow, int &nCol) const
{
	nRow = 0; nCol = 0;

	for(int i=0; i<CALENDAR_MAX_ROWS ; i++)
	{
		for(int u=0; u<CALENDAR_NUM_COLUMNS ; u++)
		{
			if (m_SelectionRange[1] == DateToSeconds(GetCell(i, u)->date))
			{
				nRow = i; 
				nCol = u;
				return true;
			}
		}
	}

	return false;
}

bool CCalendarCtrl::IsGridCellSelected(const CCalendarCell* pCell) const
{
	ASSERT(pCell);
	
	if (!pCell)
		return FALSE;

	time_t tDate = DateToSeconds(pCell->date);
	ASSERT(tDate);

	if (m_SingleSelection.GetCount())
	{
		bool bSel;
		return (m_SingleSelection.Lookup(tDate, bSel) && bSel);
	}

	// else selection range
	return (tDate >= m_SelectionRange[0] && tDate <= m_SelectionRange[1]);
}
	
bool CCalendarCtrl::IsGridCellSelected(int nRow, int nCol) const
{
	const CCalendarCell* pCell = GetCell(nRow, nCol);
	ASSERT(pCell);

	return IsGridCellSelected(pCell);
}

void CCalendarCtrl::OnRButtonDown(UINT nFlags, CPoint point) 
{
	int nRow, nCol;

	if (GetGridCellFromPoint(point, nRow, nCol))
	{
		// select if not already
		if (!IsGridCellSelected(nRow, nCol))
		{
			const CCalendarCell* pCell = GetCell(nRow, nCol);
			ASSERT(pCell);

			m_SingleSelection.RemoveAll();

			time_t tDate = DateToSeconds(pCell->date);

			m_SelectionRange[2] = 0;
			m_SelectionRange[0] = tDate;
			m_SelectionRange[1] = m_SelectionRange[0];

			Invalidate(FALSE);
			UpdateWindow();
		}
	}	
	
	SetFocus();
	CWnd::OnRButtonDown(nFlags, point);
}

void CCalendarCtrl::OnLButtonDown(UINT nFlags, CPoint point) 
{


	bool bShift = (GetKeyState(VK_SHIFT)>>8 == -1);
	bool bCtrl = (GetKeyState(VK_CONTROL)>>8 == -1);

	if (bShift || !m_bEnableMultiSel) 
		bCtrl = false; // allow one combination

	int nRow, nCol;

	if (GetGridCellFromPoint(point, nRow, nCol) && !bCtrl)
	{
		if ((m_dayCells[nRow][nCol].date < m_BoundUp) && 
			(m_dayCells[nRow][nCol].date > m_BoundDown))
		{
			m_SingleSelection.RemoveAll();

			m_SelectionRange[2] = 0;
			m_SelectionRange[0] = DateToSeconds(GetCell(nRow, nCol)->date);
			m_SelectionRange[1] = m_SelectionRange[0];

			if (m_bEnableMultiSel)
			{
				m_bSelectionStarted = true;
				SetCapture();
			}

			Invalidate(true);
		}
	}	
	
	SetFocus();
	CWnd::OnLButtonDown(nFlags, point);
}

void CCalendarCtrl::OnLButtonUp(UINT nFlags, CPoint point) 
{
	int nRow, nCol;
 

	bool bShift = (GetKeyState(VK_SHIFT)>>8 == -1);
	bool bCtrl = (GetKeyState(VK_CONTROL)>>8 == -1);

	if (bShift || !m_bEnableMultiSel) 
		bCtrl = false; // allow one combination


	if (GetGridCellFromPoint(point, nRow, nCol))
	{
		if (bCtrl)
		{
			if ((m_dayCells[nRow][nCol].date < m_BoundUp) && 
				(m_dayCells[nRow][nCol].date > m_BoundDown))	
			{
				if (m_SelectionRange[2])
				{				
					m_SelectionRange[1] = DateToSeconds(GetCell(nRow, nCol)->date);
					m_SelectionRange[0] = m_SelectionRange[1];
					
					bool bSelected;
					
					if (m_SingleSelection.Lookup(m_SelectionRange[1], bSelected))
					{
						m_SingleSelection.RemoveKey(m_SelectionRange[1]);
					}
					else if (m_SingleSelection.GetCount() < m_nMaxSel)
					{
						m_SingleSelection.SetAt(m_SelectionRange[1], true);
					}
					m_SelectionRange[2] = m_SelectionRange[1];
				}
				else
				{
					if (m_SingleSelection.GetCount() < m_nMaxSel)
						m_SingleSelection.SetAt(m_SelectionRange[1], true);
					
					m_SelectionRange[1] = DateToSeconds(GetCell(nRow, nCol)->date);
					m_SelectionRange[0] = m_SelectionRange[1];
					
					if (m_SingleSelection.GetCount() < m_nMaxSel)
						m_SingleSelection.SetAt(m_SelectionRange[1], true);
					
					m_SelectionRange[2] = m_SelectionRange[1];
				}			
			}
		}
		else if (m_bSelectionStarted)
		{
			ASSERT(m_bEnableMultiSel);

			m_SingleSelection.RemoveAll();
			m_bSelectionStarted = false;

			int nRow, nCol;

			if (GetGridCellFromPoint(point, nRow, nCol))
			{
				if ((m_dayCells[nRow][nCol].date < m_BoundUp) && 
					(m_dayCells[nRow][nCol].date > m_BoundDown))	
				{
					m_SelectionRange[1] = DateToSeconds(GetCell(nRow, nCol)->date);
				}
				else
					m_SelectionRange[1] = m_SelectionRange[0];
			}
			else
				m_SelectionRange[1] = m_SelectionRange[0];
			
			AdjustSelection();
		}
		else // single selection
		{
			if ((m_dayCells[nRow][nCol].date < m_BoundUp) && 
				(m_dayCells[nRow][nCol].date > m_BoundDown))
			{
				m_SingleSelection.RemoveAll();

				m_SelectionRange[2] = 0;
				m_SelectionRange[0] = DateToSeconds(GetCell(nRow, nCol)->date);
				m_SelectionRange[1] = m_SelectionRange[0];
			}
		}

		// always release capture
		ReleaseCapture();
		Invalidate(true);

		// notify parent
		NotifyParentClick();
	}
	else // remove selection
	{
		m_SingleSelection.RemoveAll();
		m_SelectionRange[2] = 0;

		Invalidate(true);
	}

	CWnd::OnLButtonUp(nFlags, point);
}

void CCalendarCtrl::OnMouseMove(UINT nFlags, CPoint point) 
{
	int nRow, nCol;
	if (m_bSelectionStarted)
	{
		if (GetGridCellFromPoint(point, nRow, nCol))
		{
			if ((m_dayCells[nRow][nCol].date < m_BoundUp) && 
				(m_dayCells[nRow][nCol].date > m_BoundDown))	
			{
				m_SingleSelection.RemoveAll();
 
				m_SelectionRange[1] = DateToSeconds(GetCell(nRow, nCol)->date);

				AdjustSelection();
				Invalidate(true);
			}
		}
	}	
 
	CWnd::OnMouseMove(nFlags, point);
}


void CCalendarCtrl::SetVScrollPos(UINT nPos)
{
 
	if (m_bEquipmentMode)
	{
		m_nVscrollPos = nPos;
		if (nPos <= 0)
		{
			m_nVscrollPos = 0;
		}
		int nVisRows = GetEffectiveEquipRows();
		if (nPos + nVisRows >= m_nVscrollMax)
		{
			m_nVscrollPos = m_nVscrollMax - nVisRows;
			SetScrollPos(SB_VERT, m_nVscrollPos, TRUE);
			return;
		}

		SetScrollPos(SB_VERT, m_nVscrollPos, TRUE);
	}

}



void CCalendarCtrl::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{ 
	
	int nInc = 0;
    switch (nSBCode)
    {
        case SB_TOP:
        case SB_LINEUP:
        case SB_PAGEUP:     
			nInc = -1;
			break;

		case SB_BOTTOM:
		case SB_LINEDOWN:
        case SB_PAGEDOWN:
			nInc = 1;
			break;

        case SB_THUMBTRACK: 
			nInc = nPos - m_nVscrollPos;
			break;

		case SB_ENDSCROLL:	
        default:
			nInc = 0;
    }

	if ( m_bEquipmentMode )
	{
		if (nInc<0 && m_nVscrollPos<=0)
		{
			m_nVscrollPos = 0;
			SetScrollPos(SB_VERT, m_nVscrollPos, TRUE);
			return;
		}
		int nVisRows = GetEffectiveEquipRows();
		if (nInc>0 && m_nVscrollPos + nVisRows >=m_nVscrollMax)
		{
			m_nVscrollPos =m_nVscrollMax - nVisRows ;
			SetScrollPos(SB_VERT, m_nVscrollPos, TRUE);
			return;
		}
	}


	if (nInc)
	{		
		m_nVscrollPos += nInc;	

		COleDateTime newDate = m_DateCurrent;
		if ( m_bEquipmentMode  == false )
		{	
			newDate += COleDateTimeSpan(7*nInc, 0, 0, 0); 
		}

		if ((m_BoundUp > newDate) && (newDate > m_BoundDown))
			Goto(newDate);
		else
			m_nVscrollPos -= nInc;

		SetScrollPos(SB_VERT, m_nVscrollPos, TRUE);
	}
	
	SetFocus();
	CWnd::OnVScroll(nSBCode, nPos, pScrollBar);
}



void CCalendarCtrl::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
 
	int nInc = 0;
    switch (nSBCode)
    {
	case SB_LEFT:
	case SB_LINELEFT:
	case SB_PAGELEFT:     
		nInc = -1;
		break;
		
	case SB_RIGHT:
	case SB_LINERIGHT:
	case SB_PAGERIGHT:
		nInc = 1;
		break;
		
	case SB_THUMBTRACK: 
		nInc = nPos - m_nHscrollPos;
		break;
		
	case SB_ENDSCROLL:	
	default:
		nInc = 0;
    }


	
	if ( m_bEquipmentMode )
	{
		if (nInc<0 && m_nHscrollPos<=0)
		{
			m_nHscrollPos = 0;
			SetScrollPos(SB_HORZ, m_nHscrollPos, TRUE);
			return;
		}
		if (nInc>0 && m_nHscrollPos>=m_nHscrollMax)
		{
			m_nHscrollPos =m_nHscrollMax;
			SetScrollPos(SB_HORZ, m_nHscrollPos, TRUE);
			return;
		}	
	}

	
	if (nInc)
	{		
		m_nHscrollPos += nInc;	
		
		COleDateTime newDate = m_DateCurrent;
		newDate += COleDateTimeSpan( nInc, 0, 0, 0); 
		
		if ((m_BoundUp > newDate) && (newDate > m_BoundDown))
			Goto(newDate);
		else
			m_nVscrollPos -= nInc;
		
		SetScrollPos(SB_HORZ, m_nHscrollPos, TRUE);
	}
	
	if (m_bEquipmentMode == false)
	{
		m_nHscrollPos = 0;
		SetScrollPos(SB_HORZ, m_nHscrollPos, TRUE);
	}

	
	SetFocus();
	CWnd::OnHScroll(nSBCode, nPos, pScrollBar);
}

#if _MSC_VER >= 1400
BOOL CCalendarCtrl::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
#else
void CCalendarCtrl::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
#endif
{	
	COleDateTime newDate = m_DateCurrent;

	if (zDelta < 0)
	{
		if (GetKeyState(VK_CONTROL) & 0x8000)
		{
			if( m_bEquipmentMode == false)
			{
				SetVisibleWeeks(min(m_nVisibleWeeks+1, CALENDAR_MAX_ROWS));
			}
			else
			{
				SetVisibleColumns(min(CALENDAR_NUM_COLUMNS+1, CALENDAR_NUM_COLUMNS_MAX));
			}
 			
			GetParent()->SendMessage(WM_CALENDAR_VISIBLEWEEKCHANGE);
		}
		else
			SendMessage(WM_VSCROLL,SB_LINEDOWN);
	}
	else if (zDelta > 0)
	{
		if (GetKeyState(VK_CONTROL) & 0x8000)
		{
			if( m_bEquipmentMode == false)
			{
				SetVisibleWeeks(max(m_nVisibleWeeks-1, 1));
			}
			else
			{
				SetVisibleColumns(max(CALENDAR_NUM_COLUMNS-1, 1));
			}
			GetParent()->SendMessage(WM_CALENDAR_VISIBLEWEEKCHANGE);
		}
		else
			SendMessage(WM_VSCROLL,SB_LINEUP);
	}
	
#if _MSC_VER >= 1400
	return CWnd::OnMouseWheel(nFlags, zDelta, pt);
#else
	CWnd::OnMouseWheel(nFlags, zDelta, pt);
#endif
}



CString CCalendarCtrl::GetMouseUpCellEquipment() const
{
	int nEquipmentIndex = 0;
	CString strEquipmentID = _T("");

	if (m_bEquipmentMode == false)
		return strEquipmentID;

	const CCalendarCell * pMouseMoveCell;

	int  nRow, nCol;
	if (GetGridCellFromPoint(m_MouseMovePoint, nRow, nCol))
	{
		pMouseMoveCell = GetCell(nRow, nCol);
	}
	else
	{
		return strEquipmentID;
	}


	if (pMouseMoveCell != NULL)
	{
		nEquipmentIndex = pMouseMoveCell->nEquipmentIndex;

		if (nEquipmentIndex < 0 || nEquipmentIndex >= m_Equipments.size())
		{
			strEquipmentID = _T("");
		}
		else
		{
			strEquipmentID = m_Equipments[nEquipmentIndex].EquipmentID;
		}
	}
	return strEquipmentID;
}


time_t CCalendarCtrl::DateToSeconds(const COleDateTime& date)
{
	// FIX : We need to find the gmt bias for the zone	
	time_t t = (time_t)(date - COleDateTime(1970, 1, 1, 0, 0, 0)).GetTotalSeconds();
	
	COleDateTime diffdate(t);
	int gmtbias = 0;
	
	if (diffdate > date) 
		gmtbias = -((int)(diffdate-date).GetTotalSeconds());
	else 
		gmtbias =  ((int)(date-diffdate).GetTotalSeconds());
	
	// The next check is to handle the case when system is adjusting the clock 
	// and we meet a time change
	// Note: for the missing hour case, bias is left untouched so the date may have a time > 0
	COleDateTime v((time_t)(t/86400*86400 + gmtbias));

	if (v < date) 
		gmtbias += 3600;
	
	return (time_t)(t/86400*86400 + gmtbias);
}

UINT CCalendarCtrl::OnGetDlgCode()
{
	return DLGC_WANTARROWS;
}

void CCalendarCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{		
	int nRow, nCol;
	GetLastSelectedGridCell(nRow, nCol);
	
	if ((m_dayCells[nRow][nCol].date >= m_BoundUp) || 
		(m_dayCells[nRow][nCol].date <= m_BoundDown))
	{
		return;
	}

	COleDateTime newDate = m_DateCurrent;
	
	bool bHold = (GetKeyState(VK_SHIFT)>>8 == -1)?true:false;
	bool bCtrl = (GetKeyState(VK_CONTROL)>>8 == -1)?true:false;
	
	if (bHold) 
		bCtrl = false; // allow one combination
	
	switch(nChar)
	{
	case VK_SPACE:
		if (bCtrl)
		{
			bool bSelected; 
			time_t t = DateToSeconds(GetCell(nRow, nCol)->date);
			
			if (m_SingleSelection.Lookup(t, bSelected))
			{
				m_SingleSelection.RemoveKey(t);
			}
			else if (m_SingleSelection.GetCount() < m_nMaxSel)
			{
				m_SingleSelection.SetAt(t, true);
			}
			
			NotifyParentClick();
			Invalidate(true);
		}
		break;
		
	case VK_UP:
		{
			if (!bCtrl)
			{
				m_SingleSelection.RemoveAll();
				m_SelectionRange[2] = 0;
			}
			else if (m_SingleSelection.GetCount() == 0)
			{
				m_SingleSelection.SetAt(m_SelectionRange[0], true);
			}
			
			if (nRow == 0)
			{
				newDate -= COleDateTimeSpan(7,0,0,0);
				
				if ((m_BoundUp > newDate) && (newDate > m_BoundDown))
				{
					m_nVscrollPos--;
					SetScrollPos(SB_VERT, m_nVscrollPos, TRUE);
					Goto(newDate);
				}
			}
			else
				nRow--;	
			
			if ((m_dayCells[nRow][nCol].date < m_BoundUp) && 
				(m_dayCells[nRow][nCol].date > m_BoundDown))	
			{
				m_SelectionRange[1] = DateToSeconds(GetCell(nRow, nCol)->date);
				
				if (bCtrl)
					m_SelectionRange[2] = m_SelectionRange[1];
				
				if (!bHold)
					m_SelectionRange[0] = m_SelectionRange[1];
				
				AdjustSelection();
				Invalidate(true);
			}
			
			NotifyParentClick();
		}
		break;
		
	case VK_DOWN:
		{
			if (!bCtrl)
			{
				m_SingleSelection.RemoveAll();
				m_SelectionRange[2] = 0;
			}
			else if (m_SingleSelection.GetCount() == 0)
			{
				m_SingleSelection.SetAt(m_SelectionRange[0], true);
			}
			
			if (nRow == GetVisibleWeeks()-1)
			{
				newDate += COleDateTimeSpan(7,0,0,0);
				
				if ((m_BoundUp > newDate) && (newDate > m_BoundDown))
				{
					m_nVscrollPos++;
					SetScrollPos(SB_VERT, m_nVscrollPos, TRUE);
					Goto(newDate);
				}
			}
			else
				nRow++;
			
			if ((m_dayCells[nRow][nCol].date < m_BoundUp) && 
				(m_dayCells[nRow][nCol].date > m_BoundDown))	
			{	
				m_SelectionRange[1] = DateToSeconds(GetCell(nRow, nCol)->date);
				
				if (bCtrl)
					m_SelectionRange[2] = m_SelectionRange[1];
				
				if (!bHold)
					m_SelectionRange[0] = m_SelectionRange[1];
				
				AdjustSelection();
				Invalidate(true);
			}
			
			NotifyParentClick();
		}
		break;
		
	case VK_LEFT:
		{
			if (!bCtrl)
			{
				m_SingleSelection.RemoveAll();
				m_SelectionRange[2] = 0;
			}
			else if (m_SingleSelection.GetCount() == 0)
			{
				m_SingleSelection.SetAt(m_SelectionRange[0], true);
			}
			
			if (nCol > 0) 
			{
				nCol--;
			}
			else
			{
				nCol = CALENDAR_NUM_COLUMNS-1;
				
				if (nRow == 0)
				{
					newDate -= COleDateTimeSpan(7,0,0,0);
					
					if ((m_BoundUp > newDate) && (newDate > m_BoundDown))
					{
						m_nVscrollPos--;
						SetScrollPos(SB_VERT, m_nVscrollPos, TRUE);
						Goto(newDate);
					}
					else
						nCol = 0;
				}
				else
					nRow--;
			}
			
			if ((m_dayCells[nRow][nCol].date < m_BoundUp) && 
				(m_dayCells[nRow][nCol].date > m_BoundDown))	
			{					
				m_SelectionRange[1] = DateToSeconds(GetCell(nRow, nCol)->date);
				
				if (bCtrl)
					m_SelectionRange[2] = m_SelectionRange[1];
				
				if (!bHold)
					m_SelectionRange[0] = m_SelectionRange[1];
				
				AdjustSelection();
				Invalidate(true);
			}
			
			NotifyParentClick();
		}
		break;
		
	case VK_RIGHT:
		{
			if (!bCtrl)
			{
				m_SingleSelection.RemoveAll();
				m_SelectionRange[2] = 0;
			}
			else if (m_SingleSelection.GetCount() == 0)
			{
				m_SingleSelection.SetAt(m_SelectionRange[0], true);
			}
			
			if (nCol < CALENDAR_NUM_COLUMNS-1)	
			{
				nCol++;
			}
			else
			{
				nCol = 0;
				if (nRow == GetVisibleWeeks()-1)
				{
					newDate += COleDateTimeSpan(7,0,0,0);
					
					if ((m_BoundUp > newDate) && (newDate > m_BoundDown))
					{
						m_nVscrollPos++;
						SetScrollPos(SB_VERT, m_nVscrollPos, TRUE);
						Goto(newDate);
					}
					else
						nCol = CALENDAR_NUM_COLUMNS-1;
				}
				else
					nRow++;
			}
			
			if ((m_dayCells[nRow][nCol].date < m_BoundUp) && 
				(m_dayCells[nRow][nCol].date > m_BoundDown))	
			{
				m_SelectionRange[1] = DateToSeconds(GetCell(nRow, nCol)->date);
				
				if (bCtrl)
					m_SelectionRange[2] = m_SelectionRange[1];
				
				if (!bHold)
					m_SelectionRange[0] = m_SelectionRange[1];
				
				AdjustSelection();
				Invalidate(true);
			}
			
			NotifyParentClick();
		}
		break;
	}

	CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}

time_t CCalendarCtrl::GetFirstSelectedItem() const
{
	if (m_SingleSelection.GetCount())
	{
		POSITION pos = m_SingleSelection.GetStartPosition();
		ASSERT(pos);

		bool bSelected;
		time_t date;

		m_SingleSelection.GetNextAssoc(pos, date, bSelected);
		ASSERT(bSelected);

		return date;
	}
	else if (m_SelectionRange[0] && m_SelectionRange[1])
	{
		return min(m_SelectionRange[0], m_SelectionRange[1]);
	}

	// all else
	return 0;
}


int CCalendarCtrl::GetSelectedItems(CDWordArray& dwaSelection) const
{
	if (m_SingleSelection.GetCount())
	{
		POSITION pos = m_SingleSelection.GetStartPosition();
		bool bSelected;
		time_t date;

		while (pos)
		{
			m_SingleSelection.GetNextAssoc(pos, date, bSelected);
			ASSERT(bSelected);

			dwaSelection.Add((DWORD)date);
		}
	}
	else
	{
		if (m_SelectionRange[0] && m_SelectionRange[1])
		{
			COleDateTime dmax(max(m_SelectionRange[0], m_SelectionRange[1]));
			COleDateTime dmin(min(m_SelectionRange[0], m_SelectionRange[1]));			
			COleDateTime dcur = dmin;

			// FIX : Remove added hour if any
			if (dmin.GetHour())	
				dmin -= COleDateTimeSpan(0,1,0,0);

			int nDays = 1;

			while(dcur <= dmax)
			{	
				dwaSelection.Add(DateToSeconds(dcur));
				dcur = dmin + COleDateTimeSpan(nDays,0,0,0);				
				dcur = COleDateTime(DateToSeconds(dcur));

				nDays++;
			}
		}
	}

	return dwaSelection.GetSize();
}

bool CCalendarCtrl::GetGridCellFromDate(const COleDateTime& date, int &nRow, int &nCol) const
{
	for(int i=0; i < GetVisibleWeeks() ; i++)
	{
		for(int u=0; u<CALENDAR_NUM_COLUMNS; u++)
		{
			const CCalendarCell* pCell = GetCell(i, u);

			if (floor(pCell->date.m_dt) == floor(date.m_dt))
			{
				nRow = i;
				nCol = u;

				return true;
			}
		}
	}

	return false;
}

CCalendarCell* CCalendarCtrl::GetCell(const COleDateTime& date)
{
	int nRow, nCol;

	if (GetGridCellFromDate(date, nRow, nCol))
		return GetCell(nRow, nCol);

	// else
	return NULL;
}

void CCalendarCtrl::AdjustSelection()
{
	int nSelCount = 0;

	COleDateTime dmax(max(m_SelectionRange[0], m_SelectionRange[1]));
	COleDateTime dmin(min(m_SelectionRange[0], m_SelectionRange[1]));
	COleDateTime dcur = dmin;

	while(dcur <= dmax)
	{
		nSelCount++;
		dcur += COleDateTimeSpan(1,0,0,0);			
	}

	if (nSelCount > m_nMaxSel)
	{
		if (m_SelectionRange[1] > m_SelectionRange[0])
		{
			COleDateTime dMin((time_t)m_SelectionRange[1]);
			dMin -= COleDateTimeSpan(m_nMaxSel-1,0,0,0);
			m_SelectionRange[0] = DateToSeconds(dMin);
		}
		else if (m_SelectionRange[1] < m_SelectionRange[0])
		{
			COleDateTime dMax((time_t)m_SelectionRange[1]);
			dMax += COleDateTimeSpan(m_nMaxSel-1,0,0,0);
			m_SelectionRange[0] = DateToSeconds(dMax);
		}
	}
}

COLORREF CCalendarCtrl::GetFadedThemeColour(int percent)
{	
	unsigned char r = GetRValue(m_crTheme), g = GetGValue(m_crTheme), b = GetBValue(m_crTheme);
	int al = percent*75/100;

	return RGB(	(unsigned char)(r + ((255-r)/(float)75) * al), 
				(unsigned char)(g + ((255-g)/(float)75) * al), 
				(unsigned char)(b + ((255-b)/(float)75) * al));
}

void CCalendarCtrl::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	int nRow, nCol;

	if (GetGridCellFromPoint(point, nRow, nCol))
	{
		CCalendarCell* pCell = GetCell(nRow, nCol);
		ASSERT(pCell);

		if (pCell)
			NotifyParentClick();
	}

	CWnd::OnLButtonDblClk(nFlags, point);
}

void CCalendarCtrl::NotifyParentClick()
{
	NMHDR	NotifyArea;

	NotifyArea.code = NM_CLICK;
	NotifyArea.hwndFrom = m_hWnd;
	NotifyArea.idFrom = ::GetDlgCtrlID(m_hWnd);

	GetParent()->SendMessage(WM_NOTIFY, ::GetDlgCtrlID(m_hWnd), (WPARAM)&NotifyArea);
}

void CCalendarCtrl::NotifyParentDblClick()
{
	NMHDR	NotifyArea;

	NotifyArea.code = NM_DBLCLK;
	NotifyArea.hwndFrom = m_hWnd;
	NotifyArea.idFrom = ::GetDlgCtrlID(m_hWnd);

	GetParent()->SendMessage(WM_NOTIFY, ::GetDlgCtrlID(m_hWnd), (WPARAM)&NotifyArea);
}

//设备模式下，获取设备ID信息
CString CCalendarCtrl::GetEquipmentID(int nIndex) const
{
	if (nIndex < 0 || nIndex >= m_Equipments.size())
	{
		return _T("");
	}

	return m_Equipments[nIndex].EquipmentID ;
}


//所有设备
void CCalendarCtrl::SetEquipments(CString & sEquipments)
{
	m_bEquipmentMode = true;
	m_Equipments.resize(0);
	int nPos = 0;
	int nNextPos = 0;
	CString strID, strName, strHoliday;
	CString strSplit = _T(",");
	vector<CString> strs;
	Equipment OneEquipment;
	nPos = sEquipments.Find('\n', nPos);
	while (nPos >= 0)
	{
		nNextPos = sEquipments.Find('\n', nPos+1);
		if (nNextPos > nPos)
			strID = sEquipments.Mid(nPos + 1, nNextPos - nPos - 1);
		else
			goto lineOut;
	
			nPos = sEquipments.Find('\n', nNextPos+1);
			if (nPos < 0)
			{
				nPos = sEquipments.GetLength() - 1;
			}

		if (nPos > nNextPos)
			strName = sEquipments.Mid(nNextPos + 1,  nPos - nNextPos - 1);
		else
			goto lineOut;


		nNextPos = sEquipments.Find('\n', nPos + 1);
		if (nNextPos < 0)
		{
			nNextPos = sEquipments.GetLength() - 1;
		}

		if (nNextPos > nPos )
			strHoliday = sEquipments.Mid(nPos + 1, nNextPos - nPos - 1);
		else
		{
			strHoliday = "";
		}
 
		SplitString(strHoliday, strSplit, strs);
		int nHolidaysCount = strs.size();
		

		OneEquipment.EquipmentID = strID;
		OneEquipment.EquipmentName = strName;
		
		OneEquipment.Holidays.resize(0);
		for (int i = 0; i < nHolidaysCount; i++ )
		{
			strHoliday = strs[i];
			strHoliday.Trim();
			if (strHoliday.GetLength() > 0)
			{
				int nHoliday = _wtoi(strHoliday);
				OneEquipment.Holidays.push_back(nHoliday);
			}
		}
 
		m_Equipments.push_back(OneEquipment);

		if (nNextPos <= nPos) goto lineOut;
		nPos = nNextPos;
	}

lineOut:

	m_nEquipmentCount = m_Equipments.size();
	m_DragingEquipments.resize(0);

	m_nVscrollMax = m_nEquipmentCount;
	m_nVscrollPos = 0;
 
	m_nHscrollMax = m_nEndDay - m_nBeginDay + 1;
	m_nHscrollPos = 0;

	if (m_nEquipmentCount > 0)
	{
		SetEquipmentMode(true);
	}
	else
	{
		SetEquipmentMode(false);
	}
	Reset();
 
}

//所有设备
void CCalendarCtrl::SetDragingEquipments(CString & sDragingEquipments)
{
	m_DragingEquipments.resize(0);

	if (m_bEquipmentMode == false) return;

	int nPos = 0;
	int nNextPos = 0;
	CString strID;
 
	nPos = sDragingEquipments.Find('\n', nPos);
	while (nPos >= 0)
	{
		nNextPos = sDragingEquipments.Find('\n', nPos+1);
		
		if (nNextPos < 0)
		{
			nNextPos = sDragingEquipments.GetLength() - 1;
			strID = sDragingEquipments.Mid(nPos + 1, nNextPos - nPos - 1);
			strID.Trim();
			if (strID.GetLength()>0) m_DragingEquipments.push_back(strID);
			return;
		}

		if (nNextPos > nPos)
		{
			strID = sDragingEquipments.Mid(nPos + 1, nNextPos - nPos - 1);
			strID.Trim();
			m_DragingEquipments.push_back(strID);
		}
 
		nPos = nNextPos;
	}
}

int CCalendarCtrl::GetEquipmentIndex(CString & strEquipment)
{
	for (int i=0; i<m_nEquipmentCount; i++)
	{
		if (strEquipment.CompareNoCase( m_Equipments[i].EquipmentID) == 0)
		{
			return i;
		}
	}
	return -1;
}

//所有设备
void CCalendarCtrl::SetEquipments(vector<Equipment>& Equipments)
{
	m_Equipments.resize(0);
	m_Equipments = Equipments;

	m_nEquipmentCount = m_Equipments.size();
	if (m_nEquipmentCount > 0)
	{
		SetEquipmentMode(true);
	}
	else
	{
		SetEquipmentMode(false);
	}
}
//拖拽时，显示的工序设备
void CCalendarCtrl::SetDragingEquipments(vector<CString>& DragingEquipments)
{
	m_DragingEquipments.resize(0);
	m_DragingEquipments = DragingEquipments;
}
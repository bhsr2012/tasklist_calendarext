// CalendarFrameWnd.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "CalendarExt.h"
#include "CalendarWnd.h"

#include "..\ToDoList\tdcmsg.h"
#include "..\ToDoList\tdcenum.h"

#include "..\Shared\DialogHelper.h"
#include "..\Shared\DateHelper.h"
#include "..\Shared\TimeHelper.h"
#include "..\Shared\FileMisc.h"
#include "..\Shared\UITheme.h"
#include "..\Shared\themed.h"
#include "..\Shared\dlgunits.h"
#include "..\shared\misc.h"
#include "..\shared\graphicsmisc.h"
#include "..\shared\localizer.h"

#include "..\Interfaces\IPreferences.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// equipment rows offered on the calendar toolbar
static const int s_nEquipRowPresets[] = { 1, 2, 3, 4, 5 };
static const int s_nEquipRowPresetCount = _countof(s_nEquipRowPresets);

#define CAL_CMB_EQUIP_FIRST		9		// first "N rows" entry in the combo
#define CAL_CMB_BACK_TO_CAL		(CAL_CMB_EQUIP_FIRST + s_nEquipRowPresetCount)

// filter bar collapse geometry, in pixels
const int COLLAPSE_BTN_W = 20;
const int COLLAPSE_BTN_H = 16;
const int COLLAPSE_BAR_H = 18;

static int EquipRowPresetIndex(int nRows)
{
	for (int i = 0; i < s_nEquipRowPresetCount; i++)
	{
		if (s_nEquipRowPresets[i] >= nRows)
			return i;
	}

	return s_nEquipRowPresetCount - 1;
}

/////////////////////////////////////////////////////////////////////////////

const UINT IDC_BIG_CALENDAR = 101;
const UINT IDC_MINI_CALENDAR = 102;

/////////////////////////////////////////////////////////////////////////////

const COLORREF DEF_DONECOLOR		= RGB(128, 128, 128);

/////////////////////////////////////////////////////////////////////////////
// CCalendarWnd

IMPLEMENT_DYNAMIC(CCalendarWnd, CDialog)

CCalendarWnd::CCalendarWnd()
	:	
	m_hIcon(AfxGetApp()->LoadIcon(IDR_CALENDAR)),
	m_bReadOnly(FALSE),
	m_nFilterLabelCount(0),
	m_nFilterHeight(0),
	m_bFilterCollapsed(FALSE)
{
	
}


CCalendarWnd::~CCalendarWnd()
{
}

 
//所有设备
void CCalendarWnd::SetEquipments(CString& sEquipments)
{
	m_BigCalendar.SetEquipments(sEquipments);
}


//拖拽时，显示的工序设备
void CCalendarWnd::SetDragingEquipments(CString & sDragingEquipments)
{
	m_BigCalendar.SetDragingEquipments(sDragingEquipments);
}

void CCalendarWnd::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_NUMWEEKS, m_cbNumWeeks);
	DDX_Control(pDX, IDC_SNAPMODES, m_cbSnapModes);
	DDX_Text(pDX, IDC_SELECTEDTASKDATES, m_sSelectedTaskDates);
}

BEGIN_MESSAGE_MAP(CCalendarWnd, CDialog)
	//{{AFX_MSG_MAP(CCalendarWnd)
	ON_WM_SETFOCUS()
	ON_WM_CTLCOLOR()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_BN_CLICKED(IDC_GOTOTODAY, OnGototoday)
	ON_BN_CLICKED(IDC_PREFERENCES, OnPreferences)
	ON_BN_CLICKED(IDC_COLLAPSEFILTER, OnCollapseFilter)
	//}}AFX_MSG_MAP
	ON_WM_COPYDATA()
	ON_CBN_SELCHANGE(IDC_NUMWEEKS, OnSelChangeNumWeeks)
	ON_CBN_SELCHANGE(IDC_SNAPMODES, OnSelChangeSnapMode)
	ON_NOTIFY(NM_CLICK, IDC_MINI_CALENDAR, OnMiniCalendarNotifyClick)
	ON_NOTIFY(NM_CLICK, IDC_BIG_CALENDAR, OnBigCalendarNotifyClick)
	ON_NOTIFY(NM_DBLCLK, IDC_MINI_CALENDAR, OnMiniCalendarNotifyDblClk)
	ON_NOTIFY(NM_DBLCLK, IDC_BIG_CALENDAR, OnBigCalendarNotifyDblClk)
	ON_REGISTERED_MESSAGE(WM_CALENDAR_DATECHANGE, OnBigCalendarNotifyDateChange)
	ON_REGISTERED_MESSAGE(WM_CALENDAR_SELCHANGE, OnBigCalendarNotifySelectionChange)
	ON_REGISTERED_MESSAGE(WM_CALENDAR_DRAGCHANGE, OnBigCalendarNotifyDragChange)
	ON_REGISTERED_MESSAGE(WM_CALENDAR_VISIBLEWEEKCHANGE, OnBigCalendarNotifyVisibleWeekChange)

END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCalendarWnd message handlers

void CCalendarWnd::SetReadOnly(bool bReadOnly)
{
	m_bReadOnly = bReadOnly;

	m_BigCalendar.SetReadOnly(bReadOnly);
}

BOOL CCalendarWnd::Create(DWORD dwStyle, const RECT &/*rect*/, CWnd* pParentWnd, UINT nID)
{
	if (CDialog::Create(IDD_CALENDAR_DIALOG, pParentWnd))
	{
		SetWindowLong(*this, GWL_STYLE, dwStyle);
		SetDlgCtrlID(nID);

		return TRUE;
	}

	return FALSE;
}


static CWnd * G_pToDoListWnd = NULL;
BOOL CCalendarWnd::OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct)
{
	G_pToDoListWnd = pWnd;
	CString str = CString((LPCTSTR)pCopyDataStruct->lpData, pCopyDataStruct->cbData / sizeof(TCHAR));

	//全部设备
	if (pCopyDataStruct->dwData == 3)
	{
		m_BigCalendar.SetEquipmentMode(true);
		m_BigCalendar.SetEquipments(str);

		COleDateTime Date = COleDateTime::GetCurrentTime();
		Date.m_dt = Date.m_dt - 1;
		m_MiniCalendar.SetDate(Date);
		m_BigCalendar.Goto(Date);

		InitCmb();
	}
	//拖拽的设备
	else if (pCopyDataStruct->dwData == 4)
	{
		m_BigCalendar.SetDragingEquipments(str);
	}
	//刷新任务的消息，收到该消息，日历组件刷新所有任务信息
	else if (pCopyDataStruct->dwData == 5)
	{
		OnGototoday();
	}
 
	return  CDialog::OnCopyData(pWnd, pCopyDataStruct);
}



BOOL CCalendarWnd::OnInitDialog() 
{
	CDialog::OnInitDialog();
 
	// non-translatables
	CLocalizer::EnableTranslation(*GetDlgItem(IDC_SELECTEDTASKDATES), FALSE);

	// create mini-calendar ctrl
	m_MiniCalendar.Create(NULL, NULL, WS_CHILD | WS_VISIBLE | FMC_NO3DBORDER, 
                          CRect(0,0,0,0), this, IDC_MINI_CALENDAR);

    m_MiniCalendar.SetDate(COleDateTime::GetCurrentTime());
    m_MiniCalendar.SetRowsAndColumns(3, 1);
	m_MiniCalendar.SetSpecialDaysCallback(CCalendarWnd::IsMiniCalSpecialDateCallback, (DWORD_PTR)this);

	// create big-calendar ctrl
	m_BigCalendar.Create(WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN, CRect(0,0,0,0), this, IDC_BIG_CALENDAR);
	m_BigCalendar.EnableMultiSelection(FALSE);
	m_BigCalendar.SetFirstWeekDay(CDateHelper::GetFirstDayOfWeek());


	// build snap combobox
	InitSnapCombo();

	InitCmb();

	InitFilterBar();
	ApplyFilterBarState();

	return 0;
}



void CCalendarWnd::InitCmb()
{
	while (m_cbNumWeeks.GetCount())
	{
		m_cbNumWeeks.DeleteString(0);
	}

	m_cbNumWeeks.AddString(_T("1 Week"));
	m_cbNumWeeks.AddString(_T("2 Weeks"));
	m_cbNumWeeks.AddString(_T("3 Weeks"));
	m_cbNumWeeks.AddString(_T("4 Weeks"));
	m_cbNumWeeks.AddString(_T("5 Weeks"));
	m_cbNumWeeks.AddString(_T("6 Weeks"));
	m_cbNumWeeks.AddString(_T("7 Weeks"));
	m_cbNumWeeks.AddString(_T("8 Weeks"));
	m_cbNumWeeks.AddString(_T("9 Weeks"));
	
 
	if (m_BigCalendar.m_bEquipmentMode == false)
	{
		m_cbNumWeeks.AddString(_T("切换单双周"));
		m_cbNumWeeks.SetCurSel(m_BigCalendar.GetVisibleWeeks() - 1);
	}
	else
	{
		CString sRows;
		for (int i = 0; i < s_nEquipRowPresetCount; i++)
		{
			sRows.Format(_T("%d 行"), s_nEquipRowPresets[i]);
			m_cbNumWeeks.AddString(sRows);
		}
		m_cbNumWeeks.SetCurSel(CAL_CMB_EQUIP_FIRST + EquipRowPresetIndex(m_BigCalendar.GetVisibleEquipments()));
	}
	m_cbNumWeeks.AddString(_T("切换日历设备模式"));
}

void CCalendarWnd::InitSnapCombo()
{
	CDialogHelper::AddString(m_cbSnapModes, IDS_SNAP_NEARESTHOUR,	TCCSM_NEARESTHOUR);
	CDialogHelper::AddString(m_cbSnapModes, IDS_SNAP_NEARESTDAY,	TCCSM_NEARESTDAY);
	CDialogHelper::AddString(m_cbSnapModes, IDS_SNAP_NEARESTHALFDAY, TCCSM_NEARESTHALFDAY);
	CDialogHelper::AddString(m_cbSnapModes, IDS_SNAP_FREE,			TCCSM_FREE);

	CDialogHelper::SelectItemByData(m_cbSnapModes, m_BigCalendar.GetSnapMode());
}

void CCalendarWnd::OnSelChangeNumWeeks()
{
	if  ( m_BigCalendar.m_bEquipmentMode == false )
	{
		int nIndex = m_cbNumWeeks.GetCurSel() ;
		
		//切换日历设备模式
		if (nIndex == 10)
		{
			m_BigCalendar.SetEquipmentMode(true ) ;
			InitCmb();
		}
		//切换单双周
		else if ( nIndex==9)
		{
			int nColumnCount = m_BigCalendar.GetVisibleColumns();
			if ( nColumnCount == 7 )
			{
				m_BigCalendar.SetVisibleColumns(14);
			}
			else
			{
				m_BigCalendar.SetVisibleColumns(7);
			}

		}
		else
		{
			m_BigCalendar.SetVisibleWeeks(m_cbNumWeeks.GetCurSel() + 1);
		}
	}
	else
	{
		int nIndex = m_cbNumWeeks.GetCurSel() ;

		//切换日历设备模式
		if (nIndex == CAL_CMB_BACK_TO_CAL)
		{
			m_BigCalendar.SetEquipmentMode(false);
			InitCmb();
		}
		//切换设备数
		else if ( nIndex>=CAL_CMB_EQUIP_FIRST && nIndex<CAL_CMB_BACK_TO_CAL)
		{
			int nRowCount = s_nEquipRowPresets[nIndex - CAL_CMB_EQUIP_FIRST];
			 
			m_BigCalendar.SetVisibleEquipments( nRowCount);
		}
		else
		{
			m_BigCalendar.SetVisibleColumns( (m_cbNumWeeks.GetCurSel() + 1) * 7);
		}
	}
	
}

void CCalendarWnd::OnSelChangeSnapMode()
{
	m_BigCalendar.SetSnapMode((TCC_SNAPMODE)CDialogHelper::GetSelectedItemData(m_cbSnapModes));
}

void CCalendarWnd::SavePreferences(IPreferences* pPrefs, LPCTSTR szKey) const 
{
	m_dlgPrefs.SavePreferences(pPrefs, szKey);
	pPrefs->WriteProfileInt(_T("CalendarSetting"), _T("NumEquipments"), m_BigCalendar.GetVisibleEquipments());
	pPrefs->WriteProfileInt(_T("CalendarSetting"), _T("NumWeeks"), m_BigCalendar.GetVisibleWeeks());
	pPrefs->WriteProfileInt(_T("CalendarSetting"), _T("NumColumns"), m_BigCalendar.GetVisibleColumns());
	pPrefs->WriteProfileInt(_T("CalendarSetting"), _T("EquipmentMode"),(int) m_BigCalendar.m_bEquipmentMode );
	pPrefs->WriteProfileInt(_T("CalendarSetting"), _T("CollapseFilter"), m_bFilterCollapsed);
 
}

void CCalendarWnd::LoadPreferences(const IPreferences* pPrefs, LPCTSTR szKey, BOOL bAppOnly) 
{
	// app preferences
	m_BigCalendar.SetOption(TCCO_TASKTEXTCOLORISBKGND, pPrefs->GetProfileInt(_T("Preferences"), _T("ColorTaskBackground"), FALSE));
	
	// Completion color and strikethru
	BOOL bStrikeThru = pPrefs->GetProfileInt(_T("Preferences"), _T("StrikethroughDone"), TRUE);
	COLORREF crDone = CLR_NONE;

	if (pPrefs->GetProfileInt(_T("Preferences"), _T("SpecifyDoneColor"), TRUE))
		crDone = pPrefs->GetProfileInt(_T("Preferences\\Colors"), _T("TaskDone"), DEF_DONECOLOR);

	m_BigCalendar.SetDoneTaskAttributes(crDone, bStrikeThru);

	// calendar specific preferences
	if (!bAppOnly)
	{
		m_dlgPrefs.LoadPreferences(pPrefs, szKey);
		UpdateCalendarCtrlPreferences();

		int nWeeks = pPrefs->GetProfileInt(_T("CalendarSetting"), _T("NumWeeks"), 6);
		int nColumns = pPrefs->GetProfileInt(_T("CalendarSetting"), _T("NumColumns"), 14);
		int nNumEquipments = pPrefs->GetProfileInt(_T("CalendarSetting"), _T("NumEquipments"), 5);
		m_BigCalendar.m_bEquipmentMode= (bool) pPrefs->GetProfileInt(_T("CalendarSetting"), _T("EquipmentMode"), 1);

		if (m_BigCalendar.m_bEquipmentMode == false)
		{
 			if (m_BigCalendar.SetVisibleWeeks(nWeeks))
				m_cbNumWeeks.SetCurSel(nWeeks-1);

		}
		else
		{
			m_BigCalendar.SetVisibleWeeks(nWeeks);

			if (m_BigCalendar.SetVisibleColumns(nColumns))
			{
				int nWeeks = (int)(0.5 + m_BigCalendar.GetVisibleColumns() /7.0 );
				if (nWeeks<1) nWeeks=1;
				m_cbNumWeeks.SetCurSel(nWeeks - 1);  
			}
			m_BigCalendar.SetVisibleEquipments(s_nEquipRowPresets[EquipRowPresetIndex(nNumEquipments)]);
			m_cbNumWeeks.SetCurSel(CAL_CMB_EQUIP_FIRST + EquipRowPresetIndex(m_BigCalendar.GetVisibleEquipments()));
		}
 
		// filter bar collapsed state
		m_bFilterCollapsed = pPrefs->GetProfileInt(_T("CalendarSetting"), _T("CollapseFilter"), FALSE);
		ApplyFilterBarState();

		//make sure 'today' is visible
		COleDateTime dtToday = COleDateTime::GetCurrentTime();
		m_MiniCalendar.SetDate(dtToday);
		m_BigCalendar.GotoToday();
	}
}

void CCalendarWnd::UpdateCalendarCtrlPreferences()
{
	m_BigCalendar.SetOption(TCCO_CALCMISSINGSTARTASCREATION, m_dlgPrefs.GetCalcMissingStartAsCreation());
	m_BigCalendar.SetOption(TCCO_CALCMISSINGSTARTASDUE, m_dlgPrefs.GetCalcMissingStartAsDue());
	m_BigCalendar.SetOption(TCCO_CALCMISSINGSTARTASEARLIESTDUEANDTODAY, m_dlgPrefs.GetCalcMissingStartAsEarliestDueAndToday());
	m_BigCalendar.SetOption(TCCO_CALCMISSINGDUEASSTART, m_dlgPrefs.GetCalcMissingDueAsStart());
	m_BigCalendar.SetOption(TCCO_CALCMISSINGDUEASLATESTSTARTANDTODAY, m_dlgPrefs.GetCalcMissingDueAsLatestStartAndToday());
	m_BigCalendar.SetOption(TCCO_DISPLAYCONTINUOUS, m_dlgPrefs.GetDisplayAsContinuous());
	m_BigCalendar.SetOption(TCCO_DISPLAYSTART, m_dlgPrefs.GetDisplayStart());
	m_BigCalendar.SetOption(TCCO_DISPLAYDUE, m_dlgPrefs.GetDisplayDue());
	m_BigCalendar.SetOption(TCCO_DISPLAYDONE, m_dlgPrefs.GetDisplayDone());
	m_BigCalendar.SetOption(TCCO_DISPLAYCALCSTART, m_dlgPrefs.GetDisplayCalcStart());
	m_BigCalendar.SetOption(TCCO_DISPLAYCALCDUE, m_dlgPrefs.GetDisplayCalcDue());
	m_BigCalendar.SetOption(TCCO_ADJUSTTASKHEIGHTS, m_dlgPrefs.GetAdjustTaskHeights());
}

void CCalendarWnd::SetUITheme(const UITHEME* pTheme)
{
	GraphicsMisc::VerifyDeleteObject(m_brBack);

	if (CThemed::IsAppThemed() && pTheme)
	{
		m_theme = *pTheme;
		m_brBack.CreateSolidBrush(pTheme->crAppBackLight);

		m_BigCalendar.SetThemeColour(pTheme->crAppBackDark);
		m_MiniCalendar.SetHighlightToday(TRUE, pTheme->crAppLinesDark);
	}
}

LPCTSTR CCalendarWnd::GetMenuText() const
{
	return _T("Calendar");
}

bool CCalendarWnd::ProcessMessage(MSG* /*pMsg*/) 
{
	if (!IsWindowEnabled())
		return false;

	// process editing shortcuts
	// TODO

	return false;
}

void CCalendarWnd::DoAppCommand(IUI_APPCOMMAND nCmd, DWORD /*dwExtra*/) 
{ 
	switch (nCmd)
	{
	case IUI_EXPANDALL:
		break;
		
	case IUI_COLLAPSEALL:
		break;
		
	case IUI_EXPANDSELECTED:
		break;
		
	case IUI_COLLAPSESELECTED:
		break;
		
	case IUI_TOGGLABLESORT:
		break;
		
	case IUI_SORT:
		break;
		
	case IUI_SETFOCUS:
		m_BigCalendar.SetFocus();
		JumpToTask();
		break;
	}
}

bool CCalendarWnd::CanDoAppCommand(IUI_APPCOMMAND nCmd, DWORD /*dwExtra*/) const 
{ 
	switch (nCmd)
	{
	case IUI_EXPANDALL:
		break;
		
	case IUI_COLLAPSEALL:
		break;
		
	case IUI_EXPANDSELECTED:
		break;
		
	case IUI_COLLAPSESELECTED:
		break;
		
	case IUI_TOGGLABLESORT:
		break;
		
	case IUI_SORT:
		break;
		
	case IUI_SETFOCUS:
		return true;
	}
	
	// all else
	return false;
}

bool CCalendarWnd::GetLabelEditRect(LPRECT pEdit)
{
	DWORD dwSelTask = m_BigCalendar.GetSelectedTaskID();
	m_BigCalendar.EnsureVisible(dwSelTask, FALSE);

	CRect rLabel;

	if (m_BigCalendar.GetTaskLabelRect(dwSelTask, rLabel))
	{
		m_BigCalendar.ClientToScreen(rLabel);

		// make sure it has a minimum height
		rLabel.bottom = (rLabel.top + max(rLabel.Height(), (DEF_TASK_HEIGHT - 1)));
		*pEdit = rLabel;

		return true;
	}

	// else
	return false;
}

IUI_HITTEST CCalendarWnd::HitTest(const POINT& ptScreen) const
{
	// try header
	// 6.9: disable header click because it changes the 
	// tree/list columns not the calendar columns
	if (m_BigCalendar.PtInHeader(ptScreen))
		return IUI_NOWHERE;//IUI_COLUMNHEADER;

	// then specific task
	CPoint ptBigCal(ptScreen);
	m_BigCalendar.ScreenToClient(&ptBigCal);

	if (m_BigCalendar.HitTest(ptBigCal))
		return IUI_TASK;

	// else try rest of big cal
	CRect rCal;
	m_BigCalendar.GetClientRect(rCal);

	return (rCal.PtInRect(ptBigCal) ? IUI_TASKLIST : IUI_NOWHERE);
}

static DWORD GdwTaskID = 0;
bool CCalendarWnd::SelectTask(DWORD dwTaskID)
{
	bool bSet = (m_BigCalendar.SelectTask(dwTaskID) != FALSE);

	UpdateSelectedTaskDates();
 
	GdwTaskID = dwTaskID;
 
	return bSet;
}

bool CCalendarWnd::SelectTasks(DWORD* /*pdwTaskIDs*/, int /*nTaskCount*/)
{
 
	return false; // only support single selection
}

bool CCalendarWnd::WantUpdate(int nAttribute) const
{
	return (CTaskCalendarCtrl::WantAttributeUpdate(nAttribute) != FALSE);
}

bool CCalendarWnd::PrepareNewTask(ITaskList* pTask) const
{
	return m_BigCalendar.PrepareNewTask(pTask);
}

void CCalendarWnd::UpdateTasks(const ITaskList* pTasks, IUI_UPDATETYPE nUpdate, int nEditAttribute)
{
	m_BigCalendar.UpdateTasks(pTasks, nUpdate, nEditAttribute);

	UpdateSelectedTaskDates();
}

void CCalendarWnd::Release()
{
	if (GetSafeHwnd())
		DestroyWindow();
	
	delete this;
}


void CCalendarWnd::JumpToTask()
{
	TASKCALITEM* pTCI = m_BigCalendar.GetTaskCalItem(GdwTaskID);
	if (pTCI)
	{
		COleDateTime Date = pTCI->GetAnyStartDate();
		m_MiniCalendar.SetDate(Date);
		m_BigCalendar.Goto(Date);


		CString strEquipmentID = pTCI->GetTaskEquipmentTo();
		if (strEquipmentID.GetLength() > 0)
		{
			int nIndex = m_BigCalendar.GetEquipmentIndex(strEquipmentID);
			m_BigCalendar.SetVertPos(nIndex);
		}
	}
}

void CCalendarWnd::OnSetFocus(CWnd* /*pOldWnd*/)
{
	//set focus to big calendar (seems the focus gets lost after switching back to the Calendar window from another app)
	m_BigCalendar.SetFocus();

	JumpToTask();
 
}

void CCalendarWnd::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);

	if (m_BigCalendar.GetSafeHwnd())
	{
		ResizeControls(cx, cy);

		// row height follows the new client height
		m_BigCalendar.Invalidate();
	}
}

BOOL CALLBACK CCalendarWnd::IsMiniCalSpecialDateCallback(COleDateTime &dt, DWORD_PTR dwUserData)
{
	ASSERT(dwUserData);

	CCalendarWnd* pThis = (CCalendarWnd*)dwUserData;

	return pThis->m_BigCalendar.IsSpecialDate(dt);
}


HBRUSH CCalendarWnd::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
	
	if (nCtlColor == CTLCOLOR_STATIC && m_brBack.GetSafeHandle())
	{
		pDC->SetTextColor(m_theme.crAppText);
		pDC->SetBkMode(TRANSPARENT);
		hbr = m_brBack;
	}

	return hbr;
}

BOOL CCalendarWnd::OnEraseBkgnd(CDC* pDC)
{
	// clip out children
	CDialogHelper::ExcludeCtrl(this, IDC_CALENDAR_FRAME, pDC);

	// then our background
	if (m_brBack.GetSafeHandle())
	{
		CRect rClient;
		GetClientRect(rClient);

		pDC->FillSolidRect(rClient, m_theme.crAppBackLight);
		return TRUE;
	}
	
	// else
	return CDialog::OnEraseBkgnd(pDC);
}

void CCalendarWnd::OnPreferences()
{
	if (m_dlgPrefs.DoModal() == IDOK)
	{
		// show/hide the minicalendar as necessary
		if (m_dlgPrefs.GetShowMiniCalendar())
			m_MiniCalendar.ShowWindow(SW_SHOW);
		else
			m_MiniCalendar.ShowWindow(SW_HIDE);

		// forward options to calendar ctrl
		UpdateCalendarCtrlPreferences();

		// call ResizeControls which will resize both calendars
		CRect rcFrame;
		GetClientRect(rcFrame);
		ResizeControls(rcFrame.Width(), rcFrame.Height());
	}

	//set focus to big calendar (seems the focus gets lost after the dialog is shown)
	m_BigCalendar.SetFocus();
}

void CCalendarWnd::InitFilterBar()
{
	// the frame starts immediately below the filter bar, so its template top is the bar height
	if (m_nFilterHeight <= 0)
		m_nFilterHeight = CDialogHelper::GetCtrlRect(this, IDC_CALENDAR_FRAME).top;

	// cache the filter bar labels, they are all unnamed so share IDC_STATIC
	m_nFilterLabelCount = 0;

	for (HWND hChild = ::GetWindow(GetSafeHwnd(), GW_CHILD); hChild; hChild = ::GetWindow(hChild, GW_HWNDNEXT))
	{
		if (::GetDlgCtrlID(hChild) != IDC_STATIC || m_nFilterLabelCount >= _countof(m_ahFilterLabels))
			continue;

		CRect rChild;
		::GetWindowRect(hChild, rChild);
		ScreenToClient(rChild);

		if (rChild.top < m_nFilterHeight)
			m_ahFilterLabels[m_nFilterLabelCount++] = hChild;
	}

	if (!m_btnFilter.GetSafeHwnd())
	{
		m_btnFilter.Create(_T("▲"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
						   CRect(0, 0, 0, 0), this, IDC_COLLAPSEFILTER);
		m_btnFilter.SetFont(GetFont());
	}

	// shown in place of the filter bar when collapsed
	if (!m_stFilterInfo.GetSafeHwnd())
	{
		m_stFilterInfo.Create(_T(""), WS_CHILD | SS_LEFT | SS_CENTERIMAGE,
							   CRect(0, 0, 0, 0), this);
		m_stFilterInfo.SetFont(GetFont());
	}
}

void CCalendarWnd::ShowFilterBar(BOOL bShow)
{
	static const UINT s_aFilterIds[] =
	{
		IDC_NUMWEEKS, IDC_SNAPMODES, IDC_GOTOTODAY, IDC_PREFERENCES, IDC_SELECTEDTASKDATES
	};

	for (int nID = 0; nID < _countof(s_aFilterIds); nID++)
	{
		CWnd* pCtrl = GetDlgItem(s_aFilterIds[nID]);

		if (pCtrl)
			pCtrl->ShowWindow(bShow ? SW_SHOW : SW_HIDE);
	}

	for (int nLbl = 0; nLbl < m_nFilterLabelCount; nLbl++)
		::ShowWindow(m_ahFilterLabels[nLbl], bShow ? SW_SHOW : SW_HIDE);

	if (m_stFilterInfo.GetSafeHwnd())
		m_stFilterInfo.ShowWindow(bShow ? SW_HIDE : SW_SHOW);
}

void CCalendarWnd::UpdateFilterInfo()
{
	if (!m_stFilterInfo.GetSafeHwnd())
		return;

	CString sInfo;

	if (m_BigCalendar.m_bEquipmentMode)
		sInfo.Format(_T("设备视图 · %d 行"), m_BigCalendar.GetVisibleEquipments());
	else
		sInfo.Format(_T("%d 周"), m_BigCalendar.GetVisibleWeeks());

	m_stFilterInfo.SetWindowText(sInfo);
}

void CCalendarWnd::ApplyFilterBarState()
{
	if (!m_btnFilter.GetSafeHwnd())
		return;

	ShowFilterBar(!m_bFilterCollapsed);
	m_btnFilter.SetWindowText(m_bFilterCollapsed ? _T("▼") : _T("▲"));

	CRect rClient;
	GetClientRect(rClient);
	ResizeControls(rClient.Width(), rClient.Height());
}

void CCalendarWnd::OnCollapseFilter()
{
	m_bFilterCollapsed = !m_bFilterCollapsed;
	ApplyFilterBarState();

	if (m_BigCalendar.GetSafeHwnd())
		m_BigCalendar.Invalidate();
}

void CCalendarWnd::ResizeControls(int cx, int cy)
{
	if (m_nFilterHeight <= 0)
		InitFilterBar();

	UpdateFilterInfo();

	// selected task dates takes available space, leaving room for the collapse toggle
	CRect rDates = CDialogHelper::GetCtrlRect(this, IDC_SELECTEDTASKDATES);
	rDates.right = max(rDates.left + 60, cx - COLLAPSE_BTN_W - 4);

	if (CWnd* pDates = GetDlgItem(IDC_SELECTEDTASKDATES))
		pDates->MoveWindow(rDates);

	// calendar frame
	CRect rFrame = CDialogHelper::GetCtrlRect(this, IDC_CALENDAR_FRAME);

	int nBarHeight = (m_bFilterCollapsed ? COLLAPSE_BAR_H : m_nFilterHeight);

	rFrame.top = nBarHeight;
	rFrame.right = cx;
	rFrame.bottom = cy;
	GetDlgItem(IDC_CALENDAR_FRAME)->MoveWindow(rFrame);

	// collapse toggle sits at the right hand end of the bar
	int nBtnTop = max(0, (nBarHeight - COLLAPSE_BTN_H) / 2);

	if (m_btnFilter.GetSafeHwnd())
		m_btnFilter.MoveWindow(cx - COLLAPSE_BTN_W - 2, nBtnTop, COLLAPSE_BTN_W, COLLAPSE_BTN_H);

	if (m_stFilterInfo.GetSafeHwnd())
		m_stFilterInfo.MoveWindow(6, nBtnTop, max(0, cx - COLLAPSE_BTN_W - 16), COLLAPSE_BTN_H);

	// mini-cal
	rFrame.DeflateRect(1, 1);
	int nMiniCalWidth = (m_dlgPrefs.GetShowMiniCalendar() ? m_MiniCalendar.GetMinWidth() : 0);

	CRect rItem(rFrame);
	rItem.right = rItem.left + nMiniCalWidth;

	m_MiniCalendar.MoveWindow(rItem);

	// big-cal
	rItem.left = rItem.right;
	rItem.right = rFrame.right;

	m_BigCalendar.MoveWindow(rItem);

	Invalidate(TRUE);
}

void CCalendarWnd::OnBigCalendarNotifyClick(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
	time_t dtSel = m_BigCalendar.GetFirstSelectedItem();

	if (dtSel > 0)
		m_MiniCalendar.SetDate(dtSel);

	*pResult = 0;
}

void CCalendarWnd::OnBigCalendarNotifyDblClk(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
	time_t dtSel = m_BigCalendar.GetFirstSelectedItem();

	if (dtSel > 0)
		m_MiniCalendar.SetDate(dtSel);

	*pResult = 0;
}

void CCalendarWnd::OnMiniCalendarNotifyClick(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
	COleDateTime dtSel = m_MiniCalendar.GetDate();
	m_BigCalendar.Goto(dtSel, true);

	*pResult = 0;
}

void CCalendarWnd::OnMiniCalendarNotifyDblClk(NMHDR* pNMHDR, LRESULT* pResult)
{
	OnMiniCalendarNotifyClick(pNMHDR, pResult);
}

void CCalendarWnd::OnGototoday() 
{
	COleDateTime dtToday = COleDateTime::GetCurrentTime();

	m_MiniCalendar.SetDate(dtToday);
	m_BigCalendar.GotoToday(true);
}

LRESULT CCalendarWnd::OnBigCalendarNotifyDateChange(WPARAM wp, LPARAM /*lp*/)
{
	COleDateTime dtStart, dtDue;
	CString sEquipmentID;
	if (m_BigCalendar.GetSelectedTaskDates(dtStart, dtDue))
	{
		m_BigCalendar.GetSelectedTaskEquipmentID(sEquipmentID);


		IUITASKMOD mod[3] = { 0 };
		int nNumMod = 0;

		switch (wp)
		{
		case TCCHT_BEGIN:
			if (CDateHelper::GetTimeT64(dtStart, mod[0].tValue))
			{
				mod[0].nAttrib = TDCA_STARTDATE;
				nNumMod = 1;

				if (sEquipmentID.GetLength() > 0)
				{
					mod[1].nAttrib = TDCA_EQUIPMENT;
					mod[1].szValue = sEquipmentID;
					nNumMod = 2;
				}
			}
			break;
			
		case TCCHT_MIDDLE:
			if (CDateHelper::GetTimeT64(dtStart, mod[0].tValue) &&
				CDateHelper::GetTimeT64(dtDue, mod[1].tValue))
			{
				mod[0].nAttrib = TDCA_STARTDATE;
				mod[1].nAttrib = TDCA_DUEDATE;
	
				nNumMod = 2;

				if (sEquipmentID.GetLength() > 0)
				{
					mod[2].nAttrib = TDCA_EQUIPMENT;
					mod[2].szValue = sEquipmentID;
					nNumMod = 3;
				}
			}
			break;
			
		case TCCHT_END:
			if (CDateHelper::GetTimeT64(dtDue, mod[0].tValue))
			{
				mod[0].nAttrib = TDCA_DUEDATE;
				nNumMod = 1;

				if (sEquipmentID.GetLength() > 0)
				{
					mod[1].nAttrib = TDCA_EQUIPMENT;
					mod[1].szValue = sEquipmentID;
					nNumMod = 2;
				}
			}
			break;
		}

		if (nNumMod)
			GetParent()->SendMessage(WM_IUI_MODIFYSELECTEDTASK, nNumMod, (LPARAM)(IUITASKMOD*)mod);
	}

	return 0L;
}

LRESULT CCalendarWnd::OnBigCalendarNotifySelectionChange(WPARAM /*wp*/, LPARAM lp)
{
	if (lp)
	{
		{  UpdateSelectedTaskDates();  }
		{  GetParent()->SendMessage(WM_IUI_SELECTTASK, 0, lp);  }
	}

	return 0L;
}

void CCalendarWnd::OnCancel()
{
	m_BigCalendar.CancelDrag();
}

void CCalendarWnd::UpdateSelectedTaskDates()
{
	COleDateTime dtStart, dtDue;
	CString sEquipmentID;
	if (m_BigCalendar.GetSelectedTaskDates(dtStart, dtDue))
	{
		//m_BigCalendar.GetSelectedTaskEquipmentID(sEquipmentID);

		CString sStart, sDue;

		if (CDateHelper::DateHasTime(dtStart))
			sStart = CDateHelper::FormatDate(dtStart, DHFD_TIME | DHFD_NOSEC);
		else
			sStart = CDateHelper::FormatDate(dtStart);

		if (CDateHelper::DateHasTime(dtDue))
			sDue = CDateHelper::FormatDate(dtDue, DHFD_TIME | DHFD_NOSEC);
		else
			sDue = CDateHelper::FormatDate(dtDue);

		m_sSelectedTaskDates.Format(_T("%s - %s"), sStart, sDue);
	}
	else
	{
		m_sSelectedTaskDates.Empty();
	}

	UpdateData(FALSE);
}

#include "..\ToDoList\ToDoListWnd.h"
static DWORD G_DraggingTaskID = NULL;
LRESULT CCalendarWnd::OnBigCalendarNotifyDragChange(WPARAM wp, LPARAM lp)
{
	if (wp = 1110)
	{
		if (G_DraggingTaskID != lp)
		{
			G_DraggingTaskID = lp;
			//李波 开始拖拽时，发消息给排产软件，获取拖拽状态的设备列表
			GetParent()->GetParent()->SendMessage(WM_TDL_CalendarBeginDragging, wp, lp);
		}

	}
	else
	{
		CDialogHelper::SelectItemByData(m_cbSnapModes, wp);
		UpdateSelectedTaskDates();
	}



	return 0L;
}

LRESULT CCalendarWnd::OnBigCalendarNotifyVisibleWeekChange(WPARAM /*wp*/, LPARAM /*lp*/)
{
	if ( m_BigCalendar.m_bEquipmentMode == false )
	{
		m_cbNumWeeks.SetCurSel(m_BigCalendar.GetVisibleWeeks() - 1);
	}
	else
	{
		m_cbNumWeeks.SetCurSel(CAL_CMB_EQUIP_FIRST + EquipRowPresetIndex(m_BigCalendar.GetVisibleEquipments()));
	}
 

	return 0L;
}

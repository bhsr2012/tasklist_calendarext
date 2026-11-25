// TaskCalendarCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "calprof.h"
#include "TaskCalendarCtrl.h"

#include "..\ToDoList\tdcenum.h"

#include "..\Shared\GraphicsMisc.h"
#include "..\Shared\themed.h"
#include "..\Shared\DateHelper.h"
#include "..\Shared\TimeHelper.h"
#include "..\Shared\misc.h"
#include "..\Shared\dialoghelper.h"

#include <math.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////

const double ONE_HOUR = (1.0 / 24.0);
const double END_OF_DAY = 0.999988425923; //(((24 * 60 * 60) - 1) / (24.0 * 60 * 60));
const int MIN_TASK_HEIGHT = (DEF_TASK_HEIGHT - 6);

/////////////////////////////////////////////////////////////////////////////

COLORREF TASKCALITEM::crDone = CLR_NONE;
static int GnMaxPos = 2;
/////////////////////////////////////////////////////////////////////////////

TASKCALITEM::TASKCALITEM()
	: 
	crText(CLR_NONE), 
	bDone(FALSE),
	bGoodAsDone(FALSE),
	dwTaskID(0)
{

}
	
TASKCALITEM::TASKCALITEM(const ITaskList14* pTasks, HTASKITEM hTask, DWORD dwCalcDates) 
	: 
	crText(CLR_NONE), 
	bDone(FALSE),
	bGoodAsDone(FALSE),
	dwTaskID(0)
{
	UpdateTask(pTasks, hTask, TDCA_ALL, dwCalcDates);
 
}

	
TASKCALITEM::TASKCALITEM(const TASKCALITEM& tci)
{
	*this = tci;
}

TASKCALITEM& TASKCALITEM::operator=(const TASKCALITEM& tci)
{
	sName = tci.sName;
	sFormattedName = tci.sFormattedName;
	crText = tci.crText;
	dwTaskID = tci.dwTaskID;
	bDone = tci.bDone;
	bGoodAsDone = tci.bGoodAsDone;
	dtCreation = tci.dtCreation;
	dtStart = tci.dtStart;
	dtEnd = tci.dtEnd;
	dtStartCalc = tci.dtStartCalc;
	dtEndCalc = tci.dtEndCalc;
	
	return (*this);
}

BOOL TASKCALITEM::operator==(const TASKCALITEM& tci)
{
	return ((sName == tci.sName) &&
		(crText == tci.crText) &&
		(dwTaskID == tci.dwTaskID) &&
		(bDone == tci.bDone) &&
		(bGoodAsDone == tci.bGoodAsDone) &&
		(dtCreation == tci.dtCreation) &&
		(dtStart == tci.dtStart) &&
		(dtEnd == tci.dtEnd) &&
		(dtStartCalc == tci.dtStartCalc) &&
		(dtEndCalc == tci.dtEndCalc));
}

void TASKCALITEM::UpdateTaskDates(const ITaskList14* pTasks, HTASKITEM hTask, int nEditAttrib, DWORD dwCalcDates)
{
	BOOL bUpdateAll = (nEditAttrib == TDCA_ALL);

	// check for quick exit
	BOOL bUpdateStart = WantAttributeUpdate(nEditAttrib, TDCA_STARTDATE);
	BOOL bUpdateEnd = (WantAttributeUpdate(nEditAttrib, TDCA_DUEDATE) || 
						WantAttributeUpdate(nEditAttrib, TDCA_DONEDATE));

	if (!bUpdateStart && !bUpdateEnd)
		return;

	time64_t tDate = 0;

	// get creation date once only
	if (!CDateHelper::IsDateSet(dtCreation) && pTasks->GetTaskCreationDate64(hTask, tDate))
		dtCreation = GetDate(tDate);

	// retrieve new dates
	if (bUpdateStart)
	{
		if (pTasks->GetTaskStartDate64(hTask, FALSE, tDate))
			dtStart = GetDate(tDate);
		else
			CDateHelper::ClearDate(dtStart);

		ReformatName();
	}

	if (bUpdateEnd)
	{
		if (pTasks->GetTaskDoneDate64(hTask, tDate))
		{
			dtEnd = GetDate(tDate);
			bDone = TRUE;
		}
		else if (pTasks->GetTaskDueDate64(hTask, FALSE, tDate))
		{
			dtEnd = GetDate(tDate);
			bDone = FALSE;
		}
		else
		{
			CDateHelper::ClearDate(dtEnd);
			bDone = FALSE;
		}
	}

	RecalcDates(dwCalcDates);
}

void TASKCALITEM::RecalcDates(DWORD dwCalcDates)
{
	const COleDateTime dtNow = CDateHelper::GetDateOnly(COleDateTime::GetCurrentTime());

	CDateHelper::ClearDate(dtStartCalc);
	CDateHelper::ClearDate(dtEndCalc);

	BOOL bHasStartDate = IsStartDateSet();
	BOOL bHasEndDate = IsEndDateSet();

	// calculate missing dates
	if (!bHasStartDate)
	{
		if (Misc::HasFlag(dwCalcDates, TCCO_CALCMISSINGSTARTASCREATION))
		{
			if (bHasEndDate)
				dtStartCalc = min(dtEnd, dtCreation);
			else
				dtStartCalc = dtCreation;
		}
		else if (Misc::HasFlag(dwCalcDates, TCCO_CALCMISSINGSTARTASDUE))
		{
			if (bHasEndDate)
				dtStartCalc = dtEnd;
		}
		else //if (Misc::HasFlag(dwCalcDates, TCCO_CALCMISSINGSTARTASEARLIESTDUEANDTODAY))
		{
			if (bHasEndDate)
				dtStartCalc = min(dtEnd, dtNow);
			else 
				dtStartCalc = dtNow;
		}

		dtStartCalc = CDateHelper::GetDateOnly(dtStartCalc);
	}

	if (!bHasEndDate)
	{
		if (Misc::HasFlag(dwCalcDates, TCCO_CALCMISSINGDUEASSTART))
		{
			if (bHasStartDate)
				dtEndCalc = dtStart;
		}
		else //if (Misc::HasFlag(dwCalcDates, TCCO_CALCMISSINGDUEASLATESTSTARTANDTODAY))
		{
			// take the later of today and dtStart
			if (bHasStartDate)
				dtEndCalc = max(dtStart, dtNow);
			else
				dtEndCalc = dtNow;
		}

		dtEndCalc = CDateHelper::GetDateOnly(dtEndCalc);
	}

	// adjust dtEnd to point to end of day if it has no time component
	if (IsEndDateSet())
	{
		if (!CDateHelper::DateHasTime(dtEnd))
			dtEnd.m_dt += END_OF_DAY;
	}
	else if (HasAnyEndDate())
	{
		if (!CDateHelper::DateHasTime(dtEndCalc))
			dtEndCalc.m_dt += END_OF_DAY;
	}

	// Finally ensure start date precedes end date
	if (bHasStartDate && bHasEndDate && (dtStart > dtEnd))
		dtStartCalc = CDateHelper::GetDateOnly(dtEnd);
}

BOOL TASKCALITEM::UpdateTask(const ITaskList14* pTasks, HTASKITEM hTask, int nEditAttrib, DWORD dwCalcDates)
{
	ASSERT(dwTaskID == 0 || pTasks->GetTaskID(hTask) == dwTaskID);

	if (dwTaskID == 0)
		dwTaskID = pTasks->GetTaskID(hTask);

	// snapshot current state to check for changes
	TASKCALITEM tciOrg = *this;

	if (nEditAttrib == TDCA_ALL || nEditAttrib == TDCA_TASKNAME)
		sName = pTasks->GetTaskTitle(hTask);
 
	sEquipmentName = pTasks->GetTaskEquipmentTo(hTask );
 
	UpdateTaskDates(pTasks, hTask, nEditAttrib, dwCalcDates);

	// always update colour
	crText = pTasks->GetTaskTextColor(hTask);

	// and 'Good as Done'
	bGoodAsDone = pTasks->IsTaskGoodAsDone(hTask);

	return !(*this == tciOrg);
}

CString  TASKCALITEM::GetTaskEquipmentTo() const
{
	return sEquipmentName;
}
void  TASKCALITEM::SetTaskEquipmentTo(CString & sEquipmentID)
{
	sEquipmentName = sEquipmentID;
}

BOOL TASKCALITEM::IsValid() const
{
	return (IsStartDateSet() || IsEndDateSet());
}

BOOL TASKCALITEM::IsDone() const
{
	return (bDone || bGoodAsDone);
}

COleDateTime TASKCALITEM::GetDate(time64_t tDate)
{
	COleDateTime date;

	if (tDate > 0)
		date = CDateHelper::GetDate(tDate);
	else
		CDateHelper::ClearDate(date);

	// else
	return date;
}

BOOL TASKCALITEM::IsStartDateSet() const
{
	return (CDateHelper::IsDateSet(dtStart) && !CDateHelper::IsDateSet(dtStartCalc));
}

BOOL TASKCALITEM::IsEndDateSet() const
{
	return (CDateHelper::IsDateSet(dtEnd) && !CDateHelper::IsDateSet(dtEndCalc));
}

BOOL TASKCALITEM::HasAnyStartDate() const
{
	return CDateHelper::IsDateSet(GetAnyStartDate());
}

BOOL TASKCALITEM::HasAnyEndDate() const
{
	return CDateHelper::IsDateSet(GetAnyEndDate());
}

COleDateTime TASKCALITEM::GetAnyStartDate() const
{
	// take calculated value in preference
	if (CDateHelper::IsDateSet(dtStartCalc))
		return dtStartCalc;
	else
		return dtStart;
}

COleDateTime TASKCALITEM::GetAnyEndDate() const
{
	// take calculated value in preference
	if (CDateHelper::IsDateSet(dtEndCalc))
		return dtEndCalc;
	else
		return dtEnd;
}

void TASKCALITEM::SetStartDate(const COleDateTime& date)
{
	ASSERT(CDateHelper::IsDateSet(date));

	dtStart = date;
	ReformatName();
}

void TASKCALITEM::SetEndDate(const COleDateTime& date)
{
	ASSERT(CDateHelper::IsDateSet(date));

	dtEnd = date;
}

COLORREF TASKCALITEM::GetFillColor(BOOL bSelected, BOOL bTextIsBack) const
{
	COLORREF crTemp = CLR_NONE;
	
	if (IsDone())
	{
		crTemp = GetTextColor(bSelected, bTextIsBack);
	}
	else if (HasTextColor())
	{
		if (bTextIsBack && !bSelected)
			return crText;
	}

	// check anything set
	if (crTemp == CLR_NONE)
	{
		if (HasTextColor())
			crTemp = crText;
		else
			return GetSysColor(COLOR_WINDOW);
	}

	return GraphicsMisc::Lighter(crTemp, 0.9);
}

void TASKCALITEM::SetDragingColor(COLORREF nColor)
{
	crDone = nColor;
}


COLORREF TASKCALITEM::GetBorderColor(BOOL bSelected, BOOL bTextIsBack) const
{
	if (IsDone())
	{
		if (HasTextColor())
			return crText;
	}
	else if (HasTextColor())
	{
		if (bTextIsBack && !bSelected)
			return GraphicsMisc::Darker(crText, 0.4);
		
		// else
		return crText;
	}
	
	// else
	return GetSysColor(COLOR_3DSHADOW);
}

COLORREF TASKCALITEM::GetTextColor(BOOL bSelected, BOOL bTextIsBack) const
{
	COLORREF crTemp = CLR_NONE;

	// overrides first
	if (IsDone())
	{
		if (crDone != CLR_NONE)
		{
			crTemp = crDone;
		}
		else if (!HasTextColor())
		{
			crTemp = GetSysColor(COLOR_3DSHADOW);
		}
	}
	else if (HasTextColor())
	{
		if (bTextIsBack && !bSelected)
			return GraphicsMisc::GetBestTextColor(crText);
	}

	// check anything set
	if (crTemp == CLR_NONE)
	{
		if (HasTextColor())
			crTemp = crText;
		else
			return GetSysColor(COLOR_WINDOWTEXT);
	}

	// handle selection
	return GraphicsMisc::Darker(crTemp, (bSelected ? 0.2 : 0.1));
}

BOOL TASKCALITEM::HasTextColor() const
{
	return ((crText != CLR_NONE) && (crText != GetSysColor(COLOR_WINDOWTEXT)));
}

BOOL TASKCALITEM::WantAttributeUpdate(int nEditAttrib, int nAttribMask)
{
	return ((nEditAttrib == TDCA_ALL) || (nEditAttrib == nAttribMask));
}

CString TASKCALITEM::GetName() const
{
	return (sFormattedName.IsEmpty() ? sName : sFormattedName);
}

void TASKCALITEM::ReformatName()
{
	// Prefix time if specified
	if (CDateHelper::DateHasTime(dtStart))
	{
		sFormattedName.Format(_T("%s - %s"), CTimeHelper::FormatClockTime(dtStart), sName);
	}
	else
	{
		sFormattedName.Empty();
	}
}


DWORD dwThreadID = 0;
static DWORD WINAPI  VScorll(void * pCTaskCalendarCtrl)
{
	CTaskCalendarCtrl * pCtrl = (CTaskCalendarCtrl *)pCTaskCalendarCtrl;
	Sleep(50);
	pCtrl->VScorllCheck();
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CTaskCalendarCtrl

CTaskCalendarCtrl::CTaskCalendarCtrl() 
	: 
	m_nMaxDayTaskCount(0), 
	m_bCellsCacheValid(FALSE),
	m_nCellsCacheTaskCount(-1),
	m_dwSelectedTaskID(0),
	m_bDraggingStart(FALSE), 
	m_bDraggingEnd(FALSE),
	m_bDragging(FALSE),
	m_ptDragOrigin(0),
	m_nSnapMode(TCCSM_NEARESTHOUR),
	m_dwOptions(TCCO_DISPLAYCONTINUOUS),
	m_bReadOnly(FALSE)
{
	for (int nRowInit = 0; nRowInit < CALENDAR_MAX_ROWS; nRowInit++)
	{
		m_nRowVScrollPos[nRowInit] = 0;
		m_bSbRowVisible[nRowInit] = FALSE;
		m_nSbRowRange[nRowInit] = -1;
		m_nSbRowPage[nRowInit] = -1;
		m_nSbRowPos[nRowInit] = -1;
		m_rcSbRow[nRowInit].SetRectEmpty();
	}

	GraphicsMisc::CreateFont(m_DefaultFont, _T("Tahoma"));

	m_LButtonDownPt.x = -1;
	m_LButtonDownPt.y = -1;
 
	this->RegisterTouchWindow();
 
}

CTaskCalendarCtrl::~CTaskCalendarCtrl()
{
	DeleteData();
 
	// clean up other heap allocations
	for(int i=0; i<CALENDAR_MAX_ROWS ; i++)
	{
		for(int u=0; u<CALENDAR_NUM_COLUMNS ; u++)
		{
			CCalendarCell* pCell = GetCell(i, u);

			delete static_cast<CTaskCalItemArray*>(pCell->pUserData);
			pCell->pUserData = NULL;
		}
	}
}

BEGIN_MESSAGE_MAP(CTaskCalendarCtrl, CCalendarCtrl)
	//{{AFX_MSG_MAP(CTaskCalendarCtrl)
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_SETCURSOR()
	ON_WM_LBUTTONUP()
	ON_WM_CAPTURECHANGED()
	ON_WM_KEYDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
	ON_WM_VSCROLL()
	ON_WM_HSCROLL()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTaskCalendarCtrl message handlers

INT_PTR CTaskCalendarCtrl::OnToolHitTest(CPoint point, TOOLINFO* pTI) const
{
	// perform a hit-test
	DWORD dwTaskID = HitTest(point);

	if (dwTaskID)
	{
		int nTextOffset = GetTaskTextOffset(dwTaskID);

		if ((nTextOffset != 0) || 
			!HasOption(TCCO_DISPLAYCONTINUOUS) ||
			(GetTaskHeight() < MIN_TASK_HEIGHT) ||
			m_bEquipmentMode)
		{
			pTI->hwnd = GetSafeHwnd();
			pTI->uId = dwTaskID;
			pTI->uFlags |= TTF_ALWAYSTIP;

			const TASKCALITEM* pTCI = GetTaskCalItem(dwTaskID);
			ASSERT(pTCI);

			if (!pTCI)
				return CCalendarCtrl::OnToolHitTest(point, pTI);

			// MFC will free the duplicated string
			pTI->lpszText = _tcsdup(pTCI->GetName());

			GetClientRect(&(pTI->rect));

			return dwTaskID;
		}
	}

	return CCalendarCtrl::OnToolHitTest(point, pTI);
}

void CTaskCalendarCtrl::SetOption(DWORD dwOption, BOOL bSet)
{
	if (dwOption)
	{
		DWORD dwPrev = m_dwOptions;

		if (bSet)
			m_dwOptions |= dwOption;
		else
			m_dwOptions &= ~dwOption;

		// specific handling
		if (m_dwOptions != dwPrev)
		{
			RecalcTaskDates();
			m_bCellsCacheValid = FALSE;
		}
	}
}

void CTaskCalendarCtrl::RecalcTaskDates()
{
	POSITION pos = m_mapData.GetStartPosition();
	DWORD dwTaskID = 0;
	TASKCALITEM* pTCI = NULL;

	while (pos)
	{
		m_mapData.GetNextAssoc(pos, dwTaskID, pTCI);
		ASSERT(pTCI);
		ASSERT(pTCI->dwTaskID == dwTaskID);

		pTCI->RecalcDates(m_dwOptions);
	}
}

bool CTaskCalendarCtrl::PrepareNewTask(ITaskList* pTask) const
{
	if (m_bEquipmentMode && m_nEquipmentCount < 1) return false;
	// give the task a date that will make it appear in the calendar
	time_t tDate = DateToSeconds((GetMaxDate().m_dt + GetMinDate().m_dt) / 2);

	int nRow, nCol;

	if (GetLastSelectedGridCell(nRow, nCol))
	{
		const CCalendarCell* pCell = GetCell(nRow, nCol);
		ASSERT(pCell);

		tDate = DateToSeconds(pCell->date);
	}

	if (tDate <= 0)
		return false;

	HTASKITEM hNewTask = pTask->GetFirstTask();
	ASSERT(hNewTask);

	pTask->SetTaskStartDate(hNewTask, tDate);
	pTask->SetTaskDueDate(hNewTask, tDate);

	return true;
}

BOOL CTaskCalendarCtrl::WantAttributeUpdate(int nEditAttrib)
{
	switch (nEditAttrib)
	{
	case TDCA_TASKNAME:
	case TDCA_DONEDATE:
	case TDCA_DUEDATE:
	case TDCA_STARTDATE:
	case TDCA_COLOR:
		return true;
	}
	
	// all else 
	return (nEditAttrib == TDCA_ALL);
}

void CTaskCalendarCtrl::UpdateTasks(const ITaskList* pTasks, IUI_UPDATETYPE nUpdate, int nEditAttribute)
{
	LONGLONG _tCalUT = CalPNow();
	const ITaskList14* pTasks14 = GetITLInterface<ITaskList14>(pTasks, IID_TASKLIST14);
	BOOL bChange = FALSE;

	switch (nUpdate)
	{
	case IUI_ALL:
		{ LONGLONG _t = CalPNow(); DeleteData(); CalPLog("  CAL:DeleteData", _t); }
		{ LONGLONG _t = CalPNow(); BuildData(pTasks14, pTasks->GetFirstTask(), TRUE); CalPLog("  CAL:BuildData", _t); }
		bChange = TRUE;
		break;
		
	case IUI_EDIT:
		bChange = UpdateTask(pTasks14, pTasks14->GetFirstTask(), nUpdate, nEditAttribute, TRUE);
		break;
		
	case IUI_DELETE:
		bChange = RemoveDeletedTasks(pTasks14);
		break;
		
	case IUI_ADD:
	case IUI_MOVE:
		ASSERT(0);
		break;
		
	default:
		ASSERT(0);
	}
	
	if (bChange)
	{
		m_bCellsCacheValid = FALSE;
		Invalidate(FALSE);
	}
	CalPLog("CAL:UpdateTasks", _tCalUT);
}

BOOL CTaskCalendarCtrl::RemoveDeletedTasks(const ITaskList14* pTasks)
{
	// traverse the data looking for items that do not 
	// exist in pTasks and delete them
	POSITION pos = m_mapData.GetStartPosition();
	DWORD dwTaskID = 0;
	TASKCALITEM* pTCI = NULL;
	BOOL bChange = FALSE;

	while (pos)
	{
		m_mapData.GetNextAssoc(pos, dwTaskID, pTCI);
		ASSERT(pTCI);
		ASSERT(pTCI->dwTaskID);

		if (pTasks->FindTask(pTCI->dwTaskID) == NULL)
		{
			delete pTCI;
			m_mapData.RemoveKey(dwTaskID);

			// clear selection if necessary
			if (m_dwSelectedTaskID == dwTaskID)
				m_dwSelectedTaskID = 0;

			bChange = TRUE;
		}
	}

	return bChange;
}

BOOL CTaskCalendarCtrl::UpdateTask(const ITaskList14* pTasks, HTASKITEM hTask, IUI_UPDATETYPE nUpdate, int nEditAttribute, BOOL bAndSiblings)
{
	if (hTask == NULL)
		return FALSE;

	ASSERT(nUpdate == IUI_EDIT);

	// handle task if not a reference
	if (_ttoi(pTasks->GetTaskAttribute(hTask, _T("REFID"))) > 0)
		return FALSE;

	DWORD dwTaskID = pTasks->GetTaskID(hTask);
	BOOL bChange = FALSE;

	// because we now accept completed tasks
	// we ought NEVER receive a edit notification
	// for a LEAF task we do not already have
	if (HasTask(dwTaskID)) 
	{
		TASKCALITEM* pTCI = GetTaskCalItem(dwTaskID);

		if (pTCI)
			bChange = pTCI->UpdateTask(pTasks, hTask, nEditAttribute, m_dwOptions);
	}
	else // must be a parent
	{
		HTASKITEM hSubtask = pTasks->GetFirstTask(hTask);
		//ASSERT(hSubtask);
		if ( hSubtask != NULL)
			bChange = UpdateTask(pTasks, hSubtask, nUpdate, nEditAttribute, TRUE);
	}
	
	// handle siblings WITHOUT RECURSION
	if (bAndSiblings)
	{
		HTASKITEM hSibling = pTasks->GetNextTask(hTask);
		
		while (hSibling)
		{
			// FALSE == not siblings
			if (UpdateTask(pTasks, hSibling, nUpdate, nEditAttribute, FALSE))
				bChange = TRUE;
			
			hSibling = pTasks->GetNextTask(hSibling);
		}
	}

	return bChange;
}

void CTaskCalendarCtrl::NotifyParentDateChange(TCC_HITTEST nHit)
{
	ASSERT(!m_bReadOnly);
	ASSERT(m_dwSelectedTaskID);

	if (nHit != TCCHT_NOWHERE)
		GetParent()->SendMessage(WM_CALENDAR_DATECHANGE, (WPARAM)nHit, m_dwSelectedTaskID);
}

void CTaskCalendarCtrl::NotifyParentDragChange()
{
	ASSERT(m_dwSelectedTaskID);

	GetParent()->SendMessage(WM_CALENDAR_DRAGCHANGE, (WPARAM)GetSnapMode(), m_dwSelectedTaskID);
}

BOOL CTaskCalendarCtrl::IsSpecialDate(const COleDateTime& date) const
{
	BOOL bDummy;
	return m_mapSpecial.Lookup(CDateHelper::GetDateOnly(date).m_dt, bDummy);
}

void CTaskCalendarCtrl::BuildData(const ITaskList14* pTasks, HTASKITEM hTask, BOOL bAndSiblings)
{
	if (hTask == NULL)
		return;

	// handle task unless it's a reference
	if (_ttoi(pTasks->GetTaskAttribute(hTask, _T("REFID"))) == 0)
	{
		// We are only interested in leaf-tasks
		HTASKITEM hSubTask = pTasks->GetFirstTask(hTask);
		
		if (hSubTask == NULL) // leaf-task
		{
			// sanity check
			DWORD dwTaskID = pTasks->GetTaskID(hTask);
			ASSERT(!HasTask(dwTaskID));
			
			TASKCALITEM* pTCI = new TASKCALITEM(pTasks, hTask, m_dwOptions);
			m_mapData[dwTaskID] = pTCI;
	 
			// process item for special dates
			if (pTCI->IsStartDateSet())
				m_mapSpecial[CDateHelper::GetDateOnly(pTCI->GetAnyStartDate())] = TRUE;
			
			if (pTCI->IsEndDateSet())
				m_mapSpecial[CDateHelper::GetDateOnly(pTCI->GetAnyEndDate())] = TRUE;
		}
		else // process children
		{
			BuildData(pTasks, hSubTask, TRUE);
		}
	}

	// handle siblings WITHOUT RECURSION
	if (bAndSiblings)
	{
		HTASKITEM hSibling = pTasks->GetNextTask(hTask);

		while (hSibling)
		{
			// FALSE == not siblings
			BuildData(pTasks, hSibling, FALSE);

			hSibling = pTasks->GetNextTask(hSibling);
		}
	}
}

BOOL CTaskCalendarCtrl::SetVisibleWeeks(int nWeeks)
{
	if (CCalendarCtrl::SetVisibleWeeks(nWeeks))
	{
		// delete text font so it can be recreated
		// at the right size
		if (HasOption(TCCO_ADJUSTTASKHEIGHTS))
			GraphicsMisc::VerifyDeleteObject(m_fontAltText);

		m_bCellsCacheValid = FALSE;
		return TRUE;
	}

	// else
	return FALSE;
}

int CTaskCalendarCtrl::CalcRequiredTaskFontPointSize() const
{
	if (!HasOption(TCCO_ADJUSTTASKHEIGHTS))
		return -1;

	int nTaskHeight = GetTaskHeight();

	if (nTaskHeight == DEF_TASK_HEIGHT)
		return -1;

	if (nTaskHeight < MIN_TASK_HEIGHT)
		return -1;

	return GraphicsMisc::PixelToPoint(nTaskHeight - 6);
}

void CTaskCalendarCtrl::DeleteData()
{
	POSITION pos = m_mapData.GetStartPosition();
	DWORD dwTaskID = 0;
	TASKCALITEM* pTCI = NULL;

	while (pos)
	{
		m_mapData.GetNextAssoc(pos, dwTaskID, pTCI);
		delete pTCI;
	}

	m_mapData.RemoveAll();
	m_mapSpecial.RemoveAll();

	m_bCellsCacheValid = FALSE;
	m_nCellsCacheTaskCount = -1;

	for (int nRowReset = 0; nRowReset < CALENDAR_MAX_ROWS; nRowReset++)
		m_nRowVScrollPos[nRowReset] = 0;
}

void CTaskCalendarCtrl::DrawHeader(CDC* pDC)
{
	CThemed th;
	BOOL bThemed = (th.AreControlsThemed() && th.Open(this, _T("HEADER")));
	
	CRect rc;
	GetGridRect(rc);
	
	rc.bottom = CALENDAR_HEADER_HEIGHT;
	float nWidth = 1.0 * rc.Width() / CALENDAR_NUM_COLUMNS;

	if (m_bEquipmentMode && nWidth < CALENDAR_FIRST_COL_WIDTH )
	{
		nWidth = ( 1.0 * rc.Width() - CALENDAR_FIRST_COL_WIDTH ) / ( CALENDAR_NUM_COLUMNS - 1 );
	}
	
	// Paint the whole header band first. The per-column rects below follow the
	// equipment-mode column rules and do not cover the left gutter (and, when the
	// first column is clamped, leave a strip free at its right edge). Those pixels
	// used to expose the user's system window colour (COLOR_WINDOW) as a bright
	// block in the top-right corner of the first cell.
	{
		CRect rcBand;
		GetClientRect(&rcBand);
		rcBand.right = rc.right;
		rcBand.bottom = CALENDAR_HEADER_HEIGHT;

		if (bThemed)
		{
			th.DrawBackground(pDC, HP_HEADERITEM, HIS_NORMAL, rcBand);
		}
		else
		{
			pDC->FillSolidRect(rcBand, GetSysColor(COLOR_3DFACE));
		}
	}

	CFont* pOldFont = pDC->SelectObject(&m_DefaultFont);
	bool bShort = false;
	int i;
	
	COleDateTime Day;
	int nBeginDay = GetCell(0,0)->date.m_dt;
	//int nDayOfWeek = BeginDay.GetDayOfWeek();
	
	if ( m_bEquipmentMode == false )
	{
		nBeginDay = 40000;
	}

	for (i = nBeginDay ; i < CALENDAR_NUM_COLUMNS + nBeginDay && !bShort; i++)
	{
		Day.m_dt = i;

		int nDOW = Day.GetDayOfWeek();
		
		CString csTitle = CDateHelper::GetWeekdayName(nDOW, FALSE);
 
		bShort = (pDC->GetTextExtent(csTitle).cx + 4 > nWidth);
 	}
	
	CRect rCol;
	
	for(i = nBeginDay ; i < CALENDAR_NUM_COLUMNS + nBeginDay; i++)
	{
		//int nDOW = GetDayOfWeek(i);
		
		if (m_bEquipmentMode && nWidth < CALENDAR_FIRST_COL_WIDTH)
		{
			rCol = CRect(rc.left+(i-nBeginDay-1)*nWidth+CALENDAR_FIRST_COL_WIDTH, 0, rc.left+(i-nBeginDay-1)*nWidth+CALENDAR_FIRST_COL_WIDTH + nWidth, CALENDAR_HEADER_HEIGHT);
		}
		else
		{
			rCol = CRect(rc.left+(i-nBeginDay)*nWidth, 0, rc.left+(i-nBeginDay+1)*nWidth, CALENDAR_HEADER_HEIGHT);
		}

		if (i - nBeginDay  == (CALENDAR_NUM_COLUMNS - 1))
		{
			rCol.right = rc.right;
		}
		else if (i == nBeginDay)
		{
			if (m_bEquipmentMode && nWidth < CALENDAR_FIRST_COL_WIDTH)
			{
				rCol.left = rc.left;
				rCol.right = rCol.left + CALENDAR_FIRST_COL_WIDTH;
			}
			else
			{
				// start after the left gutter, matching GetCellRect()
				rCol.left = rc.left;
				rCol.right = rCol.left + nWidth;
			}
		}
		else
		{
		//	rCol.right = rCol.left + nWidth;
		}
		
		// draw background
		if (bThemed)
		{
			th.DrawBackground(pDC, HP_HEADERITEM, HIS_NORMAL, rCol);
		}
		else
		{
			pDC->FillSolidRect(rCol, GetSysColor(COLOR_3DFACE));
			pDC->Draw3dRect(rCol, GetSysColor(COLOR_3DHIGHLIGHT), GetSysColor(COLOR_3DSHADOW));
		}
		
		Day.m_dt = i;
		int nDOW = Day.GetDayOfWeek();

		CString csTitle = CDateHelper::GetWeekdayName(nDOW, bShort)  ;


		CString strWeekDay = CDateHelper::GetWeekdayName(nDOW, bShort);
		if ( m_bEquipmentMode && i==nBeginDay )
		{
			int nMonth = Day.GetMonth();			
			csTitle = CDateHelper::GetMonthName(nMonth, FALSE) + _T("  ") + csTitle;
		}
		else if (bShort)
		{
			csTitle.Replace(_T("周"), _T(""));
		}
 
		CRect rText(rCol);
		rText.DeflateRect(0, 2, 0, 0);
		
		pDC->DrawText(csTitle, rText, DT_CENTER|DT_VCENTER);
		
		// next column
		rCol.left = rCol.right;
	}
	pDC->SelectObject(pOldFont);
}

void CTaskCalendarCtrl::DrawGrid(CDC* pDC)
{
	CCalendarCtrl::DrawGrid(pDC);
}

void CTaskCalendarCtrl::DrawCells(CDC* pDC)
{
	LONGLONG _tCalDC = CalPNow();
	// rebuild build display
	// NOTE: m_nMaxDayTaskCount/m_mapVertPos are filled by GetCellTasks().
	// They must survive a cache hit, otherwise no task can be positioned.

	if (m_mapData.GetCount())
	{
		// GetCellTasks 对每格都全表扫描(42 格 x N 任务)，若每次重绘都跑极慢。
		// 仅在数据变更/任务数变化/首格日期(月份)变化时重建每格任务数组。
		BOOL bRebuildCells = (!m_bCellsCacheValid)
			|| (m_nCellsCacheTaskCount != m_mapData.GetCount())
			|| (GetCell(0, 0)->date != m_dtCellsCacheFirst);

		if (bRebuildCells)
		{
			m_nMaxDayTaskCount = 0;
			m_mapVertPos.RemoveAll();
			m_mapTextOffset.RemoveAll();
			m_mapTaskCell.RemoveAll();
		}

		for(int i=0; i<CALENDAR_MAX_ROWS ; i++)
		{
			for(int u=0; u<CALENDAR_NUM_COLUMNS ; u++)
			{
				CCalendarCell* pCell = GetCell(i, u);
				ASSERT(pCell);

				CTaskCalItemArray* pTasks = static_cast<CTaskCalItemArray*>(pCell->pUserData);

				if (pTasks == NULL)
				{
					pTasks = new CTaskCalItemArray();
					pCell->pUserData = pTasks;
					bRebuildCells = TRUE;
				}

				ASSERT(pTasks);

				if (bRebuildCells)
				{
					GetCellTasks(pCell, *pTasks, TRUE);

					// first visible cell of each task so GetGridCellFromTask()
					// does not have to rescan every task for every cell
					DWORD dwCell = 0;

					for (int t = 0; t < pTasks->GetSize(); t++)
					{
						DWORD dwTaskID = (*pTasks)[t]->dwTaskID;

						if (dwTaskID && !m_mapTaskCell.Lookup(dwTaskID, dwCell))
							m_mapTaskCell[dwTaskID] = MAKELONG(u, i);
					}
				}
			}
		}

		if (bRebuildCells)
		{
			m_bCellsCacheValid = TRUE;
			m_nCellsCacheTaskCount = m_mapData.GetCount();
			m_dtCellsCacheFirst = GetCell(0, 0)->date;
		}
	}
	else
	{
		m_bCellsCacheValid = FALSE;
		m_nCellsCacheTaskCount = -1;
	}

	// create alternate text font as required
	int nSize = CalcRequiredTaskFontPointSize();

	if (nSize == -1)
	{
		GraphicsMisc::VerifyDeleteObject(m_fontAltText);
	}
	else
	{
		int nCurSize = GraphicsMisc::GetFontPointSize(m_fontAltText);

		if (nSize != nCurSize)
		{
			GraphicsMisc::VerifyDeleteObject(m_fontAltText);
			VERIFY(GraphicsMisc::CreateFont(m_fontAltText, _T("Tahoma"), nSize));
		}
	}

	// single scrollbar per row, refreshed once per frame (never per cell)
	UpdateRowScrollBars();

	{ LONGLONG _t = CalPNow(); CCalendarCtrl::DrawCells(pDC); CalPLog("  CAL:DrawCells.base", _t); }
	CalPLog("CAL:DrawCells", _tCalDC);
}

bool CTaskCalendarCtrl::DrawCell(CDC* pDC, const CCalendarCell* pCell, const CRect& rCell, 
							 BOOL bSelected, BOOL bToday, BOOL bShowMonth)
{
	return CCalendarCtrl::DrawCell(pDC, pCell, rCell, bSelected, bToday, bShowMonth);
}

void CTaskCalendarCtrl::DrawCellContent(CDC* pDC, const CCalendarCell* pCell, const CRect& rCell, 
										BOOL bSelected, BOOL bToday)
{
	// default drawing
	CCalendarCtrl::DrawCellContent(pDC, pCell, rCell, bSelected, bToday);

	// then ours
	if (!m_nMaxDayTaskCount)
		return;

	CTaskCalItemArray* pTasks = static_cast<CTaskCalItemArray*>(pCell->pUserData);
	ASSERT(pTasks);

	int nNumTasks = (pTasks ? pTasks->GetSize() : 0);

	if (!nNumTasks)
		return;

	// no per-cell scrollbar any more: the row scrollbar scrolls every cell of
	// that row, so the whole cell area stays available for tasks and tooltips
	int nRow = GetCellRowIndex(pCell);
	CRect rCellTrue(rCell);
	BOOL bVScrolled = (HasOption(TCCO_DISPLAYCONTINUOUS) || IsRowScrollBarActive(nRow));
	BOOL bFocused = CDialogHelper::IsChildOrSame(this, GetFocus());
	BOOL bTextColorIsBkgnd = HasOption(TCCO_TASKTEXTCOLORISBKGND);

	int nTaskHeight = GetTaskHeight();
	int nStart = (bVScrolled ? GetRowScrollOffset(nRow) : 0);
	
	for (int nTask = nStart; nTask < nNumTasks; nTask++)
	{
		const TASKCALITEM* pTCI = (*pTasks)[nTask];
		ASSERT(pTCI);
		
		CRect rTask;
		
		if (!CalcTaskCellRect(pTCI->dwTaskID, pCell, rCellTrue, rTask))
			continue;

		int nSaveDC = pDC->SaveDC();

		// draw selection
		// 李波 画选中的任务
		BOOL bSelTask = (pTCI->dwTaskID == m_dwSelectedTaskID);
		COLORREF crText = pTCI->GetTextColor(bSelTask, bTextColorIsBkgnd);
 
		if (bSelTask)
		{
 			DWORD dwSelFlags = GMIB_THEMECLASSIC;
 			CRect rClip(rCell);

			if (rTask.left <= rCellTrue.left)
				dwSelFlags |= GMIB_CLIPLEFT;

			if (rTask.right >= rCellTrue.right)
				dwSelFlags |= GMIB_CLIPRIGHT;

			GM_ITEMSTATE nState = (bFocused ? GMIS_SELECTED : GMIS_SELECTEDNOTFOCUSED);
			crText = GraphicsMisc::GetExplorerItemTextColor(crText, nState, GMIB_THEMECLASSIC);

			GraphicsMisc::DrawExplorerItemBkgnd(pDC, *this, nState, rTask, dwSelFlags, rClip);

			// 李波 判断拖拽任务条时，原始cell的设备信息与任务信息匹配,画绿色背景
			CString strCellEquipmentID = GetEquipmentID(pCell->nEquipmentIndex);
			if ( m_bDragging && strCellEquipmentID.Compare(pTCI->GetTaskEquipmentTo()) == 0)
			{
				pDC->FillSolidRect(rTask, RGB(0, 255, 0));
			}
 
		}
		else // draw task
		{
			DWORD dwFlags = GMDR_TOP;
			
			if (rTask.left > rCellTrue.left)
				dwFlags |= GMDR_LEFT;
			
			if (rTask.right < rCellTrue.right)
				dwFlags |= GMDR_RIGHT;
			
			if (rTask.bottom < rCellTrue.bottom)
				dwFlags |= GMDR_BOTTOM;
			
			COLORREF crFill = pTCI->GetFillColor(bSelTask, bTextColorIsBkgnd);
			COLORREF crBorder = pTCI->GetBorderColor(bSelTask, bTextColorIsBkgnd);
						
			GraphicsMisc::DrawRect(pDC, rTask, crFill, crBorder, 0, dwFlags);
		}
		
		// draw text if there is enough space
		if (nTaskHeight >= MIN_TASK_HEIGHT)
		{
			int nOffset = GetTaskTextOffset(pTCI->dwTaskID);
			CFont* pOldFont = NULL;
			
			if (nOffset != -1)
			{
				if (nOffset == 0)
					rTask.left += 3; // initial pos
				
				int nLeft = (rTask.left - nOffset);
				int nTop = (rTask.top + 1);
				
				if (m_fontAltText.GetSafeHandle())
				{
					pOldFont = pDC->SelectObject(&m_fontAltText);
				}
				else
				{
					if (pTCI->IsDone() && m_fontDone.GetSafeHandle())
						pOldFont = pDC->SelectObject(&m_fontDone);

					nTop++;
				}

				COLORREF crOld = pDC->SetTextColor(crText);
				CString sTitle = pTCI->GetName();
				
				pDC->ExtTextOut(nLeft, nTop, ETO_CLIPPED, rTask, sTitle, NULL);
				pDC->SetTextColor(crOld);
				
				// update text pos
				nOffset += rTask.Width();
				
				// if the offset now exceeds the text extent we can stop
				int nExtent = pDC->GetTextExtent(sTitle).cx;
				
				if (nOffset >= nExtent)
					nOffset = -1;
				
				m_mapTextOffset[pTCI->dwTaskID] = nOffset;

				// restore font
				pDC->SelectObject(pOldFont);
			}
						
			if (rTask.bottom >= rCellTrue.bottom)
				break;
		}
	}
}

void CTaskCalendarCtrl::SetDoneTaskAttributes(COLORREF color, BOOL bStrikeThru)
{
	TASKCALITEM::crDone = color;

	if (bStrikeThru)
	{
		VERIFY(GraphicsMisc::CreateFont(m_fontDone, m_DefaultFont, GMFS_STRIKETHRU, GMFS_STRIKETHRU));
	}
	else
	{
		GraphicsMisc::VerifyDeleteObject(m_fontDone);
	}
}

void CTaskCalendarCtrl::OnSetFocus(CWnd* pFocus)
{
	CCalendarCtrl::OnSetFocus(pFocus);

	UpdateRowScrollBars();
	Invalidate(FALSE);
}

void CTaskCalendarCtrl::OnKillFocus(CWnd* pFocus)
{
	CCalendarCtrl::OnKillFocus(pFocus);

	UpdateRowScrollBars();
	Invalidate(FALSE);
}

void CTaskCalendarCtrl::DrawCellFocus(CDC* /*pDC*/, const CCalendarCell* /*pCell*/, const CRect& /*rCell*/)
{
	// we handle the focus during drawing
	// CCalendarCtrl::DrawCellFocus(pDC, pCell, rCell);
}

BOOL CTaskCalendarCtrl::UpdateRowScrollBars()
{
	int nTaskHeight = GetTaskHeight();

	if (nTaskHeight < 1)
		nTaskHeight = 1;

	int nSbWidth = GetSystemMetrics(SM_CXVSCROLL);
	int nFirstDayCol = (m_bEquipmentMode ? 1 : 0);

	struct ROWSBINFO { BOOL bHaveRow; int nMaxCount; int nVisible; CRect rRow; };
	ROWSBINFO info[CALENDAR_MAX_ROWS];

	for (int nPass = 0; nPass < 2; nPass++)
	{
		BOOL bNeedAny = FALSE;

		for (int nRow = 0; nRow < CALENDAR_MAX_ROWS; nRow++)
		{
			info[nRow].bHaveRow = FALSE;
			info[nRow].nMaxCount = 0;
			info[nRow].nVisible = 0;
			info[nRow].rRow.SetRectEmpty();

			for (int nCol = nFirstDayCol; nCol < CALENDAR_NUM_COLUMNS; nCol++)
			{
				CRect rCell;

				if (!GetCellRect(nRow, nCol, rCell, TRUE))
					continue;

				if (!info[nRow].bHaveRow)
					info[nRow].rRow = rCell;
				else
					info[nRow].rRow.bottom = rCell.bottom;

				info[nRow].bHaveRow = TRUE;

				const CCalendarCell* pCell = GetCell(nRow, nCol);

				if (!pCell)
					continue;

				const CTaskCalItemArray* pTasks = static_cast<const CTaskCalItemArray*>(pCell->pUserData);

				if (pTasks)
					info[nRow].nMaxCount = max(info[nRow].nMaxCount, pTasks->GetSize());
			}

			if (info[nRow].bHaveRow)
				info[nRow].nVisible = (info[nRow].rRow.Height() / nTaskHeight);

			// Every visible equipment row gets a scrollbar strip so the last
			// row never loses its bar. Rows without overflow get a disabled bar.
			if (info[nRow].bHaveRow && (info[nRow].nVisible > 0))
				bNeedAny = TRUE;
		}

		// the strip is reserved only when some row overflows, and it always
		// sits to the LEFT of the grid so it never covers a single cell
		int nWantGutter = (bNeedAny ? (nSbWidth + 4) : 0);

		if (GetLeftGutter() != nWantGutter)
		{
			SetLeftGutter(nWantGutter);

			if (nPass == 0)
				continue; // cell geometry changed, recompute once
		}

		for (int nRow = 0; nRow < CALENDAR_MAX_ROWS; nRow++)
		{
			int nMaxCount = info[nRow].nMaxCount;
			int nVisible = info[nRow].nVisible;
			BOOL bShow = (info[nRow].bHaveRow && (nVisible > 0));

			if (!bShow)
			{
				if (m_bSbRowVisible[nRow])
				{
					if (m_sbRowVScroll[nRow].GetSafeHwnd())
						m_sbRowVScroll[nRow].ShowWindow(SW_HIDE);

					m_bSbRowVisible[nRow] = FALSE;
				}

				if (m_sbRowVScroll[nRow].GetSafeHwnd())
					m_sbRowVScroll[nRow].EnableWindow(FALSE);

				m_nRowVScrollPos[nRow] = 0;
				continue;
			}

			int nMaxPos = max(0, (nMaxCount - nVisible));
			BOOL bCanScroll = (nMaxCount > nVisible);

			if (m_nRowVScrollPos[nRow] < 0)
				m_nRowVScrollPos[nRow] = 0;

			if (m_nRowVScrollPos[nRow] > nMaxPos)
				m_nRowVScrollPos[nRow] = nMaxPos;

			// the gutter itself: x = [0, nSbWidth), exactly one bar per row
			CRect rSb(0, info[nRow].rRow.top, nSbWidth, info[nRow].rRow.bottom);

			if (rSb.bottom <= rSb.top)
				continue;

			SCROLLINFO si = { sizeof(si), (SIF_PAGE | SIF_POS | SIF_RANGE) };
			si.nMin = 0;
			si.nMax = max(nVisible, nMaxCount) - 1;
			si.nPage = nVisible;
			si.nPos = m_nRowVScrollPos[nRow];

			BOOL bSame = (m_bSbRowVisible[nRow] &&
						  m_sbRowVScroll[nRow].GetSafeHwnd() &&
						  (m_sbRowVScroll[nRow].IsWindowEnabled() == bCanScroll) &&
						  m_nSbRowRange[nRow] == si.nMax &&
						  m_nSbRowPage[nRow] == (int)si.nPage &&
						  m_nSbRowPos[nRow] == m_nRowVScrollPos[nRow] &&
						  m_rcSbRow[nRow] == rSb);

			if (bSame)
				continue;

			if (!m_sbRowVScroll[nRow].GetSafeHwnd())
			{
				if (!m_sbRowVScroll[nRow].Create(WS_CHILD | SBS_VERT, rSb, this, (UINT)(2000 + nRow)))
					continue;
			}
			else
			{
				m_sbRowVScroll[nRow].MoveWindow(rSb);
			}

			m_sbRowVScroll[nRow].EnableWindow(bCanScroll);
			m_sbRowVScroll[nRow].SetScrollInfo(&si, m_sbRowVScroll[nRow].IsWindowVisible());
			m_sbRowVScroll[nRow].ShowWindow(SW_SHOW);
			m_sbRowVScroll[nRow].Invalidate(FALSE);

			m_bSbRowVisible[nRow] = TRUE;
			m_nSbRowRange[nRow] = si.nMax;
			m_nSbRowPage[nRow] = (int)si.nPage;
			m_nSbRowPos[nRow] = m_nRowVScrollPos[nRow];
			m_rcSbRow[nRow] = rSb;
		}

		return TRUE;
	}

	return TRUE;
}

int CTaskCalendarCtrl::GetCellRowIndex(const CCalendarCell* pCell) const
{
	if (!pCell)
		return -1;

	for (int nRow = 0; nRow < CALENDAR_MAX_ROWS; nRow++)
	{
		for (int nCol = 0; nCol < CALENDAR_NUM_COLUMNS; nCol++)
		{
			if (GetCell(nRow, nCol) == pCell)
				return nRow;
		}
	}

	return -1;
}

int CTaskCalendarCtrl::GetRowScrollOffset(int nRow) const
{
	if ((nRow < 0) || (nRow >= CALENDAR_MAX_ROWS))
		return 0;

	return m_nRowVScrollPos[nRow];
}

int CTaskCalendarCtrl::GetSelectedCellRow() const
{
	int nRow = -1, nCol = -1;

	if (!GetLastSelectedGridCell(nRow, nCol))
		return -1;

	return nRow;
}


#if _MSC_VER >= 1400
BOOL CTaskCalendarCtrl::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
#else
void CTaskCalendarCtrl::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
#endif
{
	// if the mouse is over the cell with the scrollbar
	// then pass to the scrollbar instead
	int nRow, nCol;
	
	// pt is in screen coords
	ScreenToClient(&pt);
 
	if (GetGridCellFromPoint(pt, nRow, nCol) &&
		IsRowScrollBarActive(nRow))
	{
		int nLine = (abs(zDelta) / 120);
		UINT nDir = ((zDelta < 0) ? SB_LINEDOWN : SB_LINEUP);
		while (nLine--)
			SendMessage(WM_VSCROLL, nDir, (LPARAM)m_sbRowVScroll[nRow].GetSafeHwnd());

#if _MSC_VER >= 1400
		return TRUE; // eat
#else
		return; // eat
#endif
	}

#if _MSC_VER >= 1400
		return CCalendarCtrl::OnMouseWheel(nFlags, zDelta, pt);
#else
		CCalendarCtrl::OnMouseWheel(nFlags, zDelta, pt);
#endif
}


void CTaskCalendarCtrl::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	if (m_bEquipmentMode == false)
	{
		m_nHscrollPos = 0;
		SetScrollPos(SB_HORZ, m_nHscrollPos, TRUE);
		SetFocus();
		CWnd::OnHScroll(nSBCode, nPos, pScrollBar);
		return;
	}
	

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
	

	
	SetFocus();
	CWnd::OnHScroll(nSBCode, nPos, pScrollBar);
}

int CTaskCalendarCtrl::GetTaskHeight() const
{
	int nHeight = DEF_TASK_HEIGHT;

	if (m_nMaxDayTaskCount && HasOption(TCCO_ADJUSTTASKHEIGHTS))
	{
		CRect rCell;
		GetCellRect(0, 0, rCell, TRUE);

		nHeight = (rCell.Height() / m_nMaxDayTaskCount);
	}

	return max(MIN_TASK_HEIGHT, min(nHeight, DEF_TASK_HEIGHT));
}

//李波  拖拽任务  对Task和Cell的匹配，加入判断设备ID
int CTaskCalendarCtrl::GetCellTasks(const CCalendarCell * pCell, CTaskCalItemArray& aTasks, BOOL bOrdered) const
{
	COleDateTime  dtCell = pCell->date;

	ASSERT(dtCell);

#ifdef _DEBUG
	int nDay = dtCell.GetDay();
	int nMonth = dtCell.GetMonth();
#endif

	aTasks.RemoveAll();

	CString strCellEquipmentID = GetEquipmentID(pCell->nEquipmentIndex);
	//AfxMessageBox(strCellEquipmentID);

	double dCellStart = dtCell, dCellEnd = dCellStart + 1.0;
	POSITION pos = m_mapData.GetStartPosition();
	DWORD dwTaskID = 0;
	TASKCALITEM* pTCI = NULL;

	while (pos)
	{
		m_mapData.GetNextAssoc(pos, dwTaskID, pTCI);

		// ignore tasks with both start and end dates calculated
		if (!pTCI->IsValid())
			continue;

		// ignore completed tasks as required
		if (pTCI->IsDone() && !HasOption(TCCO_DISPLAYDONE))
			continue;

		// draw continuous either if the flag is set or the item is selected
		if (HasOption(TCCO_DISPLAYCONTINUOUS))
		{
			if ((pTCI->GetAnyStartDate().m_dt < dCellEnd) && 
				(pTCI->GetAnyEndDate().m_dt >= dCellStart)   )
			{
				//if (m_bEquipmentMode == false || (m_bEquipmentMode == true && strCellEquipmentID.Compare(pTCI->GetTaskEquipmentTo()) == 0))
				//如果是非设备模式
				if (m_bEquipmentMode == false )
				{
					aTasks.Add(pTCI);
				}
				//设备模式下，任务的设备信息与日历单元格的设备相符合
				else if (m_bEquipmentMode == true && strCellEquipmentID.Compare(pTCI->GetTaskEquipmentTo()) == 0)
				{
					aTasks.Add(pTCI);
				}
				//设备模式下，正在拖拽任务，该任务可用的设备信息，与 日历单元格的设备相符合
				else if (m_bEquipmentMode == true &&  m_bDragging && pTCI->dwTaskID == GetSelectedTaskID())  
				{
					//李波  这里 设备与拖拽 m_DragingEquipments设备相符合， 稍后添加接口判断当前任务适合由那些设备加工
					bool bDraggingEquipment = false;
					for (int pp = 0; pp < m_DragingEquipments.size(); pp++)
					{
						if (strCellEquipmentID.Compare(m_DragingEquipments[pp]) == 0)
						{
							bDraggingEquipment = true;
							break;
						}
					}
 
					 if(bDraggingEquipment)
					 {
						 aTasks.Add(pTCI);
					 }

					 //如果是拖拽的任务是当前设置的设备，颜色变一下
					 if (strCellEquipmentID.Compare(pTCI->GetTaskEquipmentTo()) == 0)
					 {
						 pTCI->SetDragingColor(RGB(255, 0, 0));
					 }
				}
				
			}
		}
		else
		{
			BOOL bAdded = FALSE; // only add once

			if (HasOption(TCCO_DISPLAYCALCSTART) || (HasOption(TCCO_DISPLAYSTART) && pTCI->IsStartDateSet()))
			{
				if (CDateHelper::GetDateOnly(pTCI->GetAnyStartDate()) == dtCell   )
				{
					//if (m_bEquipmentMode == false || (m_bEquipmentMode == true && strCellEquipmentID.Compare(pTCI->GetTaskEquipmentTo()) == 0))
	 

					//如果是非设备模式
					if (m_bEquipmentMode == false)
					{
						aTasks.Add(pTCI);
						bAdded = TRUE;
					}
					//设备模式下，任务的设备信息与日历单元格的设备相符合
					else if (m_bEquipmentMode == true && strCellEquipmentID.Compare(pTCI->GetTaskEquipmentTo()) == 0)
					{
						aTasks.Add(pTCI);
						bAdded = TRUE;

						//如果是拖拽的任务是当前设置的设备，颜色变一下
						if (m_bDragging && pTCI->dwTaskID == GetSelectedTaskID())
						{
							pTCI->SetDragingColor(RGB(255, 0, 0));
						}
					}
					//设备模式下，正在拖拽任务，该任务可用的设备信息，与 日历单元格的设备相符合
					else if (m_bEquipmentMode == true && m_bDragging && pTCI->dwTaskID == GetSelectedTaskID())
					{
						//李波  这里先假定与01 02 03 设备相符合， 稍后添加接口判断当前任务适合由那些设备加工
						if (strCellEquipmentID.Compare(_T("01")) == 0 ||
							strCellEquipmentID.Compare(_T("02")) == 0 ||
							strCellEquipmentID.Compare(_T("03")) == 0)
						{
							aTasks.Add(pTCI);
							bAdded = TRUE;
						}
					}

				}
			}

			// only test for due/done if start not added
			if (!bAdded)
			{
				if (HasOption(TCCO_DISPLAYCALCDUE) || 
					((HasOption(TCCO_DISPLAYDUE) || HasOption(TCCO_DISPLAYDONE)) && pTCI->IsEndDateSet()))
				{
					if (CDateHelper::GetDateOnly(pTCI->GetAnyEndDate()) == dtCell   )
					//if (m_bEquipmentMode == false || (m_bEquipmentMode == true && strCellEquipmentID.Compare(pTCI->GetTaskEquipmentTo()) == 0))

					if (m_bEquipmentMode == false)
					{
						aTasks.Add(pTCI);
					}
					//设备模式下，任务的设备信息与日历单元格的设备相符合
					else if (m_bEquipmentMode == true && strCellEquipmentID.Compare(pTCI->GetTaskEquipmentTo()) == 0)
					{
						aTasks.Add(pTCI);

						//如果是拖拽的任务是当前设置的设备，颜色变一下
						if (m_bDragging && pTCI->dwTaskID == GetSelectedTaskID())
						{
							pTCI->SetDragingColor(RGB(255, 0, 0));
						}
					}
					//设备模式下，正在拖拽任务，该任务可用的设备信息，与 日历单元格的设备相符合
					else if (m_bEquipmentMode == true && m_bDragging && pTCI->dwTaskID == GetSelectedTaskID())
					{
						//李波  这里先假定与01 02 03 设备相符合， 稍后添加接口判断当前任务适合由那些设备加工
						bool bEquipmentIsFit = false;
						for (int k = 0; k < m_DragingEquipments.size();k++)
						{
							if (strCellEquipmentID.Compare(m_DragingEquipments[k]) == 0)
							{
								bEquipmentIsFit = true;
								break;
							}
						}

						if (bEquipmentIsFit == true)
						{
							aTasks.Add(pTCI);
						}

						//如果是拖拽的任务是当前设置的设备，颜色变一下
						if (strCellEquipmentID.Compare(pTCI->GetTaskEquipmentTo()) == 0)
						{
							pTCI->SetDragingColor(RGB(255, 0, 0));
						}

					}
				}
			}
		}
	}

	if (bOrdered && aTasks.GetSize() > 1)
		qsort(aTasks.GetData(), aTasks.GetSize(), sizeof(TASKCALITEM*), CompareTCItems);

	// now go thru the list and set the position of each item 
	// if not already done
	int nMaxPos = 0;

	for (int nTask = 0; nTask < aTasks.GetSize(); nTask++)
	{
		const TASKCALITEM* pTCI = aTasks[nTask];
		ASSERT(pTCI);

		int nPos;

		if (!m_mapVertPos.Lookup(pTCI->dwTaskID, nPos))
		{
			nPos = max(nMaxPos, nTask);
			m_mapVertPos[pTCI->dwTaskID] = nPos;
		}

		nMaxPos = max(nMaxPos, nPos+1);
	}

	m_nMaxDayTaskCount = max(m_nMaxDayTaskCount, nMaxPos);

	return aTasks.GetSize();
}

int CTaskCalendarCtrl::CompareTCItems(const void* pV1, const void* pV2)
{
	typedef TASKCALITEM* PTASKCALITEM;

	const TASKCALITEM* pTCI1 = *(static_cast<const PTASKCALITEM*>(pV1));
	const TASKCALITEM* pTCI2 = *(static_cast<const PTASKCALITEM*>(pV2));

	// special case: Not drawing tasks continuous means that
	// the same task can appear twice
	if (pTCI1->dwTaskID == pTCI2->dwTaskID)
	{
		//ASSERT(!HasOption(TCCO_DISPLAYCONTINUOUS));
		return 0;
	}
	
	// earlier start date
	if (pTCI1->GetAnyStartDate() < pTCI2->GetAnyStartDate())
		return -1;

	if (pTCI1->GetAnyStartDate() > pTCI2->GetAnyStartDate())
		return 1;

	// equal so test for later end date
	if (pTCI1->GetAnyEndDate() > pTCI2->GetAnyEndDate())
		return -1;

	if (pTCI1->GetAnyEndDate() < pTCI2->GetAnyEndDate())
		return 1;

	// equal so test for task ID
	if (pTCI1->dwTaskID < pTCI2->dwTaskID)
		return -1;

	ASSERT(pTCI1->dwTaskID > pTCI2->dwTaskID);
	return 1;
}

DWORD CTaskCalendarCtrl::HitTest(const CPoint& ptCursor) const
{
	TCC_HITTEST nHit = TCCHT_NOWHERE;
	return HitTest(ptCursor, nHit);
}

#include "..\ToDoList\tdcmsg.h"
DWORD CTaskCalendarCtrl::HitTest(const CPoint& ptCursor, TCC_HITTEST& nHit) const
{
	nHit = TCCHT_NOWHERE;

	if (!m_nMaxDayTaskCount)
		return 0;

	int nRow, nCol;

	if (!GetGridCellFromPoint(ptCursor, nRow, nCol))
		return 0;

	const CCalendarCell* pCell = GetCell(nRow, nCol);
	ASSERT(pCell);
	
	if (pCell == NULL)
		return 0;

	CTaskCalItemArray* pTasks = static_cast<CTaskCalItemArray*>(pCell->pUserData);
	ASSERT(pTasks);
	
	if (!pTasks || !pTasks->GetSize())
		return 0;
	
	// determine the vertical 'task pos' of the cursor
	CRect rCell;
 
	GetCellRect(nRow, nCol, rCell, TRUE);
	
	// handle clicking above tasks
	if (ptCursor.y < rCell.top)
		return 0;
	
	int nTaskHeight = GetTaskHeight();
	int nPos = ((ptCursor.y - rCell.top) / nTaskHeight);

	//判断鼠标是否点中任务
	// look thru the tasks for this pos
	for (int nTask = 0; nTask < pTasks->GetSize(); nTask++)
	{
		const TASKCALITEM* pTCI = (*pTasks)[nTask];
		ASSERT(pTCI);

		BOOL bVScrolled = (IsGridCellSelected(pCell) || HasOption(TCCO_DISPLAYCONTINUOUS));
		int nTaskPos = GetTaskVertPos(pTCI->dwTaskID, bVScrolled, nRow);

		if (nTaskPos < 0)
			continue;
		
		//计算最下面能显示的一个任务的位置
		//已经获得修正后的任务位置


		if (nTaskPos == nPos)
		{
			// now we figure out where on the item we hit
			COleDateTime dtHit;
			if (GetDateFromPoint(ptCursor, dtHit) == false)
			{
				return 0; 
			}
			
			// now check for closeness to ends
			double dDateTol = CalcDateDragTolerance();
			
			if (fabs(dtHit.m_dt - pTCI->GetAnyStartDate().m_dt) < dDateTol)
			{
				nHit = TCCHT_BEGIN;
			}
			else if (fabs(dtHit.m_dt - pTCI->GetAnyEndDate().m_dt) < dDateTol)
			{
				nHit = TCCHT_END;
			}
			else if (dtHit > pTCI->GetAnyStartDate() && dtHit < pTCI->GetAnyEndDate())
			{
				nHit = TCCHT_MIDDLE;
			}

 
			//李波加入变更设备的消息
			//CString strCellEquipmentID = GetEquipmentID(pCell->nEquipmentIndex);
			//GetParent()->GetParent()->GetParent()->SendMessage(WM_TDCN_CHANGEEQUIPMENT, TRUE, (LPARAM)(LPCTSTR)strCellEquipmentID);
			
			return (nHit == TCCHT_NOWHERE) ? 0 : pTCI->dwTaskID;
		}
	}

	// nothing hit
	return 0;
}

double CTaskCalendarCtrl::CalcDateDragTolerance() const
{
	CRect rClient;
	GetClientRect(rClient);

	// calc equivalent of DRAG_WIDTH in days
	double dOneDay = ((double)rClient.Width() / CALENDAR_NUM_COLUMNS);
	double dDragTol = (GetSystemMetrics(SM_CXSIZEFRAME) / dOneDay);

	return min(dDragTol, 1.0);
}

void CTaskCalendarCtrl::EnsureVisible(DWORD dwTaskID, BOOL bShowStart)
{
	if (!bShowStart) // partial visibility ok
	{
		// is the task already visible to some degree
		int nRow, nCol;

		if (GetGridCellFromTask(dwTaskID, nRow, nCol))
 			return;
	}

	// else make it visible
	TASKCALITEM* pTCI = GetTaskCalItem(dwTaskID);
	ASSERT(pTCI);

	if (!pTCI)
		return;

	COleDateTime dtMin = GetMinDate(), dtMax = GetMaxDate();

	if (bShowStart)
	{
		if (pTCI->GetAnyStartDate() < dtMin || pTCI->GetAnyStartDate() > dtMax)
		{
			// need to scroll
			Goto(pTCI->GetAnyStartDate());
		}
	}
	else // allow any visibility
	{
		if (pTCI->GetAnyEndDate() <= dtMin || pTCI->GetAnyStartDate() >= dtMax)
		{
			// need to scroll
			Goto(pTCI->GetAnyStartDate());
		}
	}
}

 

bool CTaskCalendarCtrl::GetGridCellFromTask(DWORD dwTaskID, int &nRow, int &nCol) const
{
	DWORD dwCell = 0;

	LONGLONG _tGCT = CalPNow();

	if (m_bCellsCacheValid && m_mapTaskCell.Lookup(dwTaskID, dwCell))
	{
		nCol = LOWORD(dwCell);
		nRow = HIWORD(dwCell);
		CalPLog("  CAL:GridCell.fast", _tGCT);
		return true;
	}

	CalPLog("  CAL:GridCell.slow", _tGCT);

	// iterate the visible cells for the specified task
	CTaskCalItemArray aTasks;

	for(int i=0; i < GetVisibleWeeks() ; i++)
	{
		for(int u=0; u<CALENDAR_NUM_COLUMNS; u++)
		{
			const CCalendarCell* pCell = GetCell(i, u);
	 
			int nTask = GetCellTasks(pCell, aTasks, FALSE);

			while (nTask--)
			{
				if (aTasks[nTask]->dwTaskID == dwTaskID)
				{
					nRow = i;
					nCol = u;

					return true;
				}
			}
		}
	}

	return false;
}

BOOL CTaskCalendarCtrl::GetTaskLabelRect(DWORD dwTaskID, CRect& rLabel) const
{
	int nRow, nCol;

	// start with visibility check
	if (!GetGridCellFromTask(dwTaskID, nRow, nCol))
 		return FALSE;

	const CCalendarCell* pCell = GetCell(nRow, nCol);
	ASSERT(pCell);

	CRect rCell;
	VERIFY(GetCellRect(nRow, nCol, rCell, TRUE));

	return CalcTaskCellRect(dwTaskID, pCell, rCell, rLabel);
}

BOOL CTaskCalendarCtrl::CalcTaskCellRect(DWORD dwTaskID, const CCalendarCell* pCell, const CRect& rCell, CRect& rTask) const
{
	// check horizontal (date) intersection first
	TASKCALITEM* pTCI = GetTaskCalItem(dwTaskID);
	ASSERT(pTCI);

	if (!pTCI)
		return FALSE;

	double dCellStart = pCell->date;
	double dCellEnd = dCellStart + END_OF_DAY;

	if (pTCI->GetAnyEndDate().m_dt < dCellStart || pTCI->GetAnyStartDate().m_dt > dCellEnd)
		return FALSE;

	int nTaskHeight = GetTaskHeight();

	// check vertical (pos) intersection next
	BOOL bVScrolled = (IsGridCellSelected(pCell) || HasOption(TCCO_DISPLAYCONTINUOUS));
	int nPos = GetTaskVertPos(dwTaskID, bVScrolled, GetCellRowIndex(pCell));

	if (nPos < 0)
		return FALSE;

	ASSERT(nPos < m_nMaxDayTaskCount);

	

	if ((rCell.top + (nPos * nTaskHeight)) >= rCell.bottom)
		return FALSE;

	// calc rest of sides
	rTask = rCell;
 
	//计算最下面能显示的一个任务的位置

	rTask.top += (nPos * nTaskHeight);
	rTask.bottom = min(rCell.bottom, (rTask.top + nTaskHeight - 1));

 

	// left edge
	if (pTCI->GetAnyStartDate().m_dt == dCellStart)
	{ 
		// whole day
		rTask.left++;
	}
	else if (pTCI->GetAnyStartDate().m_dt > dCellStart)
	{ 
		// partial day
		rTask.left += (int)((pTCI->GetAnyStartDate().m_dt - dCellStart) * rCell.Width());
	}	
			
	// right edge
	if (pTCI->GetAnyEndDate().m_dt == dCellEnd) // whole day
	{
		rTask.right--;
	}
	else if (pTCI->GetAnyEndDate().m_dt < dCellEnd)
	{
		// partial day
		rTask.right -= (int)((dCellEnd - pTCI->GetAnyEndDate().m_dt) * rCell.Width());
	}

	return TRUE;
}


void CTaskCalendarCtrl::SetVertPos(int nPos)
{
	SetVScrollPos(nPos);
	Invalidate();
}

int CTaskCalendarCtrl::GetTaskVertPos(DWORD dwTaskID, BOOL bVScrolled, int nRow) const
{
	ASSERT(dwTaskID);
	int nPos = -1;

	m_mapVertPos.Lookup(dwTaskID, nPos);

	if ((nPos < 0) || (nPos >= m_nMaxDayTaskCount))
		return -1;

	if (bVScrolled)
		nPos -= GetRowScrollOffset(nRow);
 
	////李波，控制 nPos的大小
	//if (nPos > GnMaxPos)
	//{
	//	nPos = (nPos + 1) % (GnMaxPos + 1);
	//	if (nPos > 0) nPos--;
	//}

 	return nPos;
}

int CTaskCalendarCtrl::GetTaskTextOffset(DWORD dwTaskID) const
{
	ASSERT(dwTaskID);
	int nPos = 0;

	// special case: Always return zero if NOT drawing continuous
	// and this task is NOT selected
	if (!HasOption(TCCO_DISPLAYCONTINUOUS)/* && (m_dwSelectedTaskID != dwTaskID)*/)
		return 0;

	if (!m_mapTextOffset.Lookup(dwTaskID, nPos))
		m_mapTextOffset[dwTaskID] = nPos;

	return nPos;
}

TASKCALITEM* CTaskCalendarCtrl::GetTaskCalItem(DWORD dwTaskID) const
{
	ASSERT(dwTaskID);
	TASKCALITEM* pTCI = NULL;

	m_mapData.Lookup(dwTaskID, pTCI);
	ASSERT(pTCI);

	return pTCI;
}

BOOL CTaskCalendarCtrl::HasTask(DWORD dwTaskID) const
{
	if (dwTaskID == 0)
		return FALSE;

	TASKCALITEM* pTCI = NULL;
	return m_mapData.Lookup(dwTaskID, pTCI);
}

// external version
BOOL CTaskCalendarCtrl::SelectTask(DWORD dwTaskID)
{
	return SelectTask(dwTaskID, FALSE);
}

// internal version
BOOL CTaskCalendarCtrl::SelectTask(DWORD dwTaskID, BOOL bNotify)
{
	if (!HasTask(dwTaskID))
		return FALSE;

	if (dwTaskID != m_dwSelectedTaskID)
	{
		DWORD dwPrevSelected = m_dwSelectedTaskID;

		m_dwSelectedTaskID = dwTaskID;

		LONGLONG _tSel = CalPNow();

		if (bNotify)
		{ LONGLONG _t = CalPNow(); GetParent()->SendMessage(WM_CALENDAR_SELCHANGE, 0, dwTaskID); CalPLog("    CAL:Sel.notify", _t); }

		int nTaskPos = GetTaskVertPos(dwTaskID, true, -1);

		// repaint only the two bars whose appearance changed: a full
		// Invalidate() costs a whole grid redraw on every click
		{ LONGLONG _t = CalPNow(); InvalidateTaskRect(dwPrevSelected); InvalidateTaskRect(dwTaskID); CalPLog("    CAL:Sel.inval", _t); }
		{ LONGLONG _t = CalPNow(); UpdateWindow(); CalPLog("    CAL:Sel.update", _t); }

		CalPLog("  CAL:SelectTask", _tSel);
	}

	return TRUE;
}
void CTaskCalendarCtrl::InvalidateTaskRect(DWORD dwTaskID)
{
	if (!dwTaskID || !GetSafeHwnd())
		return;

	for (int nRow = 0; nRow < CALENDAR_MAX_ROWS; nRow++)
	{
		for (int nCol = 0; nCol < CALENDAR_NUM_COLUMNS; nCol++)
		{
			const CCalendarCell* pCell = GetCell(nRow, nCol);

			if (!pCell)
				continue;

			const CTaskCalItemArray* pTasks = static_cast<const CTaskCalItemArray*>(pCell->pUserData);

			if (!pTasks)
				continue;

			for (int nTask = 0; nTask < pTasks->GetSize(); nTask++)
			{
				if ((*pTasks)[nTask]->dwTaskID != dwTaskID)
					continue;

				CRect rCell, rTask;

				if (GetCellRect(nRow, nCol, rCell) &&
					CalcTaskCellRect(dwTaskID, pCell, rCell, rTask))
				{
					rTask.InflateRect(1, 1);
					InvalidateRect(&rTask, FALSE);
				}

				break;
			}
		}
	}
}

//static CString  G_Msg = _T("");

static bool  G_TouchUp = false;
void CTaskCalendarCtrl::OnLButtonDown(UINT nFlags, CPoint point) 
{
	DWORD dwSelID = HitTest(point);
	
	if (dwSelID)
	{
		SetFocus();
		SelectTask(dwSelID, TRUE);

		if (!m_bReadOnly)
			StartDragging(point);
	}
	else
	{
		CCalendarCtrl::OnLButtonDown(nFlags, point);
		UpdateWindow();
	}
}

void CTaskCalendarCtrl::VScorllCheck()
{
	//还在移动时
	if (G_TouchUp == false)
	{

	}
	//touch完成
	else
	{

		//GetCursorPos(&m_LButtonUpPt);
		//CString str1;
		//str1.Format(_T("G_TouchUp=true(%d)-"), (int)m_LButtonUpPt.y);
		//G_Msg += str1;

		CRect rc;
		GetClientRect(&rc);
 
		int nMiddle = 0.5* (rc.bottom + rc.top);
		int nDiv = rc.Height()/6 ;

		if ( m_LButtonDownPt.y > nMiddle)
		{
			//CString str2;
			//str2.Format(_T("(DownPt.y=%d,UpPt.y=%d)-"), (int)m_LButtonDownPt.y, (int)m_LButtonUpPt.y);
			//G_Msg += str2;
			int nRepeat = 1 + (m_LButtonDownPt.y - nMiddle) / nDiv;

			for (int i=0; i<nRepeat; i++)
			{
				OnMouseWheel(0, -125, m_LButtonDownPt);
			}
 
		}
		else if (m_LButtonDownPt.y < nMiddle)
		{
			int nRepeat = 1 + (nMiddle- m_LButtonDownPt.y) / nDiv;
 
			for (int i = 0; i < nRepeat; i++)
			{
				OnMouseWheel(0, 125, m_LButtonDownPt);
			}
		}


		//char FilePath[MAX_PATH];
		//strcpy(FilePath, "D://Touch.txt");
		//FILE *TXTFile;
		//TXTFile = fopen(FilePath, "a");
 
		//fwprintf_s(TXTFile, _T("\n%s"), G_Msg);
		//fclose(TXTFile);//关闭

		//G_Msg = _T("");
 
		m_LButtonDownPt.x = -1;
		m_LButtonDownPt.y = -1;
 
	}
}

BOOL CTaskCalendarCtrl::PreTranslateMessage(MSG* pMsg)
{
	//if (pMsg->message != 512 && pMsg->message != 15 && pMsg->message != 585 && pMsg->message != 582 && pMsg->message != 581)
	//{
	//	CString strMsg;
	//	strMsg.Format(_T("%d"), pMsg->message);
	//	AfxMessageBox(strMsg);
	//}
	if (pMsg->message == 581)   // 582 581
	{
		G_TouchUp = false;
		////m_LButtonUpPt = pMsg->pt;
		//GetCursorPos(&m_LButtonUpPt);
 
		//CString str1;
		//str1.Format(_T("581(%d)-"), (int)m_LButtonUpPt.y);
		//G_Msg += str1;

		//HANDLE hThread = CreateThread(NULL, 0, VScorll, this, NULL, &dwThreadID);
 		
 	}
	else if (pMsg->message == 582)   // 582 581
	{
		G_TouchUp = false;
		//CString str1;
		//str1.Format(_T("CH=%d,582(%d)-"), GetCellHeight(),(int)pMsg->pt.y);
		//G_Msg += str1;
	 
		if (m_LButtonDownPt.x < 0 && m_LButtonDownPt.y < 0)
		{
			m_LButtonDownPt = pMsg->pt;
			ScreenToClient(&m_LButtonDownPt);
			//m_LButtonDownPt = 
		}
	}
	else if (pMsg->message == 583)   // 582 581 581 581 583
	{
		G_TouchUp = true;

		//GetCursorPos(&m_LButtonUpPt);
		//CString str1;
		//str1.Format(_T("583(%d)-"), (int)m_LButtonUpPt.y);
		//G_Msg += str1;
 
		HANDLE hThread = CreateThread(NULL, 0, VScorll, this, NULL, &dwThreadID);
	}
	return CWnd::PreTranslateMessage(pMsg);
}

void CTaskCalendarCtrl::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{

	for (int nRow = 0; nRow < CALENDAR_MAX_ROWS; nRow++)
	{
		if (pScrollBar != &m_sbRowVScroll[nRow])
			continue;

		SCROLLINFO si = { sizeof(si), 0 };
		m_sbRowVScroll[nRow].GetScrollInfo(&si, (SIF_PAGE | SIF_POS | SIF_RANGE));

		switch (nSBCode)
		{
		case SB_BOTTOM:			m_nRowVScrollPos[nRow] = si.nMax; break;
		case SB_TOP:				m_nRowVScrollPos[nRow] = 0; break;

		case SB_LINEDOWN:		m_nRowVScrollPos[nRow] += 1; break;
		case SB_LINEUP:			m_nRowVScrollPos[nRow] -= 1; break;

		case SB_PAGEDOWN:		m_nRowVScrollPos[nRow] += si.nPage; break;
		case SB_PAGEUP:			m_nRowVScrollPos[nRow] -= si.nPage; break;

		case SB_THUMBPOSITION:	m_nRowVScrollPos[nRow] = nPos; break;
		case SB_THUMBTRACK:		m_nRowVScrollPos[nRow] = nPos; break;

		default:
			return;
		}

		m_nRowVScrollPos[nRow] = min(m_nRowVScrollPos[nRow], (si.nMax - (int)si.nPage + 1));
		m_nRowVScrollPos[nRow] = max(m_nRowVScrollPos[nRow], 0);

		if (m_nRowVScrollPos[nRow] != si.nPos)
		{
			m_sbRowVScroll[nRow].SetScrollPos(m_nRowVScrollPos[nRow]);

			CRect rRowCell;

			if (GetCellRect(nRow, 0, rRowCell, TRUE))
			{
				CRect rcClient;
				GetClientRect(&rcClient);

				CRect rRow(0, rRowCell.top, rcClient.right, rRowCell.bottom);
				InvalidateRect(rRow, TRUE);
			}
			else
			{
				Invalidate();
			}
		}

		return;
	}

	CCalendarCtrl::OnVScroll(nSBCode, nPos, pScrollBar);
}



static DWORD G_DraggingTaskID = NULL; 
BOOL CTaskCalendarCtrl::StartDragging(const CPoint& ptCursor)
{
	ASSERT(!m_bReadOnly);

	TCC_HITTEST nHit = TCCHT_NOWHERE;
	DWORD dwTaskID = HitTest(ptCursor, nHit);

	ASSERT((nHit == TCCHT_NOWHERE) || (dwTaskID != 0));

	if (nHit == TCCHT_NOWHERE)
		return FALSE;

	// when not drawing tasks continuously, it's possible
	// for the act of selecting a task to change its
	// position and thus its hit-test result
	if (dwTaskID != m_dwSelectedTaskID)
		return FALSE;

	if (!::DragDetect(GetSafeHwnd(), ptCursor))
		return FALSE;
 
	switch (nHit)
	{
	case TCCHT_BEGIN:
		SetCursor(AfxGetApp()->LoadStandardCursor(IDC_SIZEWE));
		m_bDraggingStart = TRUE;
		
		if (G_DraggingTaskID != m_dwSelectedTaskID)
		{
			G_DraggingTaskID = m_dwSelectedTaskID;
			//李波 开始拖拽时，发消息给排产软件，获取拖拽状态的设备列表
			GetParent()->SendMessage(WM_CALENDAR_DRAGCHANGE, (WPARAM)1110, m_dwSelectedTaskID);
		}

		break;
		
	case TCCHT_END:
		m_bDraggingEnd = TRUE;
		SetCursor(AfxGetApp()->LoadStandardCursor(IDC_SIZEWE));
		break;
		
	case TCCHT_MIDDLE:
		
		m_bDragging = TRUE;
		break;
		
	default:
		ASSERT(0);
		return FALSE;
	}
	
	TASKCALITEM* pDragTCI = GetTaskCalItem(dwTaskID);

	if (!pDragTCI)
	{
		m_bDragging = m_bDraggingStart = m_bDraggingEnd = FALSE;
		return FALSE;
	}

	m_tciPreDrag = *pDragTCI;
	m_ptDragOrigin = ptCursor;

	SetCapture();

	// keep parent informed
	NotifyParentDragChange();

	return TRUE;
}

BOOL CTaskCalendarCtrl::GetValidDragDate(const CPoint& ptCursor, COleDateTime& dtDrag) const
{
	CPoint ptDrag(ptCursor);

	if (!ValidateDragPoint(ptDrag))
		return FALSE;

	if (!GetDateFromPoint(ptDrag, dtDrag))
		return FALSE;

	// if dragging the whole task, then we calculate
	// dtDrag as TASKCALITEM::dtStart/dtEnd offset by the
	// difference between the current drag pos and the
	// initial drag pos
	if (m_bDragging)
	{
		COleDateTime dtOrg;
		GetDateFromPoint(m_ptDragOrigin, dtOrg);
		
		// offset from pre-drag position
		double dOffset = dtDrag.m_dt - dtOrg.m_dt;

		if (m_tciPreDrag.IsStartDateSet())
			dtDrag = m_tciPreDrag.GetAnyStartDate().m_dt + dOffset;
		else
		{
			ASSERT(m_tciPreDrag.IsEndDateSet());
			dtDrag = m_tciPreDrag.GetAnyEndDate().m_dt + dOffset;
		}
		
		SetCursor(AfxGetApp()->LoadStandardCursor(IDC_SIZEALL));
	}
	
	// adjust date depending on snap mode
	switch (GetSnapMode())
	{
	case TCCSM_NEARESTHOUR:
		dtDrag = CDateHelper::GetNearestHour(dtDrag, m_bDraggingEnd);
		break;

	case TCCSM_NEARESTDAY:
		dtDrag = CDateHelper::GetNearestDay(dtDrag, m_bDraggingEnd);
		break;

	case TCCSM_NEARESTHALFDAY:
		dtDrag = CDateHelper::GetNearestHalfDay(dtDrag, m_bDraggingEnd);
		break;

	case TCCSM_FREE:
		if (m_bDraggingEnd)
		{
			double dTime = CDateHelper::GetTimeOnly(dtDrag);

			if (dTime >= END_OF_DAY)
				dtDrag = CDateHelper::MakeDate(dtDrag, END_OF_DAY);
		}
		break;

	case TCCSM_NONE:
	default:
		ASSERT(0);
		return FALSE;
	}
	
	return TRUE;
}

TCC_SNAPMODE CTaskCalendarCtrl::GetSnapMode() const
{
	if (IsDragging())
	{
		// active keys override
		if (Misc::ModKeysArePressed(MKS_CTRL))
		{
			m_nSnapMode = TCCSM_NEARESTHOUR;
		}
		else if (Misc::ModKeysArePressed(MKS_SHIFT))
		{
			m_nSnapMode = TCCSM_NEARESTDAY;
		}
		else if (Misc::ModKeysArePressed(MKS_CTRL | MKS_SHIFT))
		{
			m_nSnapMode = TCCSM_NEARESTHALFDAY;
		}
	}

	return m_nSnapMode;
}

void CTaskCalendarCtrl::OnMouseMove(UINT nFlags, CPoint point) 
{
	if (!m_bReadOnly && UpdateDragging(point))
		return;
  
 
	CCalendarCtrl::OnMouseMove(nFlags, point);
}

BOOL CTaskCalendarCtrl::UpdateDragging(const CPoint& ptCursor)
{
	if (IsDragging())
	{
		TASKCALITEM* pTCI = GetTaskCalItem(m_dwSelectedTaskID);
		ASSERT(pTCI);

		if (!pTCI)
			return FALSE;
			
		if (IsValidDrag(ptCursor))
		{
			COleDateTime dtDrag;

			if (GetValidDragDate(ptCursor, dtDrag))
			{
				// prevent the start and end dates from overlapping
				if (m_bDraggingStart)
				{
					pTCI->SetStartDate(min(dtDrag.m_dt, pTCI->GetAnyEndDate().m_dt - ONE_HOUR));
				}
				else if (m_bDraggingEnd)
				{
					pTCI->SetEndDate(max(dtDrag.m_dt, pTCI->GetAnyStartDate().m_dt + ONE_HOUR));
				}
				else // m_bDragging
				{
					if (pTCI->IsStartDateSet() && pTCI->IsEndDateSet())
					{
						double dDuration = (pTCI->GetAnyEndDate().m_dt - pTCI->GetAnyStartDate().m_dt);
						
						pTCI->SetStartDate(dtDrag);
						pTCI->SetEndDate(dtDrag.m_dt + dDuration);
					}
					else if (pTCI->IsStartDateSet())
					{
						pTCI->SetStartDate(dtDrag);
					}
					else if (pTCI->IsEndDateSet())
					{
						pTCI->SetEndDate(dtDrag);
					}
				}
			}
		}
		else
		{
			*pTCI = m_tciPreDrag;
		}

		// always recalc dates
		if (pTCI->IsEndDateSet() && !pTCI->IsStartDateSet() && (pTCI->GetAnyEndDate() < pTCI->GetAnyStartDate()))
		{
			int breakpoint = 0;
		}
		pTCI->RecalcDates(m_dwOptions);
			
		Invalidate();
		UpdateWindow();

		// keep parent informed
		NotifyParentDragChange();

		return TRUE;
	}

	// else
	return FALSE;
}

void CTaskCalendarCtrl::OnLButtonUp(UINT nFlags, CPoint point) 
{
	m_MouseMovePoint.x = point.x;
	m_MouseMovePoint.y = point.y;

	if (!m_bReadOnly && EndDragging(point))
		return;
	
	CCalendarCtrl::OnLButtonUp(nFlags, point);
}

BOOL CTaskCalendarCtrl::EndDragging(const CPoint& ptCursor)
{
	if (IsDragging())
	{
		TASKCALITEM* pTCI = GetTaskCalItem(m_dwSelectedTaskID);
		ASSERT(pTCI);

		// dropping outside the calendar is a cancel
		CRect rClient;
		GetClientRect(rClient);
		rClient.top += CALENDAR_HEADER_HEIGHT;

		TCC_HITTEST nDragWhat = TCCHT_NOWHERE;

		if (!rClient.PtInRect(ptCursor) || !IsValidDrag(ptCursor))
		{
			*pTCI = m_tciPreDrag;
		}
		else if (m_bDraggingStart)
		{
			nDragWhat = TCCHT_BEGIN;
		}
		else if (m_bDraggingEnd)
		{
			nDragWhat = TCCHT_END;
		}
		else
		{
			ASSERT(m_bDragging);

			// if the start is calculated, treat this like an end move
			if (!pTCI->IsStartDateSet())
			{
				nDragWhat = TCCHT_END;
			}
			else if (!pTCI->IsEndDateSet())
			{
				nDragWhat = TCCHT_BEGIN;
			}
			else
			{
				ASSERT(pTCI->IsStartDateSet() && pTCI->IsEndDateSet());
				nDragWhat = TCCHT_MIDDLE;
			}
		}

		// cleanup
		m_bDraggingStart = m_bDraggingEnd = m_bDragging = FALSE;

		//李波  拖拽完成，更新设备信息
		//得到拖放单元格的设备信息
		CString sSelectedEquipmentID;
		GetSelectedTaskEquipmentID(sSelectedEquipmentID);
		//设置选中任务的设备信息
		if (sSelectedEquipmentID.GetLength()>0 )
			pTCI->SetTaskEquipmentTo(sSelectedEquipmentID);


		ReleaseCapture();
		Invalidate(FALSE);

		// keep parent informed
		NotifyParentDateChange(nDragWhat);
		NotifyParentDragChange();

		return TRUE;
	}

	// else
	return FALSE;
}

BOOL CTaskCalendarCtrl::GetSelectedTaskDates(COleDateTime& dtStart, COleDateTime& dtDue) const
{
	if ((m_dwSelectedTaskID == 0) || !HasTask(m_dwSelectedTaskID))
		return FALSE;

	TASKCALITEM* pTCI = GetTaskCalItem(m_dwSelectedTaskID);
	ASSERT(pTCI);

	if (!pTCI)
		return FALSE;

	dtStart = pTCI->GetAnyStartDate();
	dtDue = pTCI->GetAnyEndDate();

	// handle END_OF_DAY
	if (CDateHelper::GetTimeOnly(dtDue).m_dt >= END_OF_DAY)
	{
		dtDue = CDateHelper::GetDateOnly(dtDue);
	}

	return TRUE;
}

BOOL CTaskCalendarCtrl::GetSelectedTaskEquipmentID(CString & sEquipmentID) const
{
	sEquipmentID = "";
	if ((m_dwSelectedTaskID == 0) || !HasTask(m_dwSelectedTaskID))
		return FALSE;

	TASKCALITEM* pTCI = GetTaskCalItem(m_dwSelectedTaskID);
	ASSERT(pTCI);

	if (!pTCI)
		return FALSE;
 

	sEquipmentID = GetMouseUpCellEquipment(); // pTCI->GetTaskEquipmentTo();
 
	return TRUE;
}


BOOL CTaskCalendarCtrl::GetDateFromPoint(const CPoint& ptCursor, COleDateTime& date) const
{
	int nRow, nCol;

	if (GetGridCellFromPoint(ptCursor, nRow, nCol))
	{
		const CCalendarCell* pCell = GetCell(nRow, nCol);
		ASSERT(pCell);

		// calc proportion of day by 'x' coordinate
		CRect rCell;
		if ((GetCellRect(nRow, nCol, rCell)) == false)
		{
			return false;
		}

		double dTime = ((ptCursor.x - rCell.left) / (double)rCell.Width());
		date = pCell->date.m_dt + dTime;

		return TRUE;
	}

	// all else
	return FALSE;
}

BOOL CTaskCalendarCtrl::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
{
	//李波暂时注释 this is only for when we are NOT yet dragging
	//ASSERT(!IsDragging());

	if (!m_bReadOnly &&
		!m_bSelectionStarted && 
		(nHitTest == HTCLIENT) && 
		(message == WM_MOUSEMOVE) &&
		!Misc::IsKeyPressed(VK_MBUTTON) &&
		!Misc::IsKeyPressed(VK_RBUTTON))
	{
		CPoint ptCursor = CWnd::GetCurrentMessage()->pt;
		ScreenToClient(&ptCursor);

		TCC_HITTEST nHit = TCCHT_NOWHERE;
		DWORD dwHitID = HitTest(ptCursor, nHit);

		switch (nHit)
		{
		case TCCHT_BEGIN:
		case TCCHT_END:
			SetCursor(AfxGetApp()->LoadStandardCursor(IDC_SIZEWE));
			return TRUE;

		case TCCHT_MIDDLE:
			//SetCursor(AfxGetApp()->LoadStandardCursor(IDC_SIZEALL));
			break;
		}
	}
	
	// else
	return CCalendarCtrl::OnSetCursor(pWnd, nHitTest, message);
}

BOOL CTaskCalendarCtrl::IsDragging() const
{
	return (m_bDragging || m_bDraggingStart || m_bDraggingEnd);
}

BOOL CTaskCalendarCtrl::IsValidDrag(const CPoint& ptDrag) const
{
	if (!IsDragging())
		return FALSE;

	CSize size = (m_ptDragOrigin - ptDrag);

	int nCxDrag = (GetSystemMetrics(SM_CXDRAG) / 2);
	int nCyDrag = (GetSystemMetrics(SM_CYDRAG) / 2);

	return ((abs(size.cx) > nCxDrag) || 
			(abs(size.cy) > nCyDrag));
}

BOOL CTaskCalendarCtrl::ValidateDragPoint(CPoint& ptDrag) const
{
	if (!IsDragging())
		return FALSE;

	CRect rClient;
	GetClientRect(rClient);

	rClient.top += CALENDAR_HEADER_HEIGHT;

	ptDrag.x = max(ptDrag.x, rClient.left);
	ptDrag.x = min(ptDrag.x, rClient.right);
	ptDrag.y = max(ptDrag.y, rClient.top);
	ptDrag.y = min(ptDrag.y, rClient.bottom);

	return TRUE;
}

void CTaskCalendarCtrl::OnCaptureChanged(CWnd *pWnd) 
{
	// if someone else grabs the capture we cancel any drag
	if (pWnd && (pWnd != this) && IsDragging())
		CancelDrag(FALSE);
	
	CCalendarCtrl::OnCaptureChanged(pWnd);
}

void CTaskCalendarCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	switch (nChar)
	{
	case VK_ESCAPE:
		if (IsDragging())
		{
			CancelDrag(TRUE);
			return;
		}
		break;

	case VK_CONTROL:
	case VK_SHIFT:
		if (IsDragging())
			NotifyParentDragChange();
		break;

	case VK_DOWN:
	{
		int nRow = GetSelectedCellRow();

		if (IsRowScrollBarActive(nRow))
		{
			SendMessage(WM_VSCROLL, SB_LINEDOWN, (LPARAM)m_sbRowVScroll[nRow].GetSafeHwnd());
			return;
		}
	}
		break;

	case VK_UP:
	{
		int nRow = GetSelectedCellRow();

		if (IsRowScrollBarActive(nRow))
		{
			SendMessage(WM_VSCROLL, SB_LINEUP, (LPARAM)m_sbRowVScroll[nRow].GetSafeHwnd());
			return;
		}
	}
		break;
	}
	
	CCalendarCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
}

BOOL CTaskCalendarCtrl::IsRowScrollBarActive(int nRow) const
{
	if ((nRow < 0) || (nRow >= CALENDAR_MAX_ROWS))
		return FALSE;

	return (m_sbRowVScroll[nRow].GetSafeHwnd() &&
			m_sbRowVScroll[nRow].IsWindowVisible());
}

// external version
BOOL CTaskCalendarCtrl::CancelDrag()
{
	if (IsDragging())
	{
		CancelDrag(TRUE);
		return TRUE;
	}

	// else
	return FALSE;
}

// internal version
void CTaskCalendarCtrl::CancelDrag(BOOL bReleaseCapture)
{
	ASSERT(IsDragging());

	// cancel drag, restoring original task dates
	TASKCALITEM* pTCI = GetTaskCalItem(m_dwSelectedTaskID);
	ASSERT(pTCI);
	
	*pTCI = m_tciPreDrag;
	m_bDragging = m_bDraggingStart = m_bDraggingEnd = FALSE;
	
	if (bReleaseCapture)
		ReleaseCapture();

	Invalidate(FALSE);
	UpdateWindow();

	// keep parent informed
	NotifyParentDragChange();
}

void CTaskCalendarCtrl::OnRButtonDown(UINT nFlags, CPoint point) 
{
	DWORD dwTaskID = HitTest(point);
	SelectTask(dwTaskID, TRUE);
	
	CCalendarCtrl::OnRButtonDown(nFlags, point);
}

int CTaskCalendarCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CCalendarCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;

	EnableToolTips(TRUE);

	return 0;
}

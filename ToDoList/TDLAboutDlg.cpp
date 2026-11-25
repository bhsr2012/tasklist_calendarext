// TDLAboutDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "TDLAboutDlg.h"

#include "..\Shared\localizer.h"
#include "..\Shared\misc.h"
#include "..\Shared\filemisc.h"
#include "..\Shared\themed.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////

#ifndef LVS_EX_LABELTIP
#define LVS_EX_LABELTIP     0x00004000
#endif

/////////////////////////////////////////////////////////////////////////////

LPCTSTR CTDLAboutDlg::EPL_URL      = _T("https://www.eclipse.org/legal/epl-v10.html");
LPCTSTR CTDLAboutDlg::SOURCE_URL   = _T("https://github.com/bhsr2012/tasklist_calendarext");
LPCTSTR CTDLAboutDlg::LICENSE_FILE = _T("LICENSE-EPL-1.0.txt");

static LPCTSTR ABOUTCONTRIBUTION = 

_T("AWIcons\tOriginal toolbar icons\n")
_T("Abin\tIni class for tools importing\n")
_T("Alex Cohn\tMenu icon code\n")
_T("Alex Hazanov\tMSXML wrapper classes\n")
_T("Alex Sidorsky\tRussian translation\n")
_T("Armen Hakobyan\tEnhanced folder dialog\n")
_T("Atlang\tChinese translation\n")
_T("Bill Switzer\tWiki Editor\n")
_T("d3m0n\tCalendar Plugin and Testing\n")
_T("Dave Thompson\tScreencasting\n")
_T("floyd\tpart of the System Tray code\n")
_T("Gianni Carrozzo\tItalian translation\n")
_T("H.Hirasawa\tJapanese Translation\n")
_T("Hans Dietrich\tHTML control\n")
_T("Hugo Tant\tDutch translation\n")
_T("Isaac Aparicio\tSpanish translation\n")
_T("James Fancy\tChinese translation\n")
_T("Jochen Meyer\tfor his myriad contributions\n")
_T("Johan Rosengren\tOriginal RTF comments control\n")
_T("Martin Seibert\tGerman translation\n")
_T("Open Office\tSpellcheck Engine and Dictionaries\n")
_T("Patchou\tRichEdit border theming\n")
_T("Patrice Torchet\tFrench translation\n")
_T("Paul DiLascia (RIP)\tof course\n")
_T("Paul S. Vickery\tStatusbar\n")
_T("Peter Glasl\tGerman translation\n")
_T("Richard Butler\tEncryption\n")
_T("Ryan Sterzer\tLinux support\n")
_T("Valik\tPlain Text importer\n");

/////////////////////////////////////////////////////////////////////////////
// CTDLAboutDlg dialog

CTDLAboutDlg::CTDLAboutDlg(const CString& sAppTitle, CWnd* pParent /*=NULL*/)
	: CDialog(IDD_ABOUT_DIALOG, pParent)
{
	//{{AFX_DATA_INIT(CTDLAboutDlg)
	//}}AFX_DATA_INIT
	CString sEplURL;
	
	sEplURL.Format(_T("\"%s\""), EPL_URL);
	
	m_sTitle.Format(_T("%s"), sAppTitle);
	m_sLicense.Format(IDS_LICENSE, sEplURL, SOURCE_URL);
	m_sThirdParty.Format(IDS_ABOUT_PARTIES);
	
	CLocalizer::IgnoreString(m_sTitle);
	CLocalizer::IgnoreString(m_sLicense);
	CLocalizer::IgnoreString(m_sThirdParty);
	CLocalizer::IgnoreString(ABOUTCONTRIBUTION);
}


void CTDLAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTDLAboutDlg)
	DDX_Control(pDX, IDC_TITLE, m_stTitle);
	DDX_Control(pDX, IDC_LICENSE, m_stLicense);
	DDX_Control(pDX, IDC_3RDPARTY, m_stThirdParty);
	DDX_Control(pDX, IDC_CONTRIBUTORS, m_lcContributors);
	DDX_Text(pDX, IDC_TITLE, m_sTitle);
	DDX_Text(pDX, IDC_LICENSE, m_sLicense);
	DDX_Text(pDX, IDC_3RDPARTY, m_sThirdParty);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTDLAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CTDLAboutDlg)
	ON_BN_CLICKED(IDC_OPENLICENSE, OnOpenLicenseFile)
	ON_BN_CLICKED(IDC_OPENSOURCE, OnOpenSourcePage)
	ON_BN_CLICKED(IDC_COPYINFO, OnCopyInfo)
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTDLAboutDlg message handlers

BOOL CTDLAboutDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	CRect rList;
	m_lcContributors.GetClientRect(rList);
	rList.right -= GetSystemMetrics(SM_CXVSCROLL);
	
	m_lcContributors.InsertColumn(0, _T(""), LVCFMT_LEFT, (rList.Width() * 2) / 5);
	m_lcContributors.InsertColumn(1, _T(""), LVCFMT_LEFT, (rList.Width() * 3) / 5);
	
	CStringArray aRows;
	int nRows = Misc::Split(ABOUTCONTRIBUTION, aRows, '\n', TRUE);
	
	for (int nRow = 0; nRow < nRows; nRow++)
	{
		CStringArray aCols;
		int nCols = Misc::Split(aRows[nRow], aCols, '\t', TRUE);
		
		if (!nCols)
			continue;
		
		int nIndex = m_lcContributors.InsertItem(nRow, aCols[0]);
		
		if (nCols >= 2)
			m_lcContributors.SetItemText(nIndex, 1, aCols[1]);
	}
	
	// dummy imagelist to increase row height
	if (m_il.Create(1, 16, ILC_COLOR, 1, 1))
		m_lcContributors.SetImageList(&m_il, LVSIL_SMALL);
	
	ListView_SetExtendedListViewStyleEx(m_lcContributors, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
	ListView_SetExtendedListViewStyleEx(m_lcContributors, LVS_EX_LABELTIP, LVS_EX_LABELTIP);

	CThemed::SetWindowTheme(&m_lcContributors, _T("Explorer"));

	COLORREF crBk = GetSysColor(COLOR_3DFACE);
	
	m_stTitle.SetBkColor(crBk);
	m_stLicense.SetBkColor(crBk);
	m_stThirdParty.SetBkColor(crBk);

	GetDlgItem(IDOK)->SetFocus();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CTDLAboutDlg::OpenLicenseFile(CWnd* pParent)
{
	CString sPath = FileMisc::GetAppFolder();
	sPath += _T("\\");
	sPath += LICENSE_FILE;
	
	if (FileMisc::FileExists(sPath))
	{
		FileMisc::Run(pParent->GetSafeHwnd(), sPath);
		return TRUE;
	}
	
	// 未随程序安装则退回在线许可全文
	FileMisc::Run(pParent->GetSafeHwnd(), EPL_URL);
	return FALSE;
}

void CTDLAboutDlg::OpenSourcePage(CWnd* pParent)
{
	FileMisc::Run(pParent->GetSafeHwnd(), SOURCE_URL);
}

void CTDLAboutDlg::OnOpenLicenseFile()
{
	OpenLicenseFile(this);
}

void CTDLAboutDlg::OnOpenSourcePage()
{
	OpenSourcePage(this);
}

void CTDLAboutDlg::OnCopyInfo()
{
	CString sText;
	
	sText.Format(_T("TaskList（基于 ToDoList 7.0.11，Copyright (c) 2003-2015 AbstractSpoon Software）\r\n")
				 _T("许可：Eclipse Public License 1.0\r\n")
				 _T("本版本含修改，修改文件的完整源代码：%s\r\n")
				 _T("许可全文：%s\r\n")
				 _T("安装目录内另附：LICENSE-EPL-1.0.txt、NOTICE.md\r\n")
				 _T("本程序按原样提供，不附带任何担保。\r\n"),
				 SOURCE_URL, EPL_URL);
	
	if (!::OpenClipboard(GetSafeHwnd()))
		return;
	
	::EmptyClipboard();
	
	int nChars = sText.GetLength();
	HGLOBAL hMem = ::GlobalAlloc(GMEM_MOVEABLE, (nChars + 1) * sizeof(TCHAR));
	
	if (hMem)
	{
		LPTSTR psz = (LPTSTR)::GlobalLock(hMem);
		
		if (psz)
		{
			memcpy(psz, (LPCTSTR)sText, nChars * sizeof(TCHAR));
			psz[nChars] = _T('\0');
			::GlobalUnlock(hMem);
			
			::SetClipboardData(CF_UNICODETEXT, hMem);
			hMem = NULL;	// 所有权已移交系统
		}
		
		if (hMem)
			::GlobalFree(hMem);
	}
	
	::CloseClipboard();
	
	SetDlgItemText(IDC_COPYINFO, _T("已复制"));
	SetTimer(1, 1200, NULL);
}

void CTDLAboutDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 1)
	{
		KillTimer(1);
		SetDlgItemText(IDC_COPYINFO, _T("复制信息(&C)"));
		return;
	}
	
	CDialog::OnTimer(nIDEvent);
}

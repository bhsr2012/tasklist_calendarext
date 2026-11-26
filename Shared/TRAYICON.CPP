// TrayIcon.cpp : implementation of the CTrayIcon class
//

#include "stdafx.h"
#include "TrayIcon.h"
#include "autoflag.h"
#include "graphicsmisc.h"

#include <shellapi.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const UINT WM_TINOTIFY = (WM_APP+1);
const UINT WM_TASKBARCREATED = ::RegisterWindowMessage(_T("TaskbarCreated"));

enum
{
	TIMER_ANIMATE = 1,
	TIMER_SINGLECLK,
};
 
#ifndef NIF_STATE
#	define NIF_STATE 0x0008
#	define NIF_INFO  0x0010
#endif

#ifndef NIN_SELECT
#	define NIN_BALLOONSHOW (WM_USER + 2)
#	define NIN_BALLOONHIDE (WM_USER + 3)
#	define NIN_SELECT (WM_USER + 0)
#endif

#ifndef NIN_BALLOONTIMEOUT
#	define NIN_BALLOONTIMEOUT (WM_USER + 4)
#	define NIN_BALLOONUSERCLICK (WM_USER + 5)
#endif

struct NOTIFYICONDATA_TI 
{
	DWORD cbSize;
	HWND hWnd;
	UINT uID;
	UINT uFlags;
	UINT uCallbackMessage;
	HICON hIcon;
	TCHAR szTip[128];
	DWORD dwState;
	DWORD dwStateMask;
	TCHAR szInfo[256];
	UINT uTimeout;
	TCHAR szInfoTitle[64];
	DWORD dwInfoFlags;
};

//////////////////////////////////////////////////////////////////////////
///
// CTrayIcon

BEGIN_MESSAGE_MAP(CTrayIcon, CWnd)
	//{{AFX_MSG_MAP(CTrayIcon)
	ON_WM_DESTROY()
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_TINOTIFY, OnTrayIconNotify)
END_MESSAGE_MAP()

// 托盘位只有 16x16，按系统小图标尺寸加载，避免 32x32 缩放出来的虚边
static HICON LoadTrayIcon(UINT uIDIcon)
{
	int cx = GetSystemMetrics(SM_CXSMICON);
	int cy = GetSystemMetrics(SM_CYSMICON);

	HICON hIcon = (HICON)::LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(uIDIcon), IMAGE_ICON,
									 cx, cy, LR_LOADMAP3DCOLORS);
	if (hIcon == NULL)
		hIcon = AfxGetApp()->LoadIcon(uIDIcon);

	// 资源 ID 若不是 ICON 类型（例如 IDR_MAINFRAME 实际是 MENU），
	// 上面两步都会失败。此时用系统默认图标兜底，绝不注册无图标的空槽位。
	if (hIcon == NULL)
		hIcon = ::LoadIcon(NULL, IDI_APPLICATION);

	return hIcon;
}

//////////////////////////////////////////////////////////////////////////
///
// CTrayIcon construction/destruction

CTrayIcon::CTrayIcon()
{
	m_hIcon = NULL;
	m_bVisible = FALSE;
	memset(&m_nm, 0, sizeof(m_nm));
	m_sTip = _T("");
	m_bAnimationOn = FALSE;
	m_nPrevMsg = 0;
	m_bTemporaryIcon = FALSE;
}

CTrayIcon::~CTrayIcon()
{
}

// this Create takes an ID for the tip text
BOOL CTrayIcon::Create(DWORD dwStyle, CWnd* pParentWnd, UINT uID, UINT uIDIcon, UINT uIDTip)
{
	if (!CWnd::Create(NULL, _T("TrayIcon notification window"), WS_CHILD,
		CRect(0,0,0,0), pParentWnd, uID))
		return FALSE;

	m_hIcon = LoadTrayIcon(uIDIcon);
	m_sTip.LoadString(uIDTip);

	m_nm.hdr.hwndFrom = GetSafeHwnd();
	m_nm.hdr.idFrom = GetDlgCtrlID();

	if (dwStyle & WS_VISIBLE)
		ShowTrayIcon();

	return TRUE;
}

// this create takes a text string for the ti text
BOOL CTrayIcon::Create(DWORD dwStyle, CWnd* pParentWnd, UINT uID, UINT uIDIcon, LPCTSTR sTip)
{
	if (!CWnd::Create(NULL, _T("TrayIcon notification window"), WS_CHILD,
		CRect(0,0,0,0), pParentWnd, uID))
		return FALSE;

	m_hIcon = LoadTrayIcon(uIDIcon);
	m_sTip = sTip;

	m_nm.hdr.hwndFrom = GetSafeHwnd();
	m_nm.hdr.idFrom = GetDlgCtrlID();

	if (dwStyle & WS_VISIBLE)
		ShowTrayIcon();

	return TRUE;
}

void CTrayIcon::ShowTrayIcon(BOOL bShow /*=TRUE*/)
{
	if (bShow && !m_bVisible)
	{
		AddToTray();
		m_bVisible = TRUE;
	}
	else if (!bShow && m_bVisible)
	{
		DeleteFromTray();
		m_bVisible = FALSE;
	}
}

//////////////////////////////////////////////////////////////////////////
///
// CTrayIcon message handlers

void CTrayIcon::OnDestroy() 
{
	CWnd::OnDestroy();

	// 无条件撤图标：m_bVisible 只是缓存，可能与通知区的实际状态不同步
	// （图标可能是绕过 ShowTrayIcon 直接补上的）。这里若跳过，
	// 进程退出后就会在通知区留下一个空槽位。
	DeleteFromTray();
	m_bVisible = FALSE;
}

LRESULT CTrayIcon::OnTrayIconNotify(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(wParam);

	// prevent reentrancy
	static BOOL bInNotify = FALSE;

	if (bInNotify || !GetParent()->IsWindowEnabled())
		return 0L;

	CAutoFlag af(bInNotify, TRUE);
	BOOL bNotify = TRUE;

	GetCursorPos(&m_nm.ptAction);

	switch (lParam)
	{
	case WM_MOUSEMOVE:
	case WM_LBUTTONDOWN:
		bNotify = FALSE;
		break;

	case WM_LBUTTONUP:
		if (m_nPrevMsg == WM_LBUTTONDOWN) // start a timer to test for double click
		{
			UINT nDelay = GetDoubleClickTime();
			SetTimer(TIMER_SINGLECLK, nDelay, NULL);
		}

		bNotify = FALSE; // we'll handle it in OnTimer
		break;

	case WM_LBUTTONDBLCLK:
		// if we got here then the timer has not yet been tripped so it's a double click
		if (m_nPrevMsg == WM_LBUTTONUP)
		{
			KillTimer(TIMER_SINGLECLK);
			m_nm.hdr.code = NM_DBLCLK;
		}
		break;

	case WM_RBUTTONDOWN:
		bNotify = FALSE;
		break;

	case WM_RBUTTONUP:
		if (m_nPrevMsg == WM_RBUTTONDOWN)
			m_nm.hdr.code = NM_RCLICK;
		else
			bNotify = FALSE;
		break;

	case WM_RBUTTONDBLCLK:
		m_nm.hdr.code = NM_RDBLCLK;
		break;

	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MBUTTONDBLCLK:
		bNotify = FALSE;
		break;

	case NIN_BALLOONUSERCLICK:
	case NIN_BALLOONTIMEOUT:
		if (m_bTemporaryIcon)
		{
			m_bTemporaryIcon = FALSE;
			ShowTrayIcon(FALSE);
		}
		bNotify = FALSE;
		break;

	default:
		bNotify = FALSE;
		break;
	}

	if (lParam != WM_MOUSEMOVE)
		m_nPrevMsg = lParam;

	if (!bNotify)
		return TRUE;

	LRESULT lr = GetParent()->SendMessage(WM_NOTIFY, GetDlgCtrlID(),
									(LPARAM)&m_nm);

	return lr;
}

BOOL CTrayIcon::AddToTray()
{
	NOTIFYICONDATA_TI nid;
	memset(&nid, 0, sizeof(nid));

	nid.cbSize = sizeof(nid);
	nid.hWnd = GetSafeHwnd();
	nid.uID = GetDlgCtrlID();

	// 同一身份先撤再建：这样"补图标"永远不会变成"再加一个图标"，
	// 也不会因为上一次非正常退出留下的同身份槽位而注册失败。
	::Shell_NotifyIcon(NIM_DELETE, (PNOTIFYICONDATA)&nid);

	nid.uFlags = NIF_MESSAGE | NIF_ICON;
	nid.uCallbackMessage = WM_TINOTIFY;
	nid.hIcon = m_hIcon;

	if (!m_sTip.IsEmpty())
	{
		nid.uFlags |= NIF_TIP;
		//fabio_2005
#if _MSC_VER >= 1400
		_tcsncpy_s(nid.szTip, (LPTSTR)(LPCTSTR)m_sTip, sizeof(nid.szTip)/sizeof(TCHAR));
#else
		_tcsncpy(nid.szTip, (LPTSTR)(LPCTSTR)m_sTip, sizeof(nid.szTip)/sizeof(TCHAR));
#endif
		
		nid.szTip[sizeof(nid.szTip)/sizeof(TCHAR)-1] = (TCHAR)0;
	}

	// create top level parent hook first time around
	if (!ScIsHooked())
	{
		CWnd* pTLParent = GetTopLevelParent();

		if (pTLParent)
			ScHookWindow(pTLParent->GetSafeHwnd());
	}

	return Shell_NotifyIcon(NIM_ADD, (PNOTIFYICONDATA)&nid);
}

BOOL CTrayIcon::DeleteFromTray()
{
	NOTIFYICONDATA_TI nid;
	memset(&nid, 0, sizeof(nid));

	nid.cbSize = sizeof(nid);
	nid.hWnd = GetSafeHwnd();
	nid.uID = GetDlgCtrlID();

	return Shell_NotifyIcon(NIM_DELETE, (PNOTIFYICONDATA)&nid);
}

BOOL CTrayIcon::ModifyIcon(UINT uIDNewIcon)
{
	return ModifyIcon(GraphicsMisc::LoadIcon(uIDNewIcon));
}

BOOL CTrayIcon::ModifyIcon(HICON hNewIcon)
{
	NOTIFYICONDATA_TI nid;

	nid.cbSize = sizeof(nid);
	nid.hWnd = GetSafeHwnd();
	nid.uID = GetDlgCtrlID();
	nid.uFlags = NIF_ICON;
	nid.hIcon = m_hIcon = hNewIcon;

	return Shell_NotifyIcon(NIM_MODIFY, (PNOTIFYICONDATA)&nid);
}

BOOL CTrayIcon::ModifyTip(UINT uIDNewTip)
{
	return ModifyTip(CString(MAKEINTRESOURCE(uIDNewTip)));
}

BOOL CTrayIcon::ModifyTip(LPCTSTR sNewTip)
{
	if (sNewTip == m_sTip)
		return TRUE;

	NOTIFYICONDATA_TI nid;

	nid.cbSize = sizeof(nid);
	nid.hWnd = GetSafeHwnd();
	nid.uID = GetDlgCtrlID();
	nid.uFlags = 0;

	m_sTip = sNewTip;

	if (!m_sTip.IsEmpty())
	{
		nid.uFlags |= NIF_TIP;
//fabio_2005
#if _MSC_VER >= 1300
		_tcsncpy_s(nid.szTip, (LPTSTR)(LPCTSTR)m_sTip, sizeof(nid.szTip)/sizeof(TCHAR));
#else
		_tcsncpy(nid.szTip, (LPTSTR)(LPCTSTR)m_sTip, sizeof(nid.szTip)/sizeof(TCHAR));
#endif
		nid.szTip[sizeof(nid.szTip)/sizeof(TCHAR)-1] = (TCHAR)0;
	}

	return Shell_NotifyIcon(NIM_MODIFY, (PNOTIFYICONDATA)&nid);
}

BOOL CTrayIcon::SetIcon(UINT uIDNewIcon)
{
	if (m_bAnimationOn)
		StopAnimation();

	return ModifyIcon(uIDNewIcon);
}

BOOL CTrayIcon::SetIcon(HICON hNewIcon)
{
	if (m_bAnimationOn)
		StopAnimation();
	
	return ModifyIcon(hNewIcon);
}

BOOL CTrayIcon::SetTip(UINT uIDNewTip)
{
	return ModifyTip(uIDNewTip);
}

BOOL CTrayIcon::SetTip(LPCTSTR sNewTip)
{
	return ModifyTip(sNewTip);
}

void CTrayIcon::StartAnimation()
{
	ASSERT (m_aAnimationIconIDs.GetSize() && m_nAnimationDelay >= 100);

	m_nCurIcon = 0; // reset animation
	ModifyIcon(m_aAnimationIconIDs[m_nCurIcon]);

	SetTimer(TIMER_ANIMATE, m_nAnimationDelay, NULL);
	m_bAnimationOn = TRUE;
}

void CTrayIcon::StopAnimation()
{
	if (!m_bAnimationOn)
		return;

	KillTimer(1);
	m_bAnimationOn = FALSE;
	
	// reset animation
	ModifyIcon(m_aAnimationIconIDs[0]);
}

void CTrayIcon::OnTimer(UINT_PTR nIDEvent)
{
	switch (nIDEvent)
	{
	case TIMER_ANIMATE:
		m_nCurIcon++;
		m_nCurIcon %= m_aAnimationIconIDs.GetSize();

		ModifyIcon(m_aAnimationIconIDs[m_nCurIcon]);
		break;

	case TIMER_SINGLECLK:
		// if we got here then the double click did not happen
		// so we can issue a single click
		KillTimer(TIMER_SINGLECLK);
		m_nPrevMsg = 0;
		
		m_nm.hdr.code = NM_CLICK;
		GetParent()->SendMessage(WM_NOTIFY, GetDlgCtrlID(),	(LPARAM)&m_nm);
		break;
	}

	CWnd::OnTimer(nIDEvent);
}

void CTrayIcon::SetAnimationIcons(UINT pIconIDs[], int nNumIcons)
{
	ASSERT (pIconIDs != NULL && nNumIcons > 1);

	m_aAnimationIconIDs.SetSize(nNumIcons);

	while (nNumIcons--)
		m_aAnimationIconIDs.SetAt(nNumIcons, pIconIDs[nNumIcons]);
}

void CTrayIcon::SetAnimationDelay(int nDelay)
{
	ASSERT (nDelay >= 100);

	m_nAnimationDelay = max(nDelay, 100);

	if (m_bAnimationOn)
	{
		KillTimer(1);
		StartAnimation();
	}
}

LRESULT CTrayIcon::ScWindowProc(HWND hRealWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	if (msg == WM_TASKBARCREATED && m_bVisible)
	{
		AddToTray();
	}

	return CSubclasser::ScWindowProc(hRealWnd, msg, wp, lp); 
}

BOOL CTrayIcon::ShowBalloon(LPCTSTR szText, LPCTSTR szTitle, DWORD dwIcon, UINT uTimeout)
{
    // Verify input parameters.
	if (uTimeout <= 0)
		return FALSE;

    // The balloon tooltip text can be up to 255 chars long.
    ASSERT(AfxIsValidString(szText));
    ASSERT(lstrlen(szText) < 256);

    // The balloon title text can be up to 63 chars long.
    if (szTitle)
    {
        ASSERT(AfxIsValidString( szTitle));
        ASSERT(lstrlen(szTitle) < 64);
    }

    // dwBalloonIcon must be valid.
    ASSERT(NIIF_NONE == dwIcon    || NIIF_INFO == dwIcon ||
           NIIF_WARNING == dwIcon || NIIF_ERROR == dwIcon);

    // The timeout must be between 10 and 30 seconds.
    uTimeout = min(max(uTimeout, 10), 30);

	NOTIFYICONDATA_TI nid;

	nid.cbSize = sizeof(nid);
	nid.hWnd = GetSafeHwnd();
	nid.uID = GetDlgCtrlID();
	nid.uFlags = NIF_INFO;

//fabio_2005
#if _MSC_VER >= 1300
    _tcsncpy_s(nid.szInfo, szText, 256);

    if (szTitle)
        _tcsncpy_s(nid.szInfoTitle, szTitle, 64);
#else
    _tcsncpy(nid.szInfo, szText, 256);

    if (szTitle)
        _tcsncpy(nid.szInfoTitle, szTitle, 64);
#endif

    else
        nid.szInfoTitle[0] = _T('\0');

    nid.dwInfoFlags = dwIcon;
    nid.uTimeout = uTimeout * 1000;   // convert time to ms

	// if the icon is not showing then show it temporarily
	if (!m_bVisible)
	{
		m_bTemporaryIcon = TRUE;
		ShowTrayIcon(TRUE);
	}

    return Shell_NotifyIcon(NIM_MODIFY, (PNOTIFYICONDATA)&nid);
}

//////////////////////////////////////////////////////////////////////////
// 托盘槽位：一律用 (hWnd,uID) 作身份
// GUID 模式实测不可用：同一 GUID 二次 NIM_ADD 返回 E_FAIL，别的进程又删不掉该槽位，
// 一次非正常退出就把 GUID 永久占死，之后每次注册失败再退回 uID 加了第二个，只增不减。

BOOL CTrayIcon::IsInTray()
{
	if (!::IsWindow(GetSafeHwnd()))
		return FALSE;

	NOTIFYICONDATA_TI nid;
	memset(&nid, 0, sizeof(nid));

	nid.cbSize = sizeof(nid);
	nid.hWnd = GetSafeHwnd();
	nid.uID = GetDlgCtrlID();

	if (!m_sTip.IsEmpty())
	{
		nid.uFlags = NIF_TIP;
		_tcsncpy_s(nid.szTip, _countof(nid.szTip), (LPCTSTR)m_sTip, _TRUNCATE);
	}

	if (::Shell_NotifyIcon(NIM_MODIFY, (PNOTIFYICONDATA)&nid))
		return TRUE;

	// 只有在"记录确实不存在"时才判死，其它错误不判死，避免反复重加
	DWORD dwErr = ::GetLastError();
	if (dwErr == ERROR_TIMEOUT || dwErr == ERROR_NOT_FOUND || dwErr == ERROR_INVALID_WINDOW_HANDLE)
		return FALSE;

	return TRUE;
}

BOOL CTrayIcon::RefreshTrayIcon()
{
	if (!::IsWindow(GetSafeHwnd()))
		return FALSE;

	DeleteFromTray();
	BOOL bOK = AddToTray();
	m_bVisible = bOK;
	return bOK;
}

BOOL TrayDeleteSlot(HWND hWnd, UINT uID)
{
	if (hWnd == NULL || uID == 0)
		return FALSE;

	NOTIFYICONDATA_TI nid;
	memset(&nid, 0, sizeof(nid));

	nid.cbSize = sizeof(nid);
	nid.hWnd = hWnd;
	nid.uID = uID;

	return (::Shell_NotifyIcon(NIM_DELETE, (PNOTIFYICONDATA)&nid) != FALSE);
}

BOOL TraySlotExists(HWND hWnd, UINT uID)
{
	if (hWnd == NULL || uID == 0)
		return FALSE;

	NOTIFYICONDATA_TI nid;
	memset(&nid, 0, sizeof(nid));

	nid.cbSize = sizeof(nid);
	nid.hWnd = hWnd;
	nid.uID = uID;
	nid.uFlags = NIF_TIP;
	_tcsncpy_s(nid.szTip, _countof(nid.szTip), _T("任务调度"), _TRUNCATE);

	return (::Shell_NotifyIcon(NIM_MODIFY, (PNOTIFYICONDATA)&nid) != FALSE);
}

// explorer 通知区 toolbar 的按钮结构。注意两种位宽要分开：
// 32 位程序也会遇到 64 位 explorer，按目标进程的实际位宽解析，不能用自己的 sizeof。
#pragma pack(push, 1)
struct TRAY_TB_BUTTON64
{
	int iBitmap; int idCommand;
	BYTE fsState; BYTE fsStyle; BYTE bReserved[6];
	unsigned __int64 dwData; unsigned __int64 iString;
};
struct TRAY_TB_BUTTON32
{
	int iBitmap; int idCommand;
	BYTE fsState; BYTE fsStyle; BYTE bReserved[2];
	unsigned __int32 dwData; unsigned __int32 iString;
};
#pragma pack(pop)

#define TRAY_TB_BUTTONCOUNT	0x0418
#define TRAY_TB_GETBUTTON	0x0417

// 在按钮内部块里找 exe 名：宽字符（通知区存的是 UTF-16LE 路径）与 ANSI 两种都试
static BOOL TrayBlkHasName(const BYTE* p, int n, LPCTSTR pszExe)
{
	int nLen = (int)_tcslen(pszExe);
	if (p == NULL || nLen <= 0 || n <= 0)
		return FALSE;

	for (int i = 0; i + nLen * (int)sizeof(WCHAR) <= n; i++)
	{
		BOOL bSame = TRUE;
		for (int k = 0; k < nLen; k++)
		{
			WCHAR c1 = 0, c2 = (WCHAR)pszExe[k];
			memcpy(&c1, p + i + k * (int)sizeof(WCHAR), sizeof(WCHAR));
			if (c1 >= L'a' && c1 <= L'z') c1 = (WCHAR)(c1 - 32);
			if (c2 >= L'a' && c2 <= L'z') c2 = (WCHAR)(c2 - 32);
			if (c1 != c2) { bSame = FALSE; break; }
		}
		if (bSame)
			return TRUE;
	}

	for (int i = 0; i + nLen <= n; i++)
	{
		BOOL bSame = TRUE;
		for (int k = 0; k < nLen; k++)
		{
			char c1 = (char)p[i + k], c2 = (char)pszExe[k];
			if (c1 >= 'a' && c1 <= 'z') c1 = (char)(c1 - 32);
			if (c2 >= 'a' && c2 <= 'z') c2 = (char)(c2 - 32);
			if (c1 != c2) { bSame = FALSE; break; }
		}
		if (bSame)
			return TRUE;
	}

	return FALSE;
}

// 给目标进程分配远程缓冲。32 位程序必须申请 4GB 以下的地址：
// SendMessage 的 LPARAM 是 32 位，高位会被截断，explorer 就写不到我们能读的地方。
static LPVOID TrayAllocRemote(HANDLE hp, SIZE_T cb)
{
#ifdef _WIN64
	return ::VirtualAllocEx(hp, NULL, cb, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
	static const ULONG_PTR aBase[] = { 0x10000000, 0x20000000, 0x30000000, 0x08000000 };
	for (int i = 0; i < _countof(aBase); i++)
	{
		LPVOID p = ::VirtualAllocEx(hp, (LPVOID)aBase[i], cb, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		if (p != NULL)
			return p;
	}

	// 固定地址可能全被 explorer 自己占用（实测 0x10000000 已占用），
	// 此时让系统自选地址，只要仍落在低 4GB 就能用 32 位 LPARAM 传进去
	LPVOID pAny = ::VirtualAllocEx(hp, NULL, cb, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	if (pAny != NULL)
	{
		if (((ULONG_PTR)pAny >> 32) == 0)
			return pAny;

		::VirtualFreeEx(hp, pAny, 0, MEM_RELEASE);
	}

	return NULL;
#endif
}

// 读目标进程内存，地址可能是 64 位。
// 32 位程序读 64 位 explorer 的高位地址时 ReadProcessMemory 会截断地址参数，
// 只有 WOW64 专用的 NtWow64ReadVirtualMemory64 能吃完整的 64 位地址。
typedef LONG (NTAPI *PFN_NTRDVM64)(HANDLE, unsigned __int64, PVOID, unsigned __int64, unsigned __int64*);

static BOOL TrayReadRemote(HANDLE hp, unsigned __int64 addr, LPVOID buf, SIZE_T cb, SIZE_T* pRead)
{
#ifdef _WIN64
	BOOL bOK = (::ReadProcessMemory(hp, (LPCVOID)(ULONG_PTR)addr, buf, cb, pRead) != FALSE);
	if (!bOK && pRead) *pRead = 0;
	return bOK;
#else
	static PFN_NTRDVM64 s_pfn = NULL;
	static BOOL s_bInited = FALSE;

	if (!s_bInited)
	{
		s_bInited = TRUE;
		HMODULE hNtdll = ::GetModuleHandle(_T("ntdll.dll"));
		if (hNtdll != NULL)
			s_pfn = (PFN_NTRDVM64)::GetProcAddress(hNtdll, "NtWow64ReadVirtualMemory64");
	}

	if (s_pfn != NULL)
	{
		unsigned __int64 nRead = 0;
		if (s_pfn(hp, addr, buf, (unsigned __int64)cb, &nRead) >= 0)
		{
			if (pRead) *pRead = (SIZE_T)nRead;
			return TRUE;
		}
		return FALSE;
	}

	if (addr > 0xFFFFFFFFu)
		return FALSE;

	return ::ReadProcessMemory(hp, (LPCVOID)(ULONG_PTR)addr, buf, cb, pRead) != FALSE;
#endif
}

int TrayPurgeByModule(LPCTSTR pszExeName, HWND hKeepSelf)
{
	if (pszExeName == NULL || *pszExeName == 0)
		return 0;

	int nDeleted = 0;

	struct TRAY_SLOT { HWND hWnd; UINT uID; };
	TRAY_SLOT aFound[128];
	int nFound = 0;

	HWND hTray = ::FindWindow(_T("Shell_TrayWnd"), NULL);
	if (hTray == NULL)
		return 0;
	HWND hNotify = ::FindWindowEx(hTray, NULL, _T("TrayNotifyWnd"), NULL);
	if (hNotify == NULL)
		return 0;
	HWND hPager = ::FindWindowEx(hNotify, NULL, _T("SysPager"), NULL);
	if (hPager == NULL)
		hPager = hNotify;

	SYSTEM_INFO si;
	::GetNativeSystemInfo(&si);
	BOOL bNative64 = (si.wProcessorArchitecture != PROCESSOR_ARCHITECTURE_INTEL);

	HWND hTb = ::FindWindowEx(hPager, NULL, _T("ToolbarWindow32"), NULL);

	while (hTb != NULL)
	{
		RECT rc;
		::GetWindowRect(hTb, &rc);

		if (rc.right > rc.left)
		{
			DWORD dwPid = 0;
			::GetWindowThreadProcessId(hTb, &dwPid);
			HANDLE hp = ::OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION,
									  FALSE, dwPid);
			if (hp != NULL)
			{
				BOOL bWow = FALSE;
				::IsWow64Process(hp, &bWow);
				// 目标 explorer 是 32 位吗：原生 32 位系统恒 32 位；64 位系统看 WOW64 标志
				BOOL bTarget32 = bNative64 ? bWow : TRUE;

				LPVOID pRemote = TrayAllocRemote(hp, 1024);
				if (pRemote != NULL)
				{
					LRESULT nCnt = ::SendMessage(hTb, TRAY_TB_BUTTONCOUNT, 0, 0);

					for (int i = 0; (nCnt > 0) && (i < (int)nCnt) && (i < 256); i++)
					{
						if (!::SendMessage(hTb, TRAY_TB_GETBUTTON, (WPARAM)i, (LPARAM)pRemote))
							continue;

						unsigned __int64 dwData = 0;

						if (bTarget32)
						{
							TRAY_TB_BUTTON32 tb;
							memset(&tb, 0, sizeof(tb));
							SIZE_T rd = 0;
							if (!::ReadProcessMemory(hp, pRemote, &tb, sizeof(tb), &rd))
								continue;
							dwData = tb.dwData;
						}
						else
						{
							TRAY_TB_BUTTON64 tb;
							memset(&tb, 0, sizeof(tb));
							SIZE_T rd = 0;
							if (!::ReadProcessMemory(hp, pRemote, &tb, sizeof(tb), &rd))
								continue;
							dwData = tb.dwData;
						}

						if (dwData == 0)
							continue;

						BYTE blk[1024];
						memset(blk, 0, sizeof(blk));
						SIZE_T rd2 = 0;
						if (!TrayReadRemote(hp, dwData, blk, sizeof(blk), &rd2))
							continue;
						if (!TrayBlkHasName(blk, (int)rd2, pszExeName))
							continue;

						HWND hOwner = NULL;
						UINT uID = 0;

						if (bTarget32)
						{
							unsigned __int32 hw = 0;
							memcpy(&hw, blk + 0, 4);
							hOwner = (HWND)(UINT_PTR)hw;
							memcpy(&uID, blk + 4, 4);
						}
						else
						{
							unsigned __int64 hw = 0;
							memcpy(&hw, blk + 0, 8);
							hOwner = (HWND)(ULONG_PTR)hw;
							memcpy(&uID, blk + 8, 4);
						}

						if (hOwner == NULL || hOwner == hKeepSelf)
							continue;

						// 先收集再删除：删除会让后续按钮下标前移，边遍历边删会漏掉一半
						if (nFound < _countof(aFound))
						{
							aFound[nFound].hWnd = hOwner;
							aFound[nFound].uID = uID;
							nFound++;
						}
					}

					::VirtualFreeEx(hp, pRemote, 0, MEM_RELEASE);
				}
				::CloseHandle(hp);
			}
		}

		hTb = ::FindWindowEx(hPager, hTb, _T("ToolbarWindow32"), NULL);
	}

	for (int i = 0; i < nFound; i++)
	{
		if (TrayDeleteSlot(aFound[i].hWnd, aFound[i].uID))
			nDeleted++;
	}

	return nDeleted;
}


// TrayIcon.h : header file for CTrayIcon
//
//////////////////////////////////////////////////////////////////////////

#if !defined ( TRAYICON_H )
#define TRAYICON_H

#include "subclass.h"
#include <afxtempl.h>

#ifndef NIIF_NONE
#	define NIIF_NONE	0x00
#	define NIIF_INFO	0x01
#	define NIIF_WARNING 0x02
#	define NIIF_ERROR	0x03
#endif

struct NM_TRAYICON
{ 
    NMHDR hdr; 
    POINT ptAction; 
};

class CTrayIcon : public CWnd, protected CSubclasser
{
// Construction
public:
	CTrayIcon();
	virtual ~CTrayIcon();
	BOOL Create(DWORD dwStyle, CWnd* pParentWnd, UINT uID, UINT uIDIcon, UINT uIDTip = 0);
	BOOL Create(DWORD dwStyle, CWnd* pParentWnd, UINT uID, UINT uIDIcon, LPCTSTR sTip = _T(""));
	void ShowTrayIcon(BOOL bShow = TRUE);
	void StartAnimation();
	void StopAnimation();
	void SetAnimationIcons(UINT pIconIDs[], int nNumIcons);
	void SetAnimationDelay(int nDelay);
	BOOL SetTip(UINT uIDNewTip);
	BOOL SetTip(LPCTSTR sNewTip);
	BOOL SetIcon(UINT uIDNewIcon);
	BOOL SetIcon(HICON hNewIcon);

	BOOL ShowBalloon(LPCTSTR szText, LPCTSTR szTitle = NULL,
                     DWORD dwIcon = NIIF_NONE, UINT uTimeout = 5);

	// 槽位是否真的还挂在通知区域（NIM_MODIFY 只有记录存在时才会成功）
	BOOL IsInTray();
	// 同一身份先撤后建：补图标永远不会变成"再加一个"
	BOOL RefreshTrayIcon();

// Attributes
private:
	CString m_sTip;
	HICON m_hIcon;
	BOOL m_bVisible;
	NM_TRAYICON m_nm;
	UINT m_nPrevMsg;
	CUIntArray m_aAnimationIconIDs;
	BOOL m_bAnimationOn;
	int m_nAnimationDelay;
	int m_nCurIcon;
	BOOL m_bTemporaryIcon;

// Generated message map functions
protected:
	//{{AFX_MSG(CTrayIcon)
    afx_msg LRESULT OnTrayIconNotify(WPARAM wParam, LPARAM lParam);
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	virtual LRESULT ScWindowProc(HWND hRealWnd, UINT msg, WPARAM wp, LPARAM lp);

private:
	BOOL AddToTray();
	BOOL DeleteFromTray();
	BOOL ModifyTip(UINT uIDNewTip);
	BOOL ModifyTip(LPCTSTR sNewTip);
	BOOL ModifyIcon(UINT uIDNewIcon);
	BOOL ModifyIcon(HICON hNewIcon);

};

// 跨进程撤销一个托盘槽位：对已死窗口同样有效（只要知道它的 (hWnd,uID)）
BOOL TrayDeleteSlot(HWND hWnd, UINT uID);
// 复核某个槽位是否还挂在通知区域
BOOL TraySlotExists(HWND hWnd, UINT uID);
// 按 exe 名清扫通知区里的遗留图标（不区分目录、不管进程死活），hKeepSelf 的槽位保留。
// 返回清掉的个数。零位宽不同的 explorer 也能处理。
int  TrayPurgeByModule(LPCTSTR pszExeName, HWND hKeepSelf);

#endif

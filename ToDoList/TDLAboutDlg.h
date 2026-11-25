#if !defined(AFX_TDLABOUTDLG_H__01356A22_314F_49D5_B39F_A087106574AE__INCLUDED_)
#define AFX_TDLABOUTDLG_H__01356A22_314F_49D5_B39F_A087106574AE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TDLAboutDlg.h : header file
//

#include "..\3rdparty\xhtmlstatic.h"

/////////////////////////////////////////////////////////////////////////////
// CTDLAboutDlg dialog

class CTDLAboutDlg : public CDialog
{
// Construction
public:
	CTDLAboutDlg(const CString& sAppTitle, CWnd* pParent = NULL);   // standard constructor

	// 合规信息入口，主窗口菜单复用
	static BOOL OpenLicenseFile(CWnd* pParent);
	static void OpenSourcePage(CWnd* pParent);

	static LPCTSTR EPL_URL;
	static LPCTSTR SOURCE_URL;
	static LPCTSTR LICENSE_FILE;

protected:
// Dialog Data
	CXHTMLStatic	m_stTitle;
	CXHTMLStatic	m_stLicense;
	CXHTMLStatic	m_stThirdParty;
	CListCtrl		m_lcContributors;
	CString			m_sTitle;
	CString			m_sLicense;
	CString			m_sThirdParty;
	CImageList		m_il;

// Overrides
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

// Implementation
protected:
	afx_msg void OnOpenLicenseFile();
	afx_msg void OnOpenSourcePage();
	afx_msg void OnCopyInfo();
	afx_msg void OnTimer(UINT_PTR nIDEvent);

	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TDLABOUTDLG_H__01356A22_314F_49D5_B39F_A087106574AE__INCLUDED_)

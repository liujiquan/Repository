#include "StreamMedia.h"
// ffmpeg4Dlg.h : header file
//

#pragma once


// Cffmpeg4Dlg dialog
class Cffmpeg4Dlg : public CDialogEx
{
// Construction
public:
	Cffmpeg4Dlg(CWnd* pParent = NULL);	// standard constructor
	~Cffmpeg4Dlg();
// Dialog Data
	enum { IDD = IDD_FFMPEG4_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonLoad();
	afx_msg void OnBnClickedButtonPlay();
private:
//	StreamMedia*		m_pStreamMedia;
	CStreamMediaUtil*	m_pStreamMediaUtil;
	CString				m_strFilePath;
public:
	afx_msg void OnClose();
	CListCtrl m_Infolist;
	CToolTipCtrl pToolTip;
public:
	LRESULT OnSetDialogInfo(WPARAM wparam, LPARAM lparam);
	LRESULT OnSetBaseInfo(WPARAM wparam, LPARAM lparam);
	LRESULT OnSetTimeInfo(WPARAM wparam, LPARAM lparam);
	CSliderCtrl m_Slider;

	static int  TestThread_(void* lpParam );
	int  TestThreadFunc( );
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButtonPause();
};
CString GetTimeStr(int dbTime);



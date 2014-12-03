
// ffmpeg4Dlg.cpp : implementation file
//

#include "stdafx.h"
#include "ffmpeg4.h"
#include "ffmpeg4Dlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// Cffmpeg4Dlg dialog




Cffmpeg4Dlg::Cffmpeg4Dlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(Cffmpeg4Dlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_pStreamMediaUtil = NULL;
	
}

Cffmpeg4Dlg::~Cffmpeg4Dlg()
{
	if(m_pStreamMediaUtil)
	{
		delete m_pStreamMediaUtil;
		m_pStreamMediaUtil = NULL;
	}
}


void Cffmpeg4Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST2, m_Infolist);
	DDX_Control(pDX, IDC_SLIDER1, m_Slider);
}

BEGIN_MESSAGE_MAP(Cffmpeg4Dlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_LOAD, &Cffmpeg4Dlg::OnBnClickedButtonLoad)
	ON_BN_CLICKED(IDC_BUTTON_PLAY, &Cffmpeg4Dlg::OnBnClickedButtonPlay)
	ON_WM_CLOSE()
	ON_MESSAGE(WM_SETBASEINFO_MSG, OnSetDialogInfo)
	ON_MESSAGE(WM_SETINFO_MSG, OnSetBaseInfo)
	ON_MESSAGE(WM_SETCURTIME_MSG, OnSetTimeInfo)
	ON_BN_CLICKED(IDC_BUTTON1, &Cffmpeg4Dlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON_PAUSE, &Cffmpeg4Dlg::OnBnClickedButtonPause)
END_MESSAGE_MAP()


// Cffmpeg4Dlg message handlers

BOOL Cffmpeg4Dlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	// TODO: Add extra initialization here
	m_Infolist.SetExtendedStyle(LVS_EX_GRIDLINES|LVS_EX_FULLROWSELECT);
//	m_Infolist.InsertColumn(0, "ID", LVCFMT_LEFT, 100);
	m_Infolist.InsertColumn(0, "", LVCFMT_LEFT, 100);
	m_Infolist.InsertColumn(0, "type", LVCFMT_LEFT, 100);
	m_Infolist.InsertColumn(0, "Info", LVCFMT_LEFT, 100);
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void Cffmpeg4Dlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void Cffmpeg4Dlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR Cffmpeg4Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

// Load
void Cffmpeg4Dlg::OnBnClickedButtonLoad()
{
	// 文件扩展名过滤
	CString szFilter = _T("All Files (*.*)|*.*|avi Files (*.avi)|*.avi|rmvb Files (*.rmvb)|*.rmvb|3gp Files (*.3gp)|*.3gp|mp3 Files (*.mp3)|*.mp3|mp4 Files (*.mp4)|*.mp4|mpeg Files (*.ts)|*.ts|flv Files (*.flv)|*.flv|mov Files (*.mov)|*.mov||");
	CFileDialog dlg(TRUE,NULL ,NULL,OFN_PATHMUSTEXIST|OFN_HIDEREADONLY ,szFilter,NULL);
	if (IDOK == dlg.DoModal())
	{	
		//建立视频数据源（加载视频文件）
		m_strFilePath = dlg.GetPathName();
		if(m_pStreamMediaUtil == NULL)
		{
			HWND hwnd = GetDlgItem(IDC_SHOW_LIST)->GetSafeHwnd();
			CRect rect;
			GetDlgItem(IDC_SHOW_LIST)->GetClientRect(&rect);
			m_pStreamMediaUtil = new CStreamMediaUtil((long)hwnd);
			m_pStreamMediaUtil->SetShowInfo(rect.Width(), rect.Height(), m_hWnd);
		}
		m_pStreamMediaUtil->LoadStreamMedia(m_strFilePath.GetBuffer(0));
	}
/*	if(m_pStreamMediaUtil == NULL)
	{
		HWND hwnd = GetDlgItem(IDC_SHOW_LIST)->GetSafeHwnd();
		CRect rect;
		GetDlgItem(IDC_SHOW_LIST)->GetClientRect(&rect);
		m_pStreamMediaUtil = new CStreamMediaUtil((long)hwnd);
		m_pStreamMediaUtil->SetShowInfo(rect.Width(), rect.Height(), m_hWnd);
	}
	m_pStreamMediaUtil->LoadStreamMedia("http://yinyueshiting.baidu.com/data2/music/123297915/1201250291417406461128.mp3");
	*/
}

void Cffmpeg4Dlg::OnBnClickedButtonPlay()
{
	if(m_pStreamMediaUtil)
	{
		m_pStreamMediaUtil->PlayStreamMedia();
	}
}

void Cffmpeg4Dlg::OnBnClickedButtonPause()
{
	if(m_pStreamMediaUtil)
	{
		m_pStreamMediaUtil->Pause();
	}
	CString str;
	GetDlgItem(IDC_BUTTON_PAUSE)->GetWindowText(str);
	if(str == "Pause")	str = "Continue";
	else str = "Pause";
	GetDlgItem(IDC_BUTTON_PAUSE)->SetWindowText(str);
}

LRESULT Cffmpeg4Dlg::OnSetDialogInfo(WPARAM wparam, LPARAM lparam)
{
	DialogInfo* info = (DialogInfo*)wparam;
	if(info == NULL)		return 0;

	int nCount = m_Infolist.GetItemCount();
	m_Infolist.InsertItem(nCount, info->szType);
	m_Infolist.SetItemText(nCount, 0, info->szType);
	m_Infolist.SetItemText(nCount, 1, info->szSubType);
	m_Infolist.SetItemText(nCount, 2, info->szInfo);
}

LRESULT Cffmpeg4Dlg::OnSetBaseInfo(WPARAM wparam, LPARAM lparam)
{
	StreamInfo* info = (StreamInfo*)wparam;
	if(info == NULL)		return 0;

	GetDlgItem(IDC_STATIC_MAXTIME)->SetWindowText(GetTimeStr(info->Maxts));
	m_Slider.SetRange(0, info->Maxts);
	m_Slider.SetPageSize(20);	// 鼠标
	m_Slider.SetLineSize(5);	// PageDown PageUp
}

LRESULT Cffmpeg4Dlg::OnSetTimeInfo(WPARAM wparam, LPARAM lparam)
{
	StreamInfo* info = (StreamInfo*)wparam;
	if(info == NULL)		return 0;

	m_Slider.SetPos(info->dts);
	GetDlgItem(IDC_STATIC_CURTIME)->SetWindowText(GetTimeStr(info->dts));
	Invalidate();
}


void Cffmpeg4Dlg::OnClose()
{
	
	// TODO: Add your message handler code here and/or call default
	if(m_pStreamMediaUtil)
	{
		m_pStreamMediaUtil->Stop();
//		int nCount = 1000;
		while(m_pStreamMediaUtil->m_bRunning)
		{
//			Sleep(10);
//			nCount--;
//			if(nCount == 0)	break;
			Sleep(1);
		}
	}
	CDialogEx::OnClose();
}
int  Cffmpeg4Dlg::TestThread_(void* lpParam )
{
	Cffmpeg4Dlg * _this = (Cffmpeg4Dlg*)lpParam;
	if(_this)
	{
		_this->TestThreadFunc();
	}

	return 0;
}
static int pos = 0;
int  Cffmpeg4Dlg::TestThreadFunc( )
{
	while(pos < 1000)
	{
		pos++;
		m_Slider.SetPos(pos);
		CString str = GetTimeStr(pos);
		GetDlgItem(IDC_STATIC_CURTIME)->SetWindowText(str);
		CToolTipCtrl * pTIp = m_Slider.GetToolTips();
		if(pTIp)
			pTIp->UpdateTipText(str, &m_Slider);
		SDL_Delay(100);
	}
	return 0;
}


void Cffmpeg4Dlg::OnBnClickedButton1()
{
	pos = 0;
	m_Slider.SetRange(0, 1000);
	m_Slider.SetLineSize(50);		// pageup pagedown
	m_Slider.SetPageSize(200);		// lbutton click
	GetDlgItem(IDC_STATIC_MAXTIME)->SetWindowText(GetTimeStr(1000));
	pToolTip.Create(&m_Slider);
	m_Slider.SetToolTips(&pToolTip);

	SDL_CreateThread(TestThread_, this);
}

CString GetTimeStr(int dbTime)
{
	CString str;

	str.Format("%03d:%02d", dbTime/60, dbTime%60);
	return str;
}



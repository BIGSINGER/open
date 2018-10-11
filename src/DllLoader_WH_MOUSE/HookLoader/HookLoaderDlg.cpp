// HookTestDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "HookLoader.h"
#include "HookLoaderDlg.h"
#include "Tlhelp32.h"
#include <Shlwapi.h>
#include <vector>
#include <algorithm>
using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CHookLoadDlg 对话框




CHookLoadDlg::CHookLoadDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CHookLoadDlg::IDD, pParent)
	, m_szClassName(_T(""))
	, m_nHandle(0)
	, m_dwThreadId(0)
	, m_strProcessName(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);


	TCHAR szBuff[MAX_PATH*2] = {0};
	GetModuleFileName(NULL,szBuff,sizeof(szBuff));
	StrRChr(szBuff,NULL,'\\')[1] = 0;
	m_strStartPath = szBuff;
	m_strConfigFile = m_strStartPath + _T("config.ini");

	HMODULE hModule = LoadLibrary(m_strStartPath + _T("HookDll.dll"));
	if (hModule){
		StartHook = (TStartHook)GetProcAddress(hModule, "StartHook");
		StopHook = (TStopHook)GetProcAddress(hModule, "StopHook");
	}else{
		AfxMessageBox(_T("加载Hook.dll失败"));
	}

	m_bIsDrag		= FALSE;
	m_bMouseDown	= FALSE;
	m_hIconOrig		= NULL;
	m_hIconEmpty	= NULL;
	m_hWndLastFocus	= NULL;
}

void CHookLoadDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_FINDER, m_stcFinder);
	DDX_Text(pDX, IDC_EDIT_CLASS_NAME, m_szClassName);
	DDX_Text(pDX, IDC_EDIT_HANDLE, m_nHandle);
	DDX_Text(pDX, IDC_EDIT_PROCESSNAME, m_strProcessName);
}

BEGIN_MESSAGE_MAP(CHookLoadDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON_HOOK, &CHookLoadDlg::OnBnClickedButtonHook)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_BN_CLICKED(IDC_BUTTON_UNHOOK, &CHookLoadDlg::OnBnClickedButtonUnhook)
END_MESSAGE_MAP()


// CHookLoadDlg 消息处理程序

BOOL CHookLoadDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	/************************************************************************/
	m_hIconOrig  = LoadIcon(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDI_ORIG));
	m_hIconEmpty = LoadIcon(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDI_EMPTY));
	m_stcFinder.GetWindowRect(&m_rcFinder);
	ScreenToClient(&m_rcFinder);
	/************************************************************************/

	TCHAR szBuff[MAX_PATH] = {};
	GetPrivateProfileString("main", "pname", "notepad.exe", szBuff, sizeof(szBuff), m_strConfigFile);
	m_strProcessName = szBuff;
	m_strLastProcessName = m_strProcessName;
	UpdateData(FALSE);
	
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CHookLoadDlg::OnPaint()
{
	CDialog::OnPaint();
}

//当用户拖动最小化窗口时系统调用此函数取得光标显示。
//
HCURSOR CHookLoadDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

//由进程ID获取相应的主线程ID
DWORD GetThreadIdFromPID(DWORD dwProcessId)
{
	HANDLE ThreadHandle;
	THREADENTRY32 ThreadStruct;

	ThreadHandle=CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,dwProcessId);
	ThreadStruct.dwSize=sizeof(ThreadStruct);
	if(Thread32First(ThreadHandle,&ThreadStruct))
	{
		do 
		{
			if (ThreadStruct.th32OwnerProcessID==dwProcessId)
			{
				CloseHandle(ThreadHandle);
				return ThreadStruct.th32ThreadID;
			}
		} while (Thread32Next(ThreadHandle,&ThreadStruct));
	}
	CloseHandle(ThreadHandle);
	return 0;
}

// 获取进程名为xx的pid
int getPIdFromName(LPCTSTR szProcessName, vector<DWORD>&vtpids, int nLimit) {
	if (szProcessName == NULL) {
		return 0;
	}

	string strProcessName = szProcessName;
	transform(strProcessName.begin(), strProcessName.end(), strProcessName.begin(), _totlower);

	PROCESSENTRY32	ProcessEntry32;
	HANDLE			hSnap = INVALID_HANDLE_VALUE;

	ProcessEntry32.dwSize = sizeof(PROCESSENTRY32);

	hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnap == INVALID_HANDLE_VALUE) {
		return 0;
	}

	int ret = Process32First(hSnap, &ProcessEntry32);
	while (ret) {
		_tcslwr_s(ProcessEntry32.szExeFile);
		if (_tcsstr(ProcessEntry32.szExeFile, strProcessName.c_str())) {
			vtpids.push_back(ProcessEntry32.th32ProcessID);
			if (vtpids.size() >= nLimit) {
				break;
			}
		}
		ret = Process32Next(hSnap, &ProcessEntry32);
	}

	CloseHandle(hSnap);

	return (int)vtpids.size();
}

//查找一个进程名对应的pid
DWORD getPIdFromName(const char *szProcessName) {
	DWORD dwProcessId = 0;
	vector<DWORD>vtpids;

	getPIdFromName(szProcessName, vtpids, 1);
	if (vtpids.empty() == false) {
		dwProcessId = vtpids[0];
	}

	return dwProcessId;
}

CString getProcessName(DWORD dwProcessId)
{
	CString strName;
	PROCESSENTRY32	ProcessEntry32;
	HANDLE			hSnap;
	int				ret;

	ProcessEntry32.dwSize = sizeof (PROCESSENTRY32);

	hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if(hSnap != INVALID_HANDLE_VALUE){
		ret = Process32First(hSnap,&ProcessEntry32);
		while (ret){
			if ( ProcessEntry32.th32ProcessID==dwProcessId  ){
				strName=ProcessEntry32.szExeFile;
				break;
			}
			ret = Process32Next(hSnap, &ProcessEntry32);
		}

		CloseHandle(hSnap);
	}

	return strName;
}

void CHookLoadDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	if(m_rcFinder.PtInRect(point))
	{
		m_bMouseDown = TRUE;	
		SetCapture();
	}

	CDialog::OnLButtonDown(nFlags, point);
}

void CHookLoadDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	if(m_bIsDrag)
	{
		ReleaseCapture();
		m_bIsDrag = FALSE;
		m_stcFinder.SetIcon(m_hIconOrig);
		if(m_hWndLastFocus)
		{
			::InvalidateRect(NULL,NULL,TRUE);
			m_hWndLastFocus = NULL;
		}
	}
	m_bMouseDown = FALSE;

	CDialog::OnLButtonUp(nFlags, point);
}

void CHookLoadDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	if(m_bMouseDown)
	{
		::SetCursor(LoadCursor(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDC_DRAG)));
		m_stcFinder.SetIcon(m_hIconEmpty);
		m_bIsDrag = TRUE;

		//get the window
		CPoint ptCursor(point);
		ClientToScreen(&ptCursor);
		HiliTheWindow(ptCursor);
	}

	CDialog::OnMouseMove(nFlags, point);
}

void CHookLoadDlg::HiliTheWindow(CPoint point)
{
	HWND hWnd = ::WindowFromPoint(point);
	if(!hWnd) return;
	DWORD dwProcessId = 0;
	GetWindowThreadProcessId(hWnd, &dwProcessId);
	if(dwProcessId == GetCurrentProcessId()) 
		return;

	GetClassName(hWnd,m_szClassName.GetBuffer(128),128);
	m_szClassName.ReleaseBuffer();
	m_nHandle = (int)hWnd;
	m_strProcessName = getProcessName(dwProcessId);

	UpdateData(FALSE);

	HDC hdc = ::GetWindowDC(hWnd);
	if(hdc)
	{
		if(m_hWndLastFocus && m_hWndLastFocus != hWnd)
			::InvalidateRect(m_hWndLastFocus,NULL,TRUE); //::RedrawWindow(m_hWndLastFocus,NULL,NULL,RDW_FRAME|RDW_ERASE|RDW_UPDATENOW);
		m_hWndLastFocus = hWnd;

		CRect rcWnd;
		::GetWindowRect(hWnd,rcWnd);
		::MapWindowPoints(NULL,hWnd,(LPPOINT)&rcWnd,2);
		rcWnd.OffsetRect(-rcWnd.left,-rcWnd.top);
		//TRACE2("left %d,top %d,\n",rcWnd.left,rcWnd.top);

		::SelectObject(hdc,::GetStockObject(NULL_BRUSH));
		HPEN hPen = ::CreatePen(PS_SOLID,3,RGB(0,0,0));

		HPEN hPenOld = (HPEN)::SelectObject(hdc,hPen);
		::Rectangle(hdc,rcWnd.left,rcWnd.top,rcWnd.Width(),rcWnd.Height());

		::SelectObject(hdc,hPenOld);
		::DeleteObject(hPenOld);
		::ReleaseDC(hWnd,hdc);
	}
}

void CHookLoadDlg::OnBnClickedButtonHook()
{
	DWORD dwProcessId = 0;

	UpdateData();
	if ( m_strLastProcessName.CompareNoCase(m_strProcessName) !=0 ) {
		WritePrivateProfileString("main", "pname", m_strProcessName, m_strConfigFile);
	}


	if (m_nHandle){
		GetWindowThreadProcessId((HWND)((PBYTE)NULL + m_nHandle), &dwProcessId);
	}else {
		// 通过进程名找
		dwProcessId = getPIdFromName(m_strProcessName);
	}
	m_dwThreadId = GetThreadIdFromPID(dwProcessId);

	if (m_dwThreadId == 0){
		::MessageBox(NULL,_T("没有选定目标,将使用全局HOOK"),NULL,MB_OK|MB_ICONERROR);
	}

	StartHook((HWND)((PBYTE)NULL + m_nHandle), m_dwThreadId);
	TRACE0("开始HOOK……\n");
}

void CHookLoadDlg::OnBnClickedButtonUnhook()
{
	StopHook();
}

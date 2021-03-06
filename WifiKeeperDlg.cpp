
// WifiKeeperDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "WifiKeeper.h"
#include "WifiKeeperDlg.h"
#include "afxdialogex.h"
#include "cstdlib"
#include "iostream"
#include "fstream"
#include "Windows.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CWifiKeeperDlg 对话框

// Prevent ESC exit
BOOL CWifiKeeperDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		if (pMsg->wParam == VK_ESCAPE || pMsg->wParam == VK_RETURN)
		{
			return true;
		}
	}
	return CDialog::PreTranslateMessage(pMsg);
}

CWifiKeeperDlg::CWifiKeeperDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_WIFIKEEPER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CWifiKeeperDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CWifiKeeperDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(ID_START, &CWifiKeeperDlg::OnBnClickedStart)
	ON_BN_CLICKED(ID_STOP, &CWifiKeeperDlg::OnBnClickedStop)
	ON_BN_CLICKED(IDC_CHECK_TIME, &CWifiKeeperDlg::OnBnClickedCheckTime)
	ON_EN_CHANGE(IDC_EDIT_TIME, &CWifiKeeperDlg::OnEnChangeEditTime)
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CWifiKeeperDlg 消息处理程序

BOOL CWifiKeeperDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// Default Settings
	GetDlgItem(IDC_EDIT_TIME)->SetWindowText(_T("300"));
	GetDlgItem(IDC_EDIT_SSID)->SetWindowText(_T("WifiKeeper"));
	GetDlgItem(IDC_EDIT_PWD)->SetWindowText(_T("0123456789"));
	CButton* m_ctlCheck = (CButton*)GetDlgItem(IDC_CHECK_TIME);
	int CheckState = m_ctlCheck->GetCheck();
	m_ctlCheck->SetCheck(1);
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}


// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CWifiKeeperDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CWifiKeeperDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

inline bool file_exists(const std::string& name) {
	struct stat buffer;
	return (stat(name.c_str(), &buffer) == 0);
}

void CheckWifi() {
	using namespace std;
	CString powershell;
	ofstream file;
	if (!file_exists("check.ps1")) {
		file.open("check.ps1");
		powershell = { _T(R"(
$Host.UI.RawUI.WindowTitle = "WifiKeeper - Checking"
$connectionProfile = [Windows.Networking.Connectivity.NetworkInformation,Windows.Networking.Connectivity,ContentType=WindowsRuntime]::GetInternetConnectionProfile()
$tetheringManager = [Windows.Networking.NetworkOperators.NetworkOperatorTetheringManager,Windows.Networking.NetworkOperators,ContentType=WindowsRuntime]::CreateFromConnectionProfile($connectionProfile)
    if ($tetheringManager.TetheringOperationalState -eq 'Off') {
        $tetheringManager.StartTetheringAsync() | Out-Null
    }
)")
		};
		//get-process powershell | where-object {$_.MainWindowTitle -eq "WifiKeeper - Checking"} | stop-process
		file << (string)CW2A(powershell.GetString()) << endl;
		file.close();
		DWORD attributes = GetFileAttributes(_T("check.ps1"));
		SetFileAttributes(_T("check.ps1"), attributes + FILE_ATTRIBUTE_HIDDEN);
	}
	STARTUPINFO si = { sizeof(si) };
	PROCESS_INFORMATION pi = { 0 };
	TCHAR szApp[MAX_PATH] = _T("powershell -ExecutionPolicy Bypass -F check.ps1");
	if (::CreateProcess(NULL, szApp, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
	{
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
	}
	else {
		MessageBox(NULL, _T("WIFI Check Failed"), _T("WifiKeeper"), MB_OK | MB_ICONERROR);
	}
}

void StartWifi(CString Wifi_SSID, CString Wifi_PWD) {
	using namespace std;
	CString powershell;
	ofstream file;
	remove("start.ps1");
	file.open("start.ps1");
	powershell = { _T(R"(
$Host.UI.RawUI.WindowTitle = "WifiKeeper - Starting"
[Windows.System.UserProfile.LockScreen,Windows.System.UserProfile,ContentType=WindowsRuntime] | Out-Null

Add-Type -AssemblyName System.Runtime.WindowsRuntime

$asTaskGeneric = ([System.WindowsRuntimeSystemExtensions].GetMethods() | ? { $_.Name -eq 'AsTask' -and $_.GetParameters().Count -eq 1 -and $_.GetParameters()[0].ParameterType.Name -eq 'IAsyncOperation`1' })[0]
Function Await($WinRtTask, $ResultType) {
    $asTask = $asTaskGeneric.MakeGenericMethod($ResultType)
    $netTask = $asTask.Invoke($null, @($WinRtTask))
    $netTask.Wait(-1) | Out-Null
    $netTask.Result
}
Function AwaitAction($WinRtAction) {
    $asTask = ([System.WindowsRuntimeSystemExtensions].GetMethods() | ? { $_.Name -eq 'AsTask' -and $_.GetParameters().Count -eq 1 -and !$_.IsGenericMethod })[0]
    $netTask = $asTask.Invoke($null, @($WinRtAction))
    $netTask.Wait(-1) | Out-Null
}

$connectionProfile = [Windows.Networking.Connectivity.NetworkInformation,Windows.Networking.Connectivity,ContentType=WindowsRuntime]::GetInternetConnectionProfile()
$tetheringManager = [Windows.Networking.NetworkOperators.NetworkOperatorTetheringManager,Windows.Networking.NetworkOperators,ContentType=WindowsRuntime]::CreateFromConnectionProfile($connectionProfile)


$configuration = new-object Windows.Networking.NetworkOperators.NetworkOperatorTetheringAccessPointConfiguration

$configuration.Ssid = ')") + Wifi_SSID + _T(R"('
$configuration.Passphrase = ')") + Wifi_PWD + _T(R"('

AwaitAction ($tetheringManager.ConfigureAccessPointAsync($configuration))
Await ($tetheringManager.StartTetheringAsync())([Windows.Networking.NetworkOperators.NetworkOperatorTetheringOperationResult])
	)") };
	file << (string)CW2A(powershell.GetString()) << endl;
	file.close();
	DWORD attributes = GetFileAttributes(_T("start.ps1"));
	SetFileAttributes(_T("start.ps1"), attributes + FILE_ATTRIBUTE_HIDDEN);
	STARTUPINFO si = { sizeof(si) };
	PROCESS_INFORMATION pi = { 0 };
	TCHAR szApp[MAX_PATH] = _T("powershell -windowstyle hidden -ExecutionPolicy Bypass -F start.ps1");
	if (::CreateProcess(NULL, szApp, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
	{
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
		MessageBox(NULL, _T("WIFI Starting"), _T("WifiKeeper"), MB_OK | MB_ICONINFORMATION);
	}
	else {
		MessageBox(NULL, _T("WIFI Start Failed"), _T("WifiKeeper"), MB_OK | MB_ICONERROR);
	}
}

void StopWifi() {
	using namespace std;
	CString powershell;
	ofstream file;
	if (!file_exists("stop.ps1")) {
		file.open("stop.ps1");
		powershell = { R"(
$Host.UI.RawUI.WindowTitle = "WifiKeeper - Stopping"
$connectionProfile = [Windows.Networking.Connectivity.NetworkInformation,Windows.Networking.Connectivity,ContentType=WindowsRuntime]::GetInternetConnectionProfile()
$tetheringManager = [Windows.Networking.NetworkOperators.NetworkOperatorTetheringManager,Windows.Networking.NetworkOperators,ContentType=WindowsRuntime]::CreateFromConnectionProfile($connectionProfile)

Await ($tetheringManager.StopTetheringAsync()) ([Windows.Networking.NetworkOperators.NetworkOperatorTetheringOperationResult])
	)" };
		file << (string)CW2A(powershell.GetString()) << endl;
		file.close();
		DWORD attributes = GetFileAttributes(_T("stop.ps1"));
		SetFileAttributes(_T("stop.ps1"), attributes + FILE_ATTRIBUTE_HIDDEN);
	}
	STARTUPINFO si = { sizeof(si) };
	PROCESS_INFORMATION pi = { 0 };
	TCHAR szApp[MAX_PATH] = _T("powershell -windowstyle hidden -ExecutionPolicy Bypass -F stop.ps1");
	if (::CreateProcess(NULL, szApp, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
	{
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
		MessageBox(NULL, _T("WIFI Stopping"), _T("WifiKeeper"), MB_OK | MB_ICONINFORMATION);
	}
	else {
		MessageBox(NULL, _T("WIFI Stop Failed"), _T("WifiKeeper"), MB_OK | MB_ICONERROR);
	}
}

void CWifiKeeperDlg::OnBnClickedStart()
{
	using namespace std;
	CButton* m_ctlCheck = (CButton*)GetDlgItem(IDC_CHECK_TIME);
	CString Wifi_SSID, Wifi_PWD, Wifi_Time;
	int CheckState = m_ctlCheck->GetCheck();
	GetDlgItem(IDC_EDIT_SSID)->GetWindowText(Wifi_SSID);
	GetDlgItem(IDC_EDIT_PWD)->GetWindowText(Wifi_PWD);
	GetDlgItem(IDC_EDIT_TIME)->GetWindowText(Wifi_Time);
	if (Wifi_SSID != "") {
		if (Wifi_PWD != "" && Wifi_PWD.GetLength() >= 8) {
			if (CheckState != BST_CHECKED || (string)CW2A(Wifi_Time.GetString()) <= "0") {
				KillTimer(1);
				SetWindowText(_T("WifiKeeper"));
			}
			else {
				UINT Wifi_Timer = 1000 * atoi(CW2A(Wifi_Time.GetString()));
				KillTimer(1);
				SetTimer(1, Wifi_Timer, NULL);
				SetWindowText(_T("WifiKeeper - Auto Check Enabled"));
			}
			StartWifi(Wifi_SSID,Wifi_PWD);
		}
		else {
			MessageBoxA(NULL, (LPCSTR)"Invalid WIFI Password\n(password need at least 8 characters)", (LPCSTR)"WifiKeeper", MB_OK | MB_ICONWARNING);
		}
	}
	else {
		MessageBoxA(NULL, (LPCSTR)"Invalid WIFI Name", (LPCSTR)"WifiKeeper", MB_OK | MB_ICONWARNING);
	}
}

void CWifiKeeperDlg::OnBnClickedStop()
{
	StopWifi();
	KillTimer(1);
	SetWindowText(_T("WifiKeeper"));
}

void CWifiKeeperDlg::OnBnClickedCheckTime()
{
	using namespace std;
	CButton* m_ctlCheck = (CButton*)GetDlgItem(IDC_CHECK_TIME);
	CString Wifi_Time;
	int CheckState = m_ctlCheck->GetCheck();
	GetDlgItem(IDC_EDIT_TIME)->GetWindowText(Wifi_Time);
	if (CheckState == BST_CHECKED && (string)CW2A(Wifi_Time.GetString()) <= "0") m_ctlCheck->SetCheck(0);
}

void CWifiKeeperDlg::OnEnChangeEditTime()
{
	using namespace std;
	CButton* m_ctlCheck = (CButton*)GetDlgItem(IDC_CHECK_TIME);
	CString Wifi_Time;
	GetDlgItem(IDC_EDIT_TIME)->GetWindowText(Wifi_Time);
	if ((string)CW2A(Wifi_Time.GetString()) <= "0") m_ctlCheck->SetCheck(0);
}

void CWifiKeeperDlg::OnClose()
{
	using namespace std;
	ofstream file;
	remove("start.ps1");
	remove("stop.ps1");
	remove("check.ps1");
	CDialogEx::OnClose();
}

void CWifiKeeperDlg::OnDestroy()
{
	CDialogEx::OnDestroy();
	using namespace std;
	ofstream file;
	remove("start.ps1");
	remove("stop.ps1");
	remove("check.ps1");
}

void CWifiKeeperDlg::OnTimer(UINT_PTR nIDEvent)
{
	CheckWifi();
	CDialogEx::OnTimer(nIDEvent);
}

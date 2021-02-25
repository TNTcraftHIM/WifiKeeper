
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

void CheckWifi(CString Wifi_CheckTime) {
	using namespace std;
	CString powershell;
	ofstream file;
	remove("check.ps1");
	file.open("check.ps1");
	powershell = { _T(R"(
get-process powershell | where-object {$_.MainWindowTitle -eq "WifiKeeper - Auto Check"} | stop-process
$Host.UI.RawUI.WindowTitle = "WifiKeeper - Auto Check"
$connectionProfile = [Windows.Networking.Connectivity.NetworkInformation,Windows.Networking.Connectivity,ContentType=WindowsRuntime]::GetInternetConnectionProfile()
$tetheringManager = [Windows.Networking.NetworkOperators.NetworkOperatorTetheringManager,Windows.Networking.NetworkOperators,ContentType=WindowsRuntime]::CreateFromConnectionProfile($connectionProfile)
$FirstFlag='On'
$Time=)") + Wifi_CheckTime + _T(R"( 
if ($Time -eq '1') {
    Write-Output "Auto Checking WIFI Every $Time Second "
}
else {
    Write-Output "Auto Checking WIFI Every $Time Second(s) "
}
while ($true) {
    if ($FirstFlag -eq 'On'){
        $FirstFlag='Off'
    }
    else{
        if ($tetheringManager.TetheringOperationalState -eq 'Off') {
            $tetheringManager.StartTetheringAsync() | Out-Null
		    Get-Date
            Write-Output 'Restarting WIFI'
        }
    }
    Start-Sleep -s $Time
}
)")
	};
	file << (string)CW2A(powershell.GetString()) << endl;
	file.close();
	ShellExecute(0, _T("open"), _T("cmd.exe"), _T("/C attrib +h check.ps1"), 0, SW_HIDE);
	ShellExecute(0, _T("open"), _T("cmd.exe"), _T("/C start PowerShell.exe -ExecutionPolicy Bypass -F check.ps1"), 0, SW_HIDE);
}

void StartWifi(CString Wifi_SSID, CString Wifi_PWD, CString Wifi_Time) {
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
	ShellExecute(0, _T("open"), _T("cmd.exe"), _T("/C attrib +h start.ps1"), 0, SW_HIDE);
	ShellExecute(0, _T("open"), _T("cmd.exe"), _T("/C start PowerShell.exe -windowstyle hidden -ExecutionPolicy Bypass -F start.ps1"), 0, SW_HIDE);
	if ((string)CW2A(Wifi_Time.GetString()) > "0") CheckWifi(Wifi_Time);
}

void StopWifi() {
	using namespace std;
	CString powershell;
	ofstream file;
	remove("stop.ps1");
	file.open("stop.ps1");
	powershell = { R"(
$Host.UI.RawUI.WindowTitle = "WifiKeeper - Stopping"
$connectionProfile = [Windows.Networking.Connectivity.NetworkInformation,Windows.Networking.Connectivity,ContentType=WindowsRuntime]::GetInternetConnectionProfile()
$tetheringManager = [Windows.Networking.NetworkOperators.NetworkOperatorTetheringManager,Windows.Networking.NetworkOperators,ContentType=WindowsRuntime]::CreateFromConnectionProfile($connectionProfile)

Await ($tetheringManager.StopTetheringAsync()) ([Windows.Networking.NetworkOperators.NetworkOperatorTetheringOperationResult])
get-process powershell | where-object {$_.MainWindowTitle -eq "WifiKeeper - Auto Check"} | stop-process
	)" };
	file << (string)CW2A(powershell.GetString()) << endl;
	file.close();
	ShellExecute(0, _T("open"), _T("cmd.exe"), _T("/C attrib +h stop.ps1"), 0, SW_HIDE);
	ShellExecute(0, _T("open"), _T("cmd.exe"), _T("/C start PowerShell.exe -windowstyle hidden -ExecutionPolicy Bypass -F stop.ps1"), 0, SW_HIDE);
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
			if (CheckState != BST_CHECKED || (string)CW2A(Wifi_Time.GetString()) <= "0") Wifi_Time = _T("0");
			StartWifi(Wifi_SSID,Wifi_PWD,Wifi_Time);
		}
		else {
			MessageBox(_T("invalid WIFI password\n(password need at least 8 characters)"));
		}
	}
	else {
		MessageBox(_T("invalid WIFI SSID"));
	}
}

void CWifiKeeperDlg::OnBnClickedStop()
{
	StopWifi();
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
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	using namespace std;
	ofstream file;
	remove("start.ps1");
	remove("stop.ps1");
	remove("check.ps1");
	CDialogEx::OnClose();
}

#include <string>
#include <optional>
#include <Windows.h>
#include <winternl.h>
#include <psapi.h>
#include "IbWinCppLib/WinCppLib.hpp"

#pragma comment(lib, "ntdll.lib")

using namespace std;
using namespace ib;

#ifdef _DEBUG
constexpr bool kDebug = true;
#else
constexpr bool kDebug = false;
#endif

//PROCESS_QUERY_INFORMATION | PROCESS_VM_READ
unique_ptr<wchar[]> GetProcessCmdLine(HANDLE process) {
    PROCESS_BASIC_INFORMATION pbi;
    NtQueryInformationProcess(process, ProcessBasicInformation, &pbi, sizeof pbi, nullptr);
    _RTL_USER_PROCESS_PARAMETERS* rtlUserProcParamsAddress;
    if (!ReadProcessMemory(process, &pbi.PebBaseAddress->ProcessParameters, &rtlUserProcParamsAddress, sizeof(void*), nullptr)) return nullptr;
    UNICODE_STRING commandline;
    if (!ReadProcessMemory(process, &rtlUserProcParamsAddress->CommandLine, &commandline, sizeof(commandline), nullptr)) return nullptr;
    unique_ptr<wchar[]> commandline_content(new wchar[commandline.Length + 1]);  //Length doesn't include the trailing null char
    if (!ReadProcessMemory(process, commandline.Buffer, commandline_content.get(), commandline.Length, nullptr)) return nullptr;
    commandline_content[commandline.Length] = L'\0';
    return move(commandline_content);
}

//SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION
void GuardProcess(HANDLE process, wchar* cmdline, bool cmdline_include_name) {
    wchar path[MAX_PATH];
    if (!cmdline_include_name) {
        DWORD path_size = auto_cast(size(path));
        QueryFullProcessImageNameW(process, 0, path, &path_size);
        if constexpr (kDebug)
            OutputDebugStringW(path);
    }
    WaitForSingleObject(process, INFINITE);
    STARTUPINFO si{ sizeof(STARTUPINFO) };
    PROCESS_INFORMATION pi;
    CreateProcessW(cmdline_include_name ? nullptr : path, cmdline, nullptr, nullptr, false, 0, nullptr, nullptr, &si, &pi);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

int WINAPI wWinMain(HINSTANCE hInstanceExe, HINSTANCE, PTSTR pszCmdLine, int nCmdShow) {
    //Get parent process
    PROCESS_BASIC_INFORMATION pbi;
    NtQueryInformationProcess(GetCurrentProcess(), ProcessBasicInformation, &pbi, sizeof pbi, nullptr);
    DWORD parent_pid = auto_cast(pbi.Reserved3);
    if constexpr(kDebug)
        OutputDebugStringW(to_wstring(parent_pid).c_str());
    
    HANDLE parent_handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | SYNCHRONIZE, false, parent_pid);
    if (!parent_handle) return 1;

    //Get parent or specified command line, and then guard parent process
    if constexpr (kDebug) {
        OutputDebugStringW((to_wstring(wcslen(pszCmdLine)) + L' ' + pszCmdLine).c_str());  //Sometimes incorrectly empty
        OutputDebugStringW(GetCommandLineW());
    }
    wchar* specified_cmdline = GetCommandLineW();
    if (wcslen(specified_cmdline)) {
        GuardProcess(parent_handle, specified_cmdline, false);
    }
    else {
        unique_ptr<wchar[]> parent_cmdline = GetProcessCmdLine(parent_handle);
        if constexpr (kDebug)
            OutputDebugStringW(parent_cmdline.get());
        GuardProcess(parent_handle, parent_cmdline.get(), true);
    }

    CloseHandle(parent_handle);
    
    return 0;
}
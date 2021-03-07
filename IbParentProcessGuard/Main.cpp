#include <string>
#include <optional>
#include <Windows.h>
#include <winternl.h>
#include "IbWinCppLib/WinCppLib.hpp"

#pragma comment(lib, "ntdll.lib")

using namespace std;
using namespace ib;

#ifdef _DEBUG
constexpr bool kDebug = true;
#else
constexpr bool kDebug = false;
#endif

int WINAPI wWinMain(HINSTANCE hInstanceExe, HINSTANCE, PTSTR pszCmdLine, int nCmdShow) {
    //Get parent process
    PROCESS_BASIC_INFORMATION pbi;
    NtQueryInformationProcess(GetCurrentProcess(), ProcessBasicInformation, &pbi, sizeof pbi, nullptr);
    DWORD parent_pid = auto_cast(pbi.Reserved3);
    if constexpr(kDebug)
        OutputDebugStringW(to_wstring(parent_pid).c_str());
    
    HANDLE parent_handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | SYNCHRONIZE, false, parent_pid);
    if (!parent_handle) return 1;

    //Get parent command line
    NtQueryInformationProcess(parent_handle, ProcessBasicInformation, &pbi, sizeof pbi, nullptr);
    _RTL_USER_PROCESS_PARAMETERS* rtlUserProcParamsAddress;
    auto commandline = [&]() -> unique_ptr<wchar[]> {
        if(!ReadProcessMemory(parent_handle, &pbi.PebBaseAddress->ProcessParameters, &rtlUserProcParamsAddress, sizeof(void*), nullptr)) return nullptr;
        UNICODE_STRING commandline;
        if(!ReadProcessMemory(parent_handle, &rtlUserProcParamsAddress->CommandLine, &commandline, sizeof(commandline), nullptr)) return nullptr;
        unique_ptr<wchar[]> commandline_content(new wchar[commandline.Length+1]);  //Length doesn't include the trailing null char
        if(!ReadProcessMemory(parent_handle, commandline.Buffer, commandline_content.get(), commandline.Length, nullptr)) return nullptr;
        commandline_content[commandline.Length] = L'\0';
        return move(commandline_content);
    }();
    if(!commandline) {
        CloseHandle(parent_handle);
        return 2;
    }
    if constexpr(kDebug)
        OutputDebugStringW(commandline.get());

    //Guard
    WaitForSingleObject(parent_handle, INFINITE);
    STARTUPINFO si{ sizeof(STARTUPINFO) };
    PROCESS_INFORMATION pi;
    CreateProcessW(nullptr, commandline.get(), nullptr, nullptr, false, 0, nullptr, nullptr, &si, &pi);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    CloseHandle(parent_handle);
    
    return 0;
}
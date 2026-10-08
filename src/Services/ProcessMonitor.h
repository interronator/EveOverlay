#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <Windows.h>
#include <tlhelp32.h>

#include "Services/IProcessMonitor.h"

class ProcessMonitor : public IProcessMonitor
{
public:
    ProcessMonitor() = default;

    ProcessInfo GetMainProcess() override;
    ProcessUpdate GetUpdatedProcesses() override;

    static std::vector<DWORD> EnumerateMonitoredProcessIds();

private:
    static constexpr const wchar_t* MONITORED_EXECUTABLE = L"ExeFile.exe";

    struct EnumerationContext
    {
        std::unordered_map<DWORD, ProcessInfo>* Windows;
        const std::unordered_set<DWORD>* Filter;
    };

    static BOOL CALLBACK EnumerateWindowCallback(const HWND Window, const LPARAM Parameter);
    static std::unordered_map<DWORD, ProcessInfo> EnumerateMainWindows(const std::unordered_set<DWORD>* const Filter);

    std::unordered_map<HWND, std::wstring> ProcessCache;
    ProcessInfo CurrentProcessInfo;
};

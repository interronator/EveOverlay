#include "Services/ProcessMonitor.h"

// The own window does not exist yet at construction time, so it is captured lazily
ProcessInfo ProcessMonitor::GetMainProcess()
{
    if (CurrentProcessInfo.Handle != nullptr)
    {
        return CurrentProcessInfo;
    }

    const std::unordered_set<DWORD> OwnProcess{::GetCurrentProcessId()};
    const std::unordered_map<DWORD, ProcessInfo> Windows = EnumerateMainWindows(&OwnProcess);
    const std::unordered_map<DWORD, ProcessInfo>::const_iterator Found = Windows.find(::GetCurrentProcessId());
    if (Found == Windows.end())
    {
        return CurrentProcessInfo;
    }

    if (Found->second.Title.empty() == true)
    {
        return CurrentProcessInfo;
    }

    CurrentProcessInfo = Found->second;
    return CurrentProcessInfo;
}

ProcessUpdate ProcessMonitor::GetUpdatedProcesses()
{
    ProcessUpdate Update;

    const std::vector<DWORD> MonitoredProcessIds = EnumerateMonitoredProcessIds();
    const std::unordered_set<DWORD> MonitoredSet(MonitoredProcessIds.begin(), MonitoredProcessIds.end());
    const std::unordered_map<DWORD, ProcessInfo> Windows = EnumerateMainWindows(&MonitoredSet);
    std::unordered_map<HWND, std::wstring> KnownProcesses = ProcessCache;

    for (const DWORD ProcessId : MonitoredProcessIds)
    {
        const std::unordered_map<DWORD, ProcessInfo>::const_iterator Found = Windows.find(ProcessId);
        if (Found == Windows.end())
        {
            continue;
        }

        const ProcessInfo& Window = Found->second;
        const std::unordered_map<HWND, std::wstring>::iterator Cached = ProcessCache.find(Window.Handle);
        if (Cached == ProcessCache.end())
        {
            ProcessCache.emplace(Window.Handle, Window.Title);
            Update.Added.push_back(Window);
            continue;
        }

        KnownProcesses.erase(Window.Handle);
        if (Cached->second == Window.Title)
        {
            continue;
        }

        Cached->second = Window.Title;
        Update.Updated.push_back(Window);
    }

    for (const std::pair<const HWND, std::wstring>& Entry : KnownProcesses)
    {
        Update.Removed.push_back(ProcessInfo{Entry.first, Entry.second});
        ProcessCache.erase(Entry.first);
    }

    return Update;
}

// Mirrors the .NET MainWindowHandle semantics: the first visible, unowned top-level window of a process
BOOL CALLBACK ProcessMonitor::EnumerateWindowCallback(const HWND Window, const LPARAM Parameter)
{
    EnumerationContext* const Context = reinterpret_cast<EnumerationContext*>(Parameter);

    if (::IsWindowVisible(Window) == FALSE)
    {
        return TRUE;
    }

    if (::GetWindow(Window, GW_OWNER) != nullptr)
    {
        return TRUE;
    }

    // Thumbnails and similar helper windows are never the main window
    if ((::GetWindowLongPtrW(Window, GWL_EXSTYLE) & WS_EX_TOOLWINDOW) != 0)
    {
        return TRUE;
    }

    DWORD ProcessId = 0;
    ::GetWindowThreadProcessId(Window, &ProcessId);
    if (Context->Filter->contains(ProcessId) == false || Context->Windows->contains(ProcessId) == true)
    {
        return TRUE;
    }

    std::wstring Title(static_cast<size_t>(::GetWindowTextLengthW(Window)) + 1, L'\0');
    const int Length = ::GetWindowTextW(Window, Title.data(), static_cast<int>(Title.size()));
    Title.resize(static_cast<size_t>(Length));

    Context->Windows->emplace(ProcessId, ProcessInfo{Window, Title});
    return TRUE;
}

std::unordered_map<DWORD, ProcessInfo> ProcessMonitor::EnumerateMainWindows(const std::unordered_set<DWORD>* const Filter)
{
    std::unordered_map<DWORD, ProcessInfo> Windows;
    EnumerationContext Context{&Windows, Filter};
    ::EnumWindows(&ProcessMonitor::EnumerateWindowCallback, reinterpret_cast<LPARAM>(&Context));
    return Windows;
}

std::vector<DWORD> ProcessMonitor::EnumerateMonitoredProcessIds()
{
    std::vector<DWORD> ProcessIds;

    const HANDLE Snapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (Snapshot == INVALID_HANDLE_VALUE)
    {
        return ProcessIds;
    }

    PROCESSENTRY32W Entry = {};
    Entry.dwSize = sizeof(Entry);

    for (BOOL HasEntry = ::Process32FirstW(Snapshot, &Entry); HasEntry != FALSE; HasEntry = ::Process32NextW(Snapshot, &Entry))
    {
        if (::_wcsicmp(Entry.szExeFile, MONITORED_EXECUTABLE) != 0)
        {
            continue;
        }

        ProcessIds.push_back(Entry.th32ProcessID);
    }

    ::CloseHandle(Snapshot);
    return ProcessIds;
}

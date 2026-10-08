#include <cstdio>

#include "Services/ProcessMonitor.h"
#include "Services/WindowManager.h"

namespace
{
    void PrintProcesses(const wchar_t* const Label, const std::vector<ProcessInfo>& Processes)
    {
        for (const ProcessInfo& Process : Processes)
        {
            std::wprintf(L"%ls: %p \"%ls\"\n", Label, static_cast<void*>(Process.Handle), Process.Title.c_str());
        }
    }
}

int wmain()
{
    ProcessMonitor Monitor;
    WindowManager Manager;

    std::wprintf(L"Composition enabled: %d\n", Manager.IsCompositionEnabled() == true ? 1 : 0);

    const ProcessUpdate First = Monitor.GetUpdatedProcesses();
    PrintProcesses(L"Added", First.Added);
    PrintProcesses(L"Updated", First.Updated);
    PrintProcesses(L"Removed", First.Removed);

    for (const ProcessInfo& Process : First.Added)
    {
        const RECT Position = Manager.GetWindowPosition(Process.Handle);
        std::wprintf(L"Position: %ld,%ld,%ld,%ld maximized=%d minimized=%d\n", Position.left, Position.top, Position.right, Position.bottom,
            Manager.IsWindowMaximized(Process.Handle) == true ? 1 : 0, Manager.IsWindowMinimized(Process.Handle) == true ? 1 : 0);

        const HBITMAP Bitmap = Manager.GetStaticThumbnail(Process.Handle);
        std::wprintf(L"Static thumbnail: %ls\n", Bitmap != nullptr ? L"captured" : L"null (too small)");
        if (Bitmap != nullptr)
        {
            ::DeleteObject(Bitmap);
        }
    }

    const ProcessUpdate Second = Monitor.GetUpdatedProcesses();
    std::wprintf(L"Second pass: added=%zu updated=%zu removed=%zu\n", Second.Added.size(), Second.Updated.size(), Second.Removed.size());
    return 0;
}

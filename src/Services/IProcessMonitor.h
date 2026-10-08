#pragma once

#include <string>
#include <vector>

#include <Windows.h>

struct ProcessInfo
{
    HWND Handle = nullptr;
    std::wstring Title;
};

struct ProcessUpdate
{
    std::vector<ProcessInfo> Added;
    std::vector<ProcessInfo> Updated;
    std::vector<ProcessInfo> Removed;
};

class IProcessMonitor
{
public:
    virtual ~IProcessMonitor() = default;

    virtual ProcessInfo GetMainProcess() = 0;
    virtual ProcessUpdate GetUpdatedProcesses() = 0;
};

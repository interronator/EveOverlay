#pragma once

#include <Windows.h>

class SingleInstanceGuard
{
public:
    SingleInstanceGuard();
    SingleInstanceGuard(const SingleInstanceGuard&) = delete;
    SingleInstanceGuard& operator=(const SingleInstanceGuard&) = delete;

    ~SingleInstanceGuard();

    bool IsFirstInstance() const;

private:
    static constexpr const wchar_t* MUTEX_NAME = L"EveOverlay Single Instance Mutex";

    static HANDLE CreateNamedMutex();

    HANDLE const MutexHandle;
    bool FirstInstance = false;
};

#include "Application/SingleInstanceGuard.h"

SingleInstanceGuard::SingleInstanceGuard()
    : MutexHandle(CreateNamedMutex())
{
    FirstInstance = MutexHandle != nullptr && ::GetLastError() != ERROR_ALREADY_EXISTS;
}

SingleInstanceGuard::~SingleInstanceGuard()
{
    if (MutexHandle == nullptr)
    {
        return;
    }

    ::CloseHandle(MutexHandle);
}

bool SingleInstanceGuard::IsFirstInstance() const
{
    return FirstInstance;
}

// CreateMutex does not clear a stale last-error on success, so it is reset first
HANDLE SingleInstanceGuard::CreateNamedMutex()
{
    ::SetLastError(ERROR_SUCCESS);
    return ::CreateMutexW(nullptr, TRUE, MUTEX_NAME);
}

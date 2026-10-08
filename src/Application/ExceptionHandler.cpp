#include "Application/ExceptionHandler.h"

#include "Application/Logger.h"

void ExceptionHandler::Install()
{
    if (::IsDebuggerPresent() != FALSE)
    {
        return;
    }

    ::SetUnhandledExceptionFilter(&ExceptionHandler::HandleStructuredException);
}

void ExceptionHandler::Report(const std::string& Description)
{
    Logger::Error(Description);

    const std::wstring Message = L"An unexpected error occurred. Details were written to the \"logs\" folder next to the program.\n\n"
        + std::wstring(Description.begin(), Description.end());
    ::MessageBoxW(nullptr, Message.c_str(), L"Eve Overlay", MB_OK | MB_ICONERROR);
}

LONG WINAPI ExceptionHandler::HandleStructuredException(EXCEPTION_POINTERS* const Pointers)
{
    const std::string Description = "Unhandled exception, code 0x" + ToHex(Pointers->ExceptionRecord->ExceptionCode)
        + " at 0x" + ToHex(static_cast<DWORD>(reinterpret_cast<ULONG_PTR>(Pointers->ExceptionRecord->ExceptionAddress) >> 32))
        + ToHex(static_cast<DWORD>(reinterpret_cast<ULONG_PTR>(Pointers->ExceptionRecord->ExceptionAddress)));
    Report(Description);
    ::ExitProcess(1);
}

std::string ExceptionHandler::ToHex(const DWORD Value)
{
    const char* const DIGITS = "0123456789ABCDEF";
    std::string Result;
    for (int Shift = 28; Shift >= 0; Shift -= 4)
    {
        Result += DIGITS[(Value >> Shift) & 0xF];
    }

    return Result;
}

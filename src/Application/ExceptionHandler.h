#pragma once

#include <string>

#include <Windows.h>

class ExceptionHandler
{
public:
    // Left uninstalled under a debugger so the debugger sees the original exception
    static void Install();
    static void Report(const std::string& Description);

private:
    static LONG WINAPI HandleStructuredException(EXCEPTION_POINTERS* const Pointers);
    static std::string ToHex(const DWORD Value);
};

#pragma once

#include <Windows.h>

class DpiContext
{
public:
    static constexpr UINT BASE_DPI = 96;

    static UINT GetWindowDpi(const HWND Window);

    UINT GetDpi() const;
    void SetDpi(const UINT NewDpi);
    int Scale(const int Value) const;

private:
    UINT Dpi = BASE_DPI;
};

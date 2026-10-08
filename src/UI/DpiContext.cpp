#include "UI/DpiContext.h"

UINT DpiContext::GetWindowDpi(const HWND Window)
{
    const UINT WindowDpi = ::GetDpiForWindow(Window);
    if (WindowDpi == 0)
    {
        return BASE_DPI;
    }

    return WindowDpi;
}

UINT DpiContext::GetDpi() const
{
    return Dpi;
}

void DpiContext::SetDpi(const UINT NewDpi)
{
    Dpi = NewDpi;
}

int DpiContext::Scale(const int Value) const
{
    return ::MulDiv(Value, static_cast<int>(Dpi), static_cast<int>(BASE_DPI));
}

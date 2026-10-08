#include "Services/Interop/Gdi32Api.h"

HDC Gdi32Api::CreateCompatibleContext(const HDC Source)
{
    return ::CreateCompatibleDC(Source);
}

HBITMAP Gdi32Api::CreateCompatibleBitmapHandle(const HDC Source, const int Width, const int Height)
{
    return ::CreateCompatibleBitmap(Source, Width, Height);
}

HGDIOBJ Gdi32Api::SelectObjectHandle(const HDC DeviceContext, const HGDIOBJ Object)
{
    return ::SelectObject(DeviceContext, Object);
}

void Gdi32Api::DeleteContext(const HDC DeviceContext)
{
    ::DeleteDC(DeviceContext);
}

void Gdi32Api::DeleteObjectHandle(const HGDIOBJ Object)
{
    ::DeleteObject(Object);
}

bool Gdi32Api::CopyBlock(const HDC Destination, const int Width, const int Height, const HDC Source)
{
    return ::BitBlt(Destination, 0, 0, Width, Height, Source, 0, 0, SRCCOPY) != FALSE;
}

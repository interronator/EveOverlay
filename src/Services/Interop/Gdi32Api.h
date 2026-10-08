#pragma once

#include <Windows.h>

class Gdi32Api
{
public:
    static HDC CreateCompatibleContext(const HDC Source);
    static HBITMAP CreateCompatibleBitmapHandle(const HDC Source, const int Width, const int Height);
    static HGDIOBJ SelectObjectHandle(const HDC DeviceContext, const HGDIOBJ Object);
    static void DeleteContext(const HDC DeviceContext);
    static void DeleteObjectHandle(const HGDIOBJ Object);
    static bool CopyBlock(const HDC Destination, const int Width, const int Height, const HDC Source);
};

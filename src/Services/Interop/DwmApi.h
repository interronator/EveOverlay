#pragma once

#include <Windows.h>
#include <dwmapi.h>

class DwmApi
{
public:
    static bool IsCompositionEnabled();
    static HRESULT RegisterThumbnail(const HWND Destination, const HWND Source, HTHUMBNAIL* const Thumbnail);
    static HRESULT UnregisterThumbnail(const HTHUMBNAIL Thumbnail);
    static HRESULT UpdateThumbnailProperties(const HTHUMBNAIL Thumbnail, const DWM_THUMBNAIL_PROPERTIES& Properties);
};

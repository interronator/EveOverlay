#include "Services/Interop/DwmApi.h"

bool DwmApi::IsCompositionEnabled()
{
    BOOL Enabled = FALSE;
    if (FAILED(::DwmIsCompositionEnabled(&Enabled)) == true)
    {
        return false;
    }

    return Enabled == TRUE;
}

HRESULT DwmApi::RegisterThumbnail(const HWND Destination, const HWND Source, HTHUMBNAIL* const Thumbnail)
{
    return ::DwmRegisterThumbnail(Destination, Source, Thumbnail);
}

HRESULT DwmApi::UnregisterThumbnail(const HTHUMBNAIL Thumbnail)
{
    return ::DwmUnregisterThumbnail(Thumbnail);
}

HRESULT DwmApi::UpdateThumbnailProperties(const HTHUMBNAIL Thumbnail, const DWM_THUMBNAIL_PROPERTIES& Properties)
{
    return ::DwmUpdateThumbnailProperties(Thumbnail, &Properties);
}

#pragma once

#include "Views/ThumbnailView.h"

// GDI fallback used in compatibility mode: a periodically captured bitmap of the client
class StaticThumbnailView : public ThumbnailView
{
public:
    explicit StaticThumbnailView(const IWindowManager& WindowManagerReference);
    ~StaticThumbnailView() override;

protected:
    void RefreshThumbnail(const bool ForceRefresh) override;
    void ResizeThumbnail(const int BaseWidth, const int BaseHeight, const int HighlightTop, const int HighlightRight, const int HighlightBottom, const int HighlightLeft) override;
    void PaintContent(const HDC DeviceContext) override;

private:
    void ReleaseBitmap();

    HBITMAP Bitmap = nullptr;
    Size BitmapSize;
    RECT Destination = {};
};

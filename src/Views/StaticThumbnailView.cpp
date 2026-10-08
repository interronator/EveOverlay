#include "Views/StaticThumbnailView.h"

StaticThumbnailView::StaticThumbnailView(const IWindowManager& WindowManagerReference)
    : ThumbnailView(WindowManagerReference)
{
}

StaticThumbnailView::~StaticThumbnailView()
{
    ReleaseBitmap();
}

void StaticThumbnailView::RefreshThumbnail(const bool ForceRefresh)
{
    if (ForceRefresh == false)
    {
        return;
    }

    const HBITMAP Captured = GetWindowManager().GetStaticThumbnail(GetId());
    if (Captured == nullptr)
    {
        return;
    }

    ReleaseBitmap();
    Bitmap = Captured;

    BITMAP Description = {};
    ::GetObjectW(Bitmap, sizeof(Description), &Description);
    BitmapSize = Size{Description.bmWidth, Description.bmHeight};

    Invalidate();
}

void StaticThumbnailView::ResizeThumbnail(const int BaseWidth, const int BaseHeight, const int HighlightTop, const int HighlightRight, const int HighlightBottom, const int HighlightLeft)
{
    Destination = RECT{HighlightLeft, HighlightTop, BaseWidth - HighlightRight, BaseHeight - HighlightBottom};
    Invalidate();
}

void StaticThumbnailView::PaintContent(const HDC DeviceContext)
{
    if (Bitmap == nullptr)
    {
        return;
    }

    const HDC SourceContext = ::CreateCompatibleDC(DeviceContext);
    const HGDIOBJ PreviousBitmap = ::SelectObject(SourceContext, Bitmap);

    const int PreviousMode = ::SetStretchBltMode(DeviceContext, HALFTONE);
    ::SetBrushOrgEx(DeviceContext, 0, 0, nullptr);
    ::StretchBlt(DeviceContext, Destination.left, Destination.top, Destination.right - Destination.left, Destination.bottom - Destination.top,
        SourceContext, 0, 0, BitmapSize.Width, BitmapSize.Height, SRCCOPY);
    ::SetStretchBltMode(DeviceContext, PreviousMode);

    ::SelectObject(SourceContext, PreviousBitmap);
    ::DeleteDC(SourceContext);
}

void StaticThumbnailView::ReleaseBitmap()
{
    if (Bitmap == nullptr)
    {
        return;
    }

    ::DeleteObject(Bitmap);
    Bitmap = nullptr;
}

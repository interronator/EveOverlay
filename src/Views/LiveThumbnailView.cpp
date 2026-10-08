#include "Views/LiveThumbnailView.h"

LiveThumbnailView::LiveThumbnailView(const IWindowManager& WindowManagerReference)
    : ThumbnailView(WindowManagerReference)
{
}

void LiveThumbnailView::RefreshThumbnail(const bool ForceRefresh)
{
    // The old thumbnail is released only after the new one exists, to avoid flicker
    std::unique_ptr<IDwmThumbnail> ObsoleteThumbnail;
    if (ForceRefresh == true && Thumbnail != nullptr)
    {
        if (Thumbnail->IsRegistered() == true)
        {
            Thumbnail->Update();
            return;
        }

        ObsoleteThumbnail = std::move(Thumbnail);
    }

    if (Thumbnail == nullptr)
    {
        RegisterThumbnail();
    }
}

void LiveThumbnailView::ResizeThumbnail(const int BaseWidth, const int BaseHeight, const int HighlightTop, const int HighlightRight, const int HighlightBottom, const int HighlightLeft)
{
    const int Left = HighlightLeft;
    const int Top = HighlightTop;
    const int Right = BaseWidth - HighlightRight;
    const int Bottom = BaseHeight - HighlightBottom;

    if (StartLocation.X == Left && StartLocation.Y == Top && EndLocation.X == Right && EndLocation.Y == Bottom)
    {
        return;
    }

    StartLocation = Point{Left, Top};
    EndLocation = Point{Right, Bottom};

    if (Thumbnail == nullptr)
    {
        return;
    }

    Thumbnail->Move(Left, Top, Right, Bottom);
    Thumbnail->Update();
}

void LiveThumbnailView::RegisterThumbnail()
{
    Thumbnail = GetWindowManager().GetLiveThumbnail(m_hWnd, GetId());
    Thumbnail->Move(StartLocation.X, StartLocation.Y, EndLocation.X, EndLocation.Y);
    Thumbnail->Update();
}

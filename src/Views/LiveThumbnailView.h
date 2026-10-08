#pragma once

#include <memory>

#include "Services/IDwmThumbnail.h"
#include "Views/ThumbnailView.h"

// Thumbnail rendered by DWM
class LiveThumbnailView : public ThumbnailView
{
public:
    explicit LiveThumbnailView(const IWindowManager& WindowManagerReference);

protected:
    void RefreshThumbnail(const bool ForceRefresh) override;
    void ResizeThumbnail(const int BaseWidth, const int BaseHeight, const int HighlightTop, const int HighlightRight, const int HighlightBottom, const int HighlightLeft) override;

private:
    void RegisterThumbnail();

    std::unique_ptr<IDwmThumbnail> Thumbnail;
    Point StartLocation{0, 0};
    Point EndLocation{0, 0};};

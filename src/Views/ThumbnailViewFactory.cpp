#include "Views/ThumbnailViewFactory.h"

#include "Views/LiveThumbnailView.h"
#include "Views/StaticThumbnailView.h"

ThumbnailViewFactory::ThumbnailViewFactory(const IWindowManager& WindowManagerReference, const ThumbnailConfiguration& ConfigurationReference)
    : WindowManagerInstance(WindowManagerReference)
    , Configuration(ConfigurationReference)
{
}

std::unique_ptr<IThumbnailView> ThumbnailViewFactory::Create(const HWND Id, const std::wstring& Title, const Size ThumbnailSize)
{
    if (Configuration.EnableCompatibilityMode == true)
    {
        std::unique_ptr<StaticThumbnailView> View = std::make_unique<StaticThumbnailView>(WindowManagerInstance);
        View->Initialize(Id, Title, ThumbnailSize);
        return View;
    }

    std::unique_ptr<LiveThumbnailView> View = std::make_unique<LiveThumbnailView>(WindowManagerInstance);
    View->Initialize(Id, Title, ThumbnailSize);
    return View;
}

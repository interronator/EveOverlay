#pragma once

#include <memory>
#include <string>

#include "Config/ThumbnailConfiguration.h"
#include "Services/IWindowManager.h"
#include "Views/IThumbnailView.h"

class IThumbnailViewFactory
{
public:
    virtual ~IThumbnailViewFactory() = default;

    virtual std::unique_ptr<IThumbnailView> Create(HWND Id, const std::wstring& Title, Size ThumbnailSize) = 0;
};

class ThumbnailViewFactory : public IThumbnailViewFactory
{
public:
    ThumbnailViewFactory(const IWindowManager& WindowManagerReference, const ThumbnailConfiguration& ConfigurationReference);

    std::unique_ptr<IThumbnailView> Create(const HWND Id, const std::wstring& Title, const Size ThumbnailSize) override;

private:
    const IWindowManager& WindowManagerInstance;
    const ThumbnailConfiguration& Configuration;
};

#pragma once

#include <string>

#include "Application/Signal.h"
#include "Config/ThumbnailConfiguration.h"

class ITabPage
{
public:
    ITabPage() = default;
    ITabPage(const ITabPage&) = delete;
    ITabPage& operator=(const ITabPage&) = delete;
    virtual ~ITabPage() = default;

    Signal<> SettingsChanged;

    virtual const std::string& GetTitle() const = 0;
    virtual const std::string& GetDescription() const = 0;
    virtual void Draw() = 0;
    virtual void LoadFromConfiguration(const ThumbnailConfiguration& Configuration) = 0;
    virtual void StoreToConfiguration(ThumbnailConfiguration& Configuration) const = 0;
};

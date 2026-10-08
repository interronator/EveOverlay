#pragma once

#include <string>

#include "UI/ITabPage.h"
#include "UI/Widgets.h"

class ZoomTab : public ITabPage
{
public:
    const std::string& GetTitle() const override;
    const std::string& GetDescription() const override;
    void Draw() override;
    void LoadFromConfiguration(const ThumbnailConfiguration& Configuration) override;
    void StoreToConfiguration(ThumbnailConfiguration& Configuration) const override;

private:
    static constexpr int MINIMUM_FACTOR = 2;
    static constexpr int MAXIMUM_FACTOR = 10;

    const std::string Title = "Zoom";
    const std::string Description = "Enlarge a preview while the pointer is over it.";

    bool ZoomEnabled = false;
    int ZoomFactor = MINIMUM_FACTOR;
    int AnchorIndex = 0;
    NumberField FactorField;
};

#pragma once

#include <string>

#include "Application/Signal.h"
#include "UI/ITabPage.h"
#include "UI/Widgets.h"

class ThumbnailTab : public ITabPage
{
public:
    Signal<> ThumbnailSizeChanged;

    const std::string& GetTitle() const override;
    const std::string& GetDescription() const override;
    void Draw() override;
    void LoadFromConfiguration(const ThumbnailConfiguration& Configuration) override;
    void StoreToConfiguration(ThumbnailConfiguration& Configuration) const override;

    void SetThumbnailSize(const Size NewSize);
    void SetMoveAll(const bool Enabled);

private:
    static constexpr int MINIMUM_OPACITY = 20;
    static constexpr int MAXIMUM_OPACITY = 100;

    const std::string Title = "Thumbnail";
    const std::string Description = "Opacity and size of the live previews.";

    int OpacityPercent = MAXIMUM_OPACITY;
    bool MoveAll = false;
    bool LockPreviews = false;
    bool SnapEnabled = true;
    int ThumbnailWidth = 0;
    int ThumbnailHeight = 0;
    Size MinimumSize = Size{0, 0};
    Size MaximumSize = Size{99999, 99999};
    NumberField WidthField;
    NumberField HeightField;
};

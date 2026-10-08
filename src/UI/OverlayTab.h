#pragma once

#include <cstdint>
#include <string>

#include <imgui.h>

#include "UI/ITabPage.h"

class OverlayTab : public ITabPage
{
public:
    const std::string& GetTitle() const override;
    const std::string& GetDescription() const override;
    void Draw() override;
    void LoadFromConfiguration(const ThumbnailConfiguration& Configuration) override;
    void StoreToConfiguration(ThumbnailConfiguration& Configuration) const override;

private:
    static uint8_t ToByte(const float Channel);

    // Committed when the mouse is released inside the picker or the popup closes, so dragging does not write the config every frame
    bool DrawColorRow();

    const std::string Title = "Overlay";
    const std::string Description = "Frames and highlights drawn on the previews.";

    bool ShowOverlay = false;
    bool ShowFrames = false;
    bool HighlightActive = false;
    bool ColorDirty = false;
    ImVec4 HighlightColor = ImVec4(0.678f, 1.0f, 0.184f, 1.0f);
};

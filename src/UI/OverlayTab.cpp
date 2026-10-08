#include "UI/OverlayTab.h"

#include <algorithm>

#include "UI/Theme.h"
#include "UI/Widgets.h"

const std::string& OverlayTab::GetTitle() const
{
    return Title;
}

const std::string& OverlayTab::GetDescription() const
{
    return Description;
}

void OverlayTab::Draw()
{
    bool Changed = false;

    Widgets::BeginCard("##Overlay");
    Changed = Widgets::ToggleRow("Show overlay", ShowOverlay) == true || Changed == true;
    Widgets::RowDivider();
    Changed = Widgets::ToggleRow("Show frames", ShowFrames) == true || Changed == true;
    Widgets::RowDivider();
    Changed = Widgets::ToggleRow("Highlight active client", HighlightActive) == true || Changed == true;
    Widgets::EndCard();

    ImGui::BeginDisabled(HighlightActive == false);
    Widgets::BeginCard("##Highlight");
    Changed = DrawColorRow() == true || Changed == true;
    Widgets::EndCard();
    ImGui::EndDisabled();

    if (Changed == true)
    {
        SettingsChanged.Emit();
    }
}

void OverlayTab::LoadFromConfiguration(const ThumbnailConfiguration& Configuration)
{
    ShowOverlay = Configuration.ShowThumbnailOverlays;
    ShowFrames = Configuration.ShowThumbnailFrames;
    HighlightActive = Configuration.EnableActiveClientHighlight;
    HighlightColor = ImVec4(Configuration.ActiveClientHighlightColor.Red / 255.0f, Configuration.ActiveClientHighlightColor.Green / 255.0f, Configuration.ActiveClientHighlightColor.Blue / 255.0f, 1.0f);
}

void OverlayTab::StoreToConfiguration(ThumbnailConfiguration& Configuration) const
{
    Configuration.ShowThumbnailOverlays = ShowOverlay;
    Configuration.ShowThumbnailFrames = ShowFrames;
    Configuration.EnableActiveClientHighlight = HighlightActive;
    Configuration.ActiveClientHighlightColor = Color{255, ToByte(HighlightColor.x), ToByte(HighlightColor.y), ToByte(HighlightColor.z)};
}

uint8_t OverlayTab::ToByte(const float Channel)
{
    return static_cast<uint8_t>(std::clamp(static_cast<int>(Channel * 255.0f + 0.5f), 0, 255));
}

bool OverlayTab::DrawColorRow()
{
    const float SwatchWidth = Theme::Px(96.0f);
    Widgets::RowLabel("Highlight color", SwatchWidth);

    if (ImGui::ColorButton("##Swatch", HighlightColor, ImGuiColorEditFlags_NoAlpha, ImVec2(SwatchWidth, ImGui::GetFrameHeight())) == true)
    {
        ImGui::OpenPopup("##Picker");
    }

    Widgets::EndRow();

    bool Committed = false;
    if (ImGui::BeginPopup("##Picker") == true)
    {
        float Rgb[3] = {HighlightColor.x, HighlightColor.y, HighlightColor.z};
        const ImGuiColorEditFlags PickerFlags = ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_InputRGB;
        if (ImGui::ColorPicker3("##Color", Rgb, PickerFlags) == true)
        {
            HighlightColor = ImVec4(Rgb[0], Rgb[1], Rgb[2], 1.0f);
            ColorDirty = true;
        }

        if (ColorDirty == true && ImGui::IsMouseReleased(ImGuiMouseButton_Left) == true)
        {
            ColorDirty = false;
            Committed = true;
        }

        ImGui::EndPopup();
        return Committed;
    }

    if (ColorDirty == true)
    {
        ColorDirty = false;
        Committed = true;
    }

    return Committed;
}

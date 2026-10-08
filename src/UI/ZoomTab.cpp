#include "UI/ZoomTab.h"

const std::string& ZoomTab::GetTitle() const
{
    return Title;
}

const std::string& ZoomTab::GetDescription() const
{
    return Description;
}

void ZoomTab::Draw()
{
    bool Changed = false;

    Widgets::BeginCard("##Zoom");
    Changed = Widgets::ToggleRow("Zoom on hover", ZoomEnabled) == true || Changed == true;
    Widgets::EndCard();

    ImGui::BeginDisabled(ZoomEnabled == false);

    Widgets::BeginCard("##Factor");
    Changed = Widgets::NumberRow("Zoom factor", FactorField, ZoomFactor, MINIMUM_FACTOR, MAXIMUM_FACTOR, 1) == true || Changed == true;
    Widgets::EndCard();

    Widgets::SectionLabel("ANCHOR");
    Changed = Widgets::AnchorGrid("##Anchor", AnchorIndex) == true || Changed == true;

    ImGui::EndDisabled();

    if (Changed == true)
    {
        SettingsChanged.Emit();
    }
}

void ZoomTab::LoadFromConfiguration(const ThumbnailConfiguration& Configuration)
{
    ZoomEnabled = Configuration.ThumbnailZoomEnabled;
    ZoomFactor = Configuration.ThumbnailZoomFactor;
    AnchorIndex = static_cast<int>(Configuration.ThumbnailZoomAnchor);
}

void ZoomTab::StoreToConfiguration(ThumbnailConfiguration& Configuration) const
{
    Configuration.ThumbnailZoomEnabled = ZoomEnabled;
    Configuration.ThumbnailZoomFactor = ZoomFactor;
    Configuration.ThumbnailZoomAnchor = static_cast<ZoomAnchor>(AnchorIndex);
}

#include "UI/ThumbnailTab.h"

#include <algorithm>
#include <cmath>

#include "UI/Theme.h"

const std::string& ThumbnailTab::GetTitle() const
{
    return Title;
}

const std::string& ThumbnailTab::GetDescription() const
{
    return Description;
}

void ThumbnailTab::Draw()
{
    Widgets::BeginCard("##Appearance");
    const bool OpacityChanged = Widgets::SliderRow("Opacity", OpacityPercent, MINIMUM_OPACITY, MAXIMUM_OPACITY, "%d%%");
    Widgets::EndCard();

    Widgets::SectionLabel("SIZE");
    Widgets::BeginCard("##Size");
    const bool WidthChanged = Widgets::NumberRow("Thumbnail width", WidthField, ThumbnailWidth, MinimumSize.Width, MaximumSize.Width, 10);
    Widgets::RowDivider();
    const bool HeightChanged = Widgets::NumberRow("Thumbnail height", HeightField, ThumbnailHeight, MinimumSize.Height, MaximumSize.Height, 10);
    Widgets::EndCard();

    Widgets::SectionLabel("BEHAVIOR");
    Widgets::BeginCard("##Behavior");
    bool BehaviorChanged = Widgets::ToggleRow("Snap to the nearest thumbnail", SnapEnabled);
    Widgets::RowDivider();
    BehaviorChanged = Widgets::ToggleRow("Move all thumbnails together", MoveAll) == true || BehaviorChanged == true;
    Widgets::RowDivider();
    BehaviorChanged = Widgets::ToggleRow("Lock previews in place", LockPreviews) == true || BehaviorChanged == true;
    Widgets::EndCard();

    if (Organizer != nullptr)
    {
        ImGui::Dummy(ImVec2(0.0f, Theme::Px(6.0f)));
        Widgets::SectionLabel("THUMBNAIL ORGANIZER");
        Organizer->Draw();
    }

    if (WidthChanged == true || HeightChanged == true)
    {
        ThumbnailSizeChanged.Emit();
        return;
    }

    if (OpacityChanged == true || BehaviorChanged == true)
    {
        SettingsChanged.Emit();
    }
}

void ThumbnailTab::LoadFromConfiguration(const ThumbnailConfiguration& Configuration)
{
    const int Percent = static_cast<int>(std::lround(Configuration.ThumbnailOpacity * 100.0));
    OpacityPercent = std::clamp(Percent, MINIMUM_OPACITY, MAXIMUM_OPACITY);

    MoveAll = Configuration.MoveAllThumbnails;
    LockPreviews = Configuration.LockThumbnails;
    SnapEnabled = Configuration.EnableThumbnailSnap;
    MinimumSize = Configuration.ThumbnailMinimumSize;
    MaximumSize = Configuration.ThumbnailMaximumSize;
    SetThumbnailSize(Configuration.ThumbnailSize);
}

void ThumbnailTab::StoreToConfiguration(ThumbnailConfiguration& Configuration) const
{
    Configuration.MoveAllThumbnails = MoveAll;
    Configuration.LockThumbnails = LockPreviews;
    Configuration.EnableThumbnailSnap = SnapEnabled;
    Configuration.ThumbnailOpacity = static_cast<double>(OpacityPercent) / 100.0;
    Configuration.ThumbnailSize = Size{std::clamp(ThumbnailWidth, MinimumSize.Width, MaximumSize.Width), std::clamp(ThumbnailHeight, MinimumSize.Height, MaximumSize.Height)};
}

void ThumbnailTab::SetOrganizer(OrganizerTab* const NewOrganizer)
{
    Organizer = NewOrganizer;
}

void ThumbnailTab::SetThumbnailSize(const Size NewSize)
{
    ThumbnailWidth = std::clamp(NewSize.Width, MinimumSize.Width, MaximumSize.Width);
    ThumbnailHeight = std::clamp(NewSize.Height, MinimumSize.Height, MaximumSize.Height);
}

#include "UI/Widgets.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>

#include "UI/Theme.h"
#include "UI/Tooltips.h"

float Widgets::RowStartY = 0.0f;
const char* Widgets::RowTipText = nullptr;
ImVec2 Widgets::RowTipOrigin = ImVec2(0.0f, 0.0f);
float Widgets::RowTipWidth = 0.0f;
ImGuiID Widgets::RowHoverKey = 0;
double Widgets::RowHoverSince = 0.0;
int Widgets::TooltipFrame = -1;

bool NumberField::Draw(const char* const Id, int& Value, const int Minimum, const int Maximum, const int Step)
{
    ImGui::PushID(Id);

    bool Changed = false;
    const float ButtonSize = ImGui::GetFrameHeight();

    if (Editing == false)
    {
        std::snprintf(Text, sizeof(Text), "%d", Value);
    }

    if (ImGui::Button("-", ImVec2(ButtonSize, ButtonSize)) == true)
    {
        Value = std::clamp(Value - Step, Minimum, Maximum);
        Changed = true;
    }

    ImGui::SameLine(0.0f, Theme::Px(4.0f));
    ImGui::SetNextItemWidth(Theme::Px(64.0f));
    ImGui::InputText("##Value", Text, sizeof(Text), ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_AutoSelectAll);
    Editing = ImGui::IsItemActive();

    if (ImGui::IsItemDeactivatedAfterEdit() == true)
    {
        const int Parsed = std::atoi(Text);
        const int Clamped = std::clamp(Parsed, Minimum, Maximum);
        Changed = Changed == true || Clamped != Value;
        Value = Clamped;
    }

    ImGui::SameLine(0.0f, Theme::Px(4.0f));
    if (ImGui::Button("+", ImVec2(ButtonSize, ButtonSize)) == true)
    {
        Value = std::clamp(Value + Step, Minimum, Maximum);
        Changed = true;
    }

    ImGui::PopID();
    return Changed;
}

float NumberField::GetWidth()
{
    return ImGui::GetFrameHeight() * 2.0f + Theme::Px(64.0f) + Theme::Px(8.0f);
}

SliderInputField::Result SliderInputField::Draw(const char* const Id, int& Value, const int Minimum, const int Maximum)
{
    ImGui::PushID(Id);

    Result Outcome;
    if (Editing == false)
    {
        std::snprintf(Text, sizeof(Text), "%d", Value);
    }

    ImGui::SetNextItemWidth(Theme::Px(SLIDER_WIDTH));
    if (ImGui::SliderInt("##Slider", &Value, Minimum, Maximum, "%d", ImGuiSliderFlags_AlwaysClamp) == true)
    {
        Outcome.Changed = true;
    }

    if (ImGui::IsItemDeactivatedAfterEdit() == true)
    {
        Outcome.Committed = true;
    }

    ImGui::SameLine(0.0f, Theme::Px(8.0f));
    ImGui::SetNextItemWidth(Theme::Px(INPUT_WIDTH));
    ImGui::InputText("##Value", Text, sizeof(Text), ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue);
    Editing = ImGui::IsItemActive();

    if (ImGui::IsItemDeactivatedAfterEdit() == true)
    {
        const int Clamped = std::clamp(std::atoi(Text), Minimum, Maximum);
        Outcome.Changed = Outcome.Changed == true || Clamped != Value;
        Outcome.Committed = true;
        Value = Clamped;
    }

    ImGui::PopID();
    return Outcome;
}

float SliderInputField::GetWidth()
{
    return Theme::Px(SLIDER_WIDTH + INPUT_WIDTH + 8.0f);
}

bool Widgets::BeginCard(const char* const Id)
{
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::SURFACE);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(Theme::Px(16.0f), Theme::Px(6.0f)));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(Theme::Px(8.0f), Theme::Px(0.0f)));
    return ImGui::BeginChild(Id, ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
}

void Widgets::EndCard()
{
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0.0f, Theme::Px(4.0f)));
}

void Widgets::HoverTip(const char* const Text)
{
    if (Text == nullptr || ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip) == false)
    {
        return;
    }

    ShowTip(Text);
}

void Widgets::ShowTip(const char* const Text)
{
    TooltipFrame = ImGui::GetFrameCount();
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(Theme::Px(320.0f));
    ImGui::TextUnformatted(Text);
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
}

void Widgets::ShowRowTip()
{
    if (RowTipText == nullptr)
    {
        return;
    }

    const ImVec2 Maximum(RowTipOrigin.x + RowTipWidth, RowTipOrigin.y + GetRowHeight());
    const ImGuiID Key = ImGui::GetID(RowTipText);
    const bool Hovered = ImGui::IsWindowHovered() == true && ImGui::IsMouseHoveringRect(RowTipOrigin, Maximum) == true;
    if (Hovered == false)
    {
        if (RowHoverKey == Key)
        {
            RowHoverKey = 0;
        }

        return;
    }

    if (RowHoverKey != Key)
    {
        RowHoverKey = Key;
        RowHoverSince = ImGui::GetTime();
    }

    if (TooltipFrame == ImGui::GetFrameCount() || ImGui::GetTime() - RowHoverSince < ROW_TIP_DELAY || ImGui::IsMouseDown(ImGuiMouseButton_Left) == true)
    {
        return;
    }

    ShowTip(RowTipText);
}

void Widgets::RowDivider()
{
    const ImVec2 Position = ImGui::GetCursorScreenPos();
    const float Width = ImGui::GetContentRegionAvail().x;
    ImGui::GetWindowDrawList()->AddLine(Position, ImVec2(Position.x + Width, Position.y), ImGui::GetColorU32(ImGuiCol_Border));
    ImGui::Dummy(ImVec2(0.0f, 1.0f));
}

float Widgets::GetRowHeight()
{
    return ImGui::GetFrameHeight() + Theme::Px(14.0f);
}

void Widgets::SectionLabel(const char* const Text)
{
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::TEXT_DISABLED);
    ImGui::TextUnformatted(Text);
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0.0f, Theme::Px(2.0f)));
}

void Widgets::PageHeading(const char* const Title, const char* const Description)
{
    ImGui::PushFont(Theme::GetBoldFont(), 22.0f);
    ImGui::TextUnformatted(Title);
    ImGui::PopFont();

    ImGui::PushStyleColor(ImGuiCol_Text, Theme::TEXT_DISABLED);
    ImGui::TextUnformatted(Description);
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0.0f, Theme::Px(10.0f)));
}

bool Widgets::ToggleRow(const char* const Label, bool& Value)
{
    ImGui::PushID(Label);

    const float RowHeight = GetRowHeight();
    const float RowWidth = ImGui::GetContentRegionAvail().x;
    const ImVec2 Origin = ImGui::GetCursorScreenPos();

    const bool Pressed = ImGui::InvisibleButton("##Row", ImVec2(RowWidth, RowHeight));
    const bool Hovered = ImGui::IsItemHovered();
    HoverTip(Tooltips::Find(Label));

    if (Pressed == true)
    {
        Value = Value == false;
    }

    const float Target = Value == true ? 1.0f : 0.0f;
    float& Animation = *ImGui::GetStateStorage()->GetFloatRef(ImGui::GetItemID(), Target);
    Animation += (Target - Animation) * std::min(1.0f, ImGui::GetIO().DeltaTime * 20.0f);

    ImDrawList* const DrawList = ImGui::GetWindowDrawList();
    const float CenterY = Origin.y + RowHeight * 0.5f;

    const ImVec4 TextColor = ImGui::GetStyleColorVec4(ImGuiCol_Text);
    const ImVec2 TextSize = ImGui::CalcTextSize(Label);
    DrawList->AddText(ImVec2(Origin.x, CenterY - TextSize.y * 0.5f), ImGui::GetColorU32(TextColor), Label);

    const float TrackHeight = ImGui::GetFrameHeight() * 0.85f;
    const float TrackWidth = TrackHeight * 1.8f;
    const ImVec2 TrackMin(Origin.x + RowWidth - TrackWidth, CenterY - TrackHeight * 0.5f);
    const ImVec2 TrackMax(TrackMin.x + TrackWidth, TrackMin.y + TrackHeight);

    ImVec4 OffColor = Theme::Lift(Theme::SURFACE, 0.12f);
    if (Hovered == true)
    {
        OffColor = Theme::Lift(OffColor, 0.05f);
    }

    const ImVec4 TrackColor = Theme::Mix(OffColor, Theme::ACCENT, Animation);
    DrawList->AddRectFilled(TrackMin, TrackMax, ImGui::GetColorU32(TrackColor), TrackHeight * 0.5f);

    const float Inset = TrackHeight * 0.14f;
    const float KnobRadius = TrackHeight * 0.5f - Inset;
    const float KnobTravel = TrackWidth - TrackHeight;
    const ImVec2 KnobCenter(TrackMin.x + TrackHeight * 0.5f + KnobTravel * Animation, CenterY);
    const ImVec4 KnobColor = Theme::Mix(Theme::TEXT_DISABLED, ImVec4(1.0f, 1.0f, 1.0f, 1.0f), Animation);
    DrawList->AddCircleFilled(KnobCenter, KnobRadius, ImGui::GetColorU32(KnobColor), 24);

    ImGui::PopID();
    return Pressed;
}

void Widgets::RowLabel(const char* const Label, const float ControlWidth)
{
    const float StartX = ImGui::GetCursorPosX();
    const float Available = ImGui::GetContentRegionAvail().x;
    const float RowHeight = GetRowHeight();
    const float StartY = ImGui::GetCursorPosY();
    RowStartY = StartY;
    RowTipText = Tooltips::Find(Label);
    RowTipOrigin = ImGui::GetCursorScreenPos();
    RowTipWidth = Available;

    ImGui::SetCursorPosY(StartY + (RowHeight - ImGui::GetTextLineHeight()) * 0.5f);
    ImGui::TextUnformatted(Label);

    ImGui::SameLine();
    ImGui::SetCursorPosX(StartX + Available - ControlWidth);
    ImGui::SetCursorPosY(StartY + (RowHeight - ImGui::GetFrameHeight()) * 0.5f);
}

void Widgets::EndRow()
{
    ImGui::SetCursorPosY(RowStartY);
    ImGui::Dummy(ImVec2(0.0f, GetRowHeight()));
    ShowRowTip();
}

void Widgets::InfoRow(const char* const Label, const char* const Value)
{
    RowLabel(Label, ImGui::CalcTextSize(Value).x);
    ImGui::AlignTextToFramePadding();
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::TEXT_DISABLED);
    ImGui::TextUnformatted(Value);
    ImGui::PopStyleColor();
    EndRow();
}

bool Widgets::NumberRow(const char* const Label, NumberField& Field, int& Value, const int Minimum, const int Maximum, const int Step)
{
    RowLabel(Label, NumberField::GetWidth());
    const bool Changed = Field.Draw(Label, Value, Minimum, Maximum, Step);
    EndRow();
    return Changed;
}

SliderInputField::Result Widgets::SliderInputRow(const char* const Label, SliderInputField& Field, int& Value, const int Minimum, const int Maximum)
{
    RowLabel(Label, SliderInputField::GetWidth());
    const SliderInputField::Result Outcome = Field.Draw(Label, Value, Minimum, Maximum);
    EndRow();
    return Outcome;
}

bool Widgets::SliderRow(const char* const Label, int& Value, const int Minimum, const int Maximum, const char* const Format)
{
    const float SliderWidth = Theme::Px(220.0f);
    RowLabel(Label, SliderWidth);
    ImGui::SetNextItemWidth(SliderWidth);
    const bool Changed = ImGui::SliderInt("##Slider", &Value, Minimum, Maximum, Format, ImGuiSliderFlags_AlwaysClamp);
    EndRow();
    return Changed;
}

bool Widgets::AnchorGrid(const char* const Id, int& Selected)
{
    ImGui::PushID(Id);

    const float Cell = Theme::Px(34.0f);
    const float Gap = Theme::Px(6.0f);
    const float Pad = Theme::Px(8.0f);
    const float Side = Cell * 3.0f + Gap * 2.0f + Pad * 2.0f;
    const ImVec2 Origin = ImGui::GetCursorScreenPos();
    ImDrawList* const DrawList = ImGui::GetWindowDrawList();

    DrawList->AddRectFilled(Origin, ImVec2(Origin.x + Side, Origin.y + Side), ImGui::GetColorU32(ImGuiCol_FrameBg), Theme::Px(8.0f));
    DrawList->AddRect(Origin, ImVec2(Origin.x + Side, Origin.y + Side), ImGui::GetColorU32(ImGuiCol_Border), Theme::Px(8.0f));

    bool Changed = false;
    for (int Index = 0; Index < 9; Index++)
    {
        const int Column = Index % 3;
        const int Row = Index / 3;
        const ImVec2 CellMin(Origin.x + Pad + Column * (Cell + Gap), Origin.y + Pad + Row * (Cell + Gap));

        ImGui::SetCursorScreenPos(CellMin);
        ImGui::PushID(Index);
        const bool Pressed = ImGui::InvisibleButton("##Cell", ImVec2(Cell, Cell));
        const bool Hovered = ImGui::IsItemHovered();
        HoverTip(ANCHOR_TIPS[Index]);
        ImGui::PopID();

        if (Pressed == true && Selected != Index)
        {
            Selected = Index;
            Changed = true;
        }

        const ImVec2 CellMax(CellMin.x + Cell, CellMin.y + Cell);
        const ImVec2 Center((CellMin.x + CellMax.x) * 0.5f, (CellMin.y + CellMax.y) * 0.5f);

        if (Selected == Index)
        {
            DrawList->AddRectFilled(CellMin, CellMax, ImGui::GetColorU32(Theme::WithAlpha(Theme::ACCENT, 0.25f)), Theme::Px(6.0f));
            DrawList->AddRect(CellMin, CellMax, ImGui::GetColorU32(Theme::ACCENT), Theme::Px(6.0f), 0, Theme::Px(1.5f));
            DrawList->AddCircleFilled(Center, Theme::Px(5.0f), ImGui::GetColorU32(Theme::ACCENT), 16);
            continue;
        }

        if (Hovered == true)
        {
            DrawList->AddRectFilled(CellMin, CellMax, ImGui::GetColorU32(ImGuiCol_ButtonHovered), Theme::Px(6.0f));
        }

        DrawList->AddCircleFilled(Center, Theme::Px(2.5f), ImGui::GetColorU32(Theme::TEXT_DISABLED), 12);
    }

    ImGui::SetCursorScreenPos(ImVec2(Origin.x, Origin.y + Side));
    ImGui::Dummy(ImVec2(Side, 0.0f));
    ImGui::PopID();
    return Changed;
}

float Widgets::GetLayoutCardWidth()
{
    return Theme::Px(104.0f);
}

bool Widgets::LayoutCard(const char* const Id, const GridShape& Shape, const int FilledCount, const bool Selected)
{
    ImGui::PushID(Id);

    const float CardWidth = GetLayoutCardWidth();
    const float CardHeight = Theme::Px(92.0f);
    const float Padding = Theme::Px(8.0f);
    const float CellGap = Theme::Px(2.0f);
    const float LabelHeight = ImGui::GetTextLineHeight() + Theme::Px(4.0f);

    const ImVec2 Minimum = ImGui::GetCursorScreenPos();
    const ImVec2 Maximum(Minimum.x + CardWidth, Minimum.y + CardHeight);

    const bool Pressed = ImGui::InvisibleButton("##Card", ImVec2(CardWidth, CardHeight));
    const bool Hovered = ImGui::IsItemHovered();
    char Tip[160] = {};
    std::snprintf(Tip, sizeof(Tip), Shape.PartialRowFirst == true ? "%d columns by %d rows. The shorter row goes on top, centred over the rest." : "%d columns by %d rows. Click to choose this layout.", Shape.Columns, Shape.Rows);
    HoverTip(Tip);

    ImDrawList* const DrawList = ImGui::GetWindowDrawList();
    const float Rounding = Theme::Px(8.0f);
    DrawList->AddRectFilled(Minimum, Maximum, ImGui::GetColorU32(Selected == true ? Theme::WithAlpha(Theme::ACCENT, 0.16f) : (Hovered == true ? ImGui::GetStyleColorVec4(ImGuiCol_FrameBgHovered) : ImGui::GetStyleColorVec4(ImGuiCol_FrameBg))), Rounding);
    DrawList->AddRect(Minimum, Maximum, ImGui::GetColorU32(Selected == true ? Theme::ACCENT : ImGui::GetStyleColorVec4(ImGuiCol_Border)), Rounding, 0, Selected == true ? Theme::Px(1.5f) : 1.0f);

    const float AreaWidth = CardWidth - Padding * 2.0f;
    const float AreaHeight = CardHeight - Padding * 2.0f - LabelHeight;
    const float FitWidth = (AreaWidth - CellGap * static_cast<float>(Shape.Columns - 1)) / static_cast<float>(Shape.Columns);
    const float FitHeight = (AreaHeight - CellGap * static_cast<float>(Shape.Rows - 1)) / static_cast<float>(Shape.Rows);
    const float CellHeight = std::min(FitHeight, FitWidth * 9.0f / 16.0f);
    const float CellWidth = CellHeight * 16.0f / 9.0f;

    const float GridWidth = CellWidth * static_cast<float>(Shape.Columns) + CellGap * static_cast<float>(Shape.Columns - 1);
    const float GridHeight = CellHeight * static_cast<float>(Shape.Rows) + CellGap * static_cast<float>(Shape.Rows - 1);
    const float GridLeft = Minimum.x + Padding + (AreaWidth - GridWidth) * 0.5f;
    const float GridTop = Minimum.y + Padding + (AreaHeight - GridHeight) * 0.5f;

    const ImU32 FilledColor = ImGui::GetColorU32(Selected == true ? Theme::ACCENT : Theme::TEXT_DISABLED);
    for (int Index = 0; Index < FilledCount; Index++)
    {
        const ThumbnailArranger::CellPosition Position = ThumbnailArranger::GetCellPosition(Index, Shape, FilledCount);
        const float CellLeft = GridLeft + Position.Column * (CellWidth + CellGap);
        const float CellTop = GridTop + static_cast<float>(Position.Row) * (CellHeight + CellGap);
        DrawList->AddRectFilled(ImVec2(CellLeft, CellTop), ImVec2(CellLeft + CellWidth, CellTop + CellHeight), FilledColor, Theme::Px(1.5f));
    }

    char Label[32] = {};
    std::snprintf(Label, sizeof(Label), Shape.PartialRowFirst == true ? "%d x %d top" : "%d x %d", Shape.Columns, Shape.Rows);
    const ImVec2 LabelSize = ImGui::CalcTextSize(Label);
    DrawList->AddText(ImVec2(Minimum.x + (CardWidth - LabelSize.x) * 0.5f, Maximum.y - Padding - LabelSize.y), ImGui::GetColorU32(Selected == true || Hovered == true ? Theme::TEXT : Theme::TEXT_DISABLED), Label);

    ImGui::PopID();
    return Pressed;
}

bool Widgets::IsLinkClicked(const char* const Text)
{
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::ACCENT);
    ImGui::TextUnformatted(Text);
    ImGui::PopStyleColor();

    const bool Hovered = ImGui::IsItemHovered();
    if (Hovered == true)
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        const ImVec2 Minimum = ImGui::GetItemRectMin();
        const ImVec2 Maximum = ImGui::GetItemRectMax();
        ImGui::GetWindowDrawList()->AddLine(ImVec2(Minimum.x, Maximum.y), Maximum, ImGui::GetColorU32(Theme::ACCENT));
    }

    return Hovered == true && ImGui::IsMouseClicked(ImGuiMouseButton_Left) == true;
}

#pragma once

#include <imgui.h>

#include "Config/ThumbnailArrangement.h"

// Persistent state of one numeric input; the typed text is only applied when the field loses focus
class NumberField
{
public:
    bool Draw(const char* const Id, int& Value, const int Minimum, const int Maximum, const int Step);

    static float GetWidth();

private:
    char Text[16] = {};
    bool Editing = false;
};

// A slider with a typed-value box beside it. Changed is true while the value moves (for live previews); Committed is true once
// a drag ends or typed text is accepted, which is when the value should be saved.
class SliderInputField
{
public:
    struct Result
    {
        bool Changed = false;
        bool Committed = false;
    };

    Result Draw(const char* const Id, int& Value, const int Minimum, const int Maximum);

    static float GetWidth();

private:
    static constexpr float SLIDER_WIDTH = 150.0f;
    static constexpr float INPUT_WIDTH = 64.0f;

    char Text[16] = {};
    bool Editing = false;
};

class Widgets
{
public:
    static bool BeginCard(const char* const Id);
    static void EndCard();

    // Tooltip for the item submitted just before; the standard hover delay keeps it from flashing past
    static void HoverTip(const char* const Text);

    static void ShowTip(const char* const Text);

    // Rows are not single items, so hovering is tested against the row's rectangle. A control that showed its own tip wins.
    static void ShowRowTip();

    static void RowDivider();
    static float GetRowHeight();
    static void SectionLabel(const char* const Text);
    static void PageHeading(const char* const Title, const char* const Description);

    // A whole-row switch; returns true when the value was flipped
    static bool ToggleRow(const char* const Label, bool& Value);

    // Draws the label on the left and reserves the right edge for a control of the given width
    static void RowLabel(const char* const Label, const float ControlWidth);

    static void EndRow();
    static void InfoRow(const char* const Label, const char* const Value);
    static bool NumberRow(const char* const Label, NumberField& Field, int& Value, const int Minimum, const int Maximum, const int Step);
    static SliderInputField::Result SliderInputRow(const char* const Label, SliderInputField& Field, int& Value, const int Minimum, const int Maximum);
    static bool SliderRow(const char* const Label, int& Value, const int Minimum, const int Maximum, const char* const Format);

    // Nine-cell picker; Selected is the row-major index of the chosen cell
    static bool AnchorGrid(const char* const Id, int& Selected);

    static float GetLayoutCardWidth();

    // A selectable card previewing a grid; the first FilledCount cells are solid and the rest are empty slots
    static bool LayoutCard(const char* const Id, const GridShape& Shape, const int FilledCount, const bool Selected);

    static bool IsLinkClicked(const char* const Text);

private:
    static constexpr const char* ANCHOR_TIPS[9] = {"Grow the zoomed preview toward the bottom right", "Grow toward the bottom, centred", "Grow toward the bottom left", "Grow toward the right, centred", "Grow from the centre in all directions", "Grow toward the left, centred", "Grow toward the top right", "Grow toward the top, centred", "Grow toward the top left"};
    static constexpr double ROW_TIP_DELAY = 0.35;

    static float RowStartY;
    static const char* RowTipText;
    static ImVec2 RowTipOrigin;
    static float RowTipWidth;
    static ImGuiID RowHoverKey;
    static double RowHoverSince;
    static int TooltipFrame;
};

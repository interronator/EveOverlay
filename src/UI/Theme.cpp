#include "UI/Theme.h"

ImVec4 Theme::BACKGROUND = ImVec4(0.059f, 0.067f, 0.082f, 1.0f);
ImVec4 Theme::SURFACE = ImVec4(0.082f, 0.094f, 0.125f, 1.0f);
ImVec4 Theme::PANE = ImVec4(0.043f, 0.051f, 0.067f, 1.0f);
ImVec4 Theme::BORDER = ImVec4(0.15f, 0.17f, 0.21f, 1.0f);
ImVec4 Theme::ACCENT = ImVec4(0.851f, 0.467f, 0.341f, 1.0f);
ImVec4 Theme::TEXT = ImVec4(0.90f, 0.91f, 0.93f, 1.0f);
ImVec4 Theme::TEXT_DISABLED = ImVec4(0.54f, 0.57f, 0.64f, 1.0f);

bool Theme::DarkTheme = true;
ImFont* Theme::RegularFont = nullptr;
ImFont* Theme::BoldFont = nullptr;

ImVec4 Theme::Shade(const ImVec4& Source, const float Amount)
{
    return ImVec4(Clamp(Source.x + Amount), Clamp(Source.y + Amount), Clamp(Source.z + Amount), Source.w);
}

ImVec4 Theme::WithAlpha(const ImVec4& Source, const float Alpha)
{
    return ImVec4(Source.x, Source.y, Source.z, Alpha);
}

ImVec4 Theme::Mix(const ImVec4& From, const ImVec4& To, const float Amount)
{
    return ImVec4(From.x + (To.x - From.x) * Amount, From.y + (To.y - From.y) * Amount, From.z + (To.z - From.z) * Amount, From.w + (To.w - From.w) * Amount);
}

float Theme::Px(const float Value)
{
    return Value * ImGui::GetStyle().FontScaleDpi;
}

void Theme::SetFonts(ImFont* const Regular, ImFont* const Bold)
{
    RegularFont = Regular;
    BoldFont = Bold;
}

ImFont* Theme::GetRegularFont()
{
    return RegularFont;
}

ImFont* Theme::GetBoldFont()
{
    return BoldFont;
}

void Theme::Apply(const float Scale)
{
    ImGuiStyle& Style = ImGui::GetStyle();
    Style = ImGuiStyle();
    if (DarkTheme == true)
    {
        ImGui::StyleColorsDark(&Style);
    }
    else
    {
        ImGui::StyleColorsLight(&Style);
    }

    Style.FontSizeBase = BASE_FONT_SIZE;
    Style.WindowRounding = 0.0f;
    Style.WindowBorderSize = 0.0f;
    Style.FrameRounding = 6.0f;
    Style.ChildRounding = 8.0f;
    Style.PopupRounding = 8.0f;
    Style.GrabRounding = 6.0f;
    Style.FrameBorderSize = 0.0f;
    Style.ChildBorderSize = 1.0f;
    Style.PopupBorderSize = 1.0f;
    Style.FramePadding = ImVec2(9.0f, 5.0f);
    Style.ItemSpacing = ImVec2(8.0f, 8.0f);
    Style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
    Style.ScrollbarSize = 10.0f;
    Style.ScrollbarRounding = 6.0f;
    Style.GrabMinSize = 14.0f;

    ImVec4* const Colors = Style.Colors;
    Colors[ImGuiCol_Text] = TEXT;
    Colors[ImGuiCol_TextDisabled] = TEXT_DISABLED;
    Colors[ImGuiCol_WindowBg] = BACKGROUND;
    Colors[ImGuiCol_ChildBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    Colors[ImGuiCol_PopupBg] = WithAlpha(SURFACE, 0.99f);
    Colors[ImGuiCol_Border] = BORDER;
    Colors[ImGuiCol_FrameBg] = PANE;
    Colors[ImGuiCol_FrameBgHovered] = Lift(PANE, 0.05f);
    Colors[ImGuiCol_FrameBgActive] = Lift(PANE, 0.08f);
    Colors[ImGuiCol_Button] = Lift(SURFACE, 0.03f);
    Colors[ImGuiCol_ButtonHovered] = Lift(SURFACE, 0.08f);
    Colors[ImGuiCol_ButtonActive] = Lift(SURFACE, 0.12f);
    Colors[ImGuiCol_Header] = WithAlpha(ACCENT, 0.22f);
    Colors[ImGuiCol_HeaderHovered] = Lift(SURFACE, 0.10f);
    Colors[ImGuiCol_HeaderActive] = WithAlpha(ACCENT, 0.35f);
    Colors[ImGuiCol_CheckMark] = ACCENT;
    Colors[ImGuiCol_SliderGrab] = ACCENT;
    Colors[ImGuiCol_SliderGrabActive] = Lift(ACCENT, 0.1f);
    Colors[ImGuiCol_Separator] = BORDER;
    Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    Colors[ImGuiCol_ScrollbarGrab] = Lift(SURFACE, 0.1f);
    Colors[ImGuiCol_ScrollbarGrabHovered] = Lift(SURFACE, 0.16f);
    Colors[ImGuiCol_ScrollbarGrabActive] = Lift(SURFACE, 0.22f);
    Colors[ImGuiCol_TextSelectedBg] = WithAlpha(ACCENT, 0.43f);
    Colors[ImGuiCol_NavCursor] = WithAlpha(ACCENT, 0.8f);

    Style.ScaleAllSizes(Scale);
    Style.FontScaleDpi = Scale;
}

float Theme::Clamp(const float Value)
{
    if (Value < 0.0f)
    {
        return 0.0f;
    }

    if (Value > 1.0f)
    {
        return 1.0f;
    }

    return Value;
}

ImVec4 Theme::Lift(const ImVec4& Source, const float Amount)
{
    return Shade(Source, DarkTheme == true ? Amount : -Amount);
}

bool Theme::IsDark()
{
    return DarkTheme;
}

void Theme::SetDark(const bool Dark)
{
    DarkTheme = Dark;
    if (Dark == true)
    {
        BACKGROUND = ImVec4(0.059f, 0.067f, 0.082f, 1.0f);
        SURFACE = ImVec4(0.082f, 0.094f, 0.125f, 1.0f);
        PANE = ImVec4(0.043f, 0.051f, 0.067f, 1.0f);
        BORDER = ImVec4(0.15f, 0.17f, 0.21f, 1.0f);
        ACCENT = ImVec4(0.851f, 0.467f, 0.341f, 1.0f);
        TEXT = ImVec4(0.90f, 0.91f, 0.93f, 1.0f);
        TEXT_DISABLED = ImVec4(0.54f, 0.57f, 0.64f, 1.0f);
        return;
    }

    BACKGROUND = ImVec4(0.93f, 0.94f, 0.96f, 1.0f);
    SURFACE = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    PANE = ImVec4(0.90f, 0.91f, 0.94f, 1.0f);
    BORDER = ImVec4(0.80f, 0.82f, 0.87f, 1.0f);
    ACCENT = ImVec4(0.80f, 0.38f, 0.25f, 1.0f);
    TEXT = ImVec4(0.10f, 0.12f, 0.16f, 1.0f);
    TEXT_DISABLED = ImVec4(0.42f, 0.45f, 0.52f, 1.0f);
}

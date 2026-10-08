#pragma once

#include <imgui.h>

class Theme
{
public:
    static ImVec4 BACKGROUND;
    static ImVec4 SURFACE;
    static ImVec4 PANE;
    static ImVec4 BORDER;
    static ImVec4 ACCENT;
    static ImVec4 TEXT;
    static ImVec4 TEXT_DISABLED;

    static ImVec4 Shade(const ImVec4& Source, const float Amount);
    // Moves a colour away from the background: lighter on the dark theme, darker on the light one
    static ImVec4 Lift(const ImVec4& Source, const float Amount);
    static ImVec4 WithAlpha(const ImVec4& Source, const float Alpha);
    static ImVec4 Mix(const ImVec4& From, const ImVec4& To, const float Amount);
    static float Px(const float Value);
    static void SetFonts(ImFont* const Regular, ImFont* const Bold);
    static ImFont* GetRegularFont();
    static ImFont* GetBoldFont();
    static void SetDark(const bool Dark);
    static bool IsDark();
    static void Apply(const float Scale);

private:
    static constexpr float BASE_FONT_SIZE = 15.0f;

    static float Clamp(const float Value);

    static bool DarkTheme;
    static ImFont* RegularFont;
    static ImFont* BoldFont;
};

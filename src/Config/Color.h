#pragma once

#include <cstdint>
#include <string>

#include <Windows.h>

// Reads and writes the same text forms as System.Drawing.ColorConverter so existing config files keep working
struct Color
{
    uint8_t Alpha = 255;
    uint8_t Red = 0;
    uint8_t Green = 0;
    uint8_t Blue = 0;

    static Color FromRgb(const uint32_t Rgb);
    static bool TryParse(const std::string& Text, Color* const Result);

    bool operator==(const Color& Other) const = default;

    COLORREF ToColorRef() const;
    std::string ToString() const;

private:
    struct NamedColor
    {
        const char* Name;
        uint32_t Rgb;
    };

    static bool TryParseHex(const std::string& Digits, Color* const Result);
    static bool TryParseComponents(const std::string& Text, Color* const Result);
    static bool TryParseName(const std::string& Name, Color* const Result);

    static const NamedColor NAMED_COLORS[];
};

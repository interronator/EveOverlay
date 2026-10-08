#pragma once

#include <string>

struct Point
{
    int X = 0;
    int Y = 0;

    static bool TryParse(const std::string& Text, Point* const Result);
    std::string ToString() const;
};

struct Size
{
    int Width = 0;
    int Height = 0;

    static bool TryParse(const std::string& Text, Size* const Result);
    std::string ToString() const;
};

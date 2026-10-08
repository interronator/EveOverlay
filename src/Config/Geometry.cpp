#include "Config/Geometry.h"

#include <vector>

#include "Config/TextUtil.h"

bool Point::TryParse(const std::string& Text, Point* const Result)
{
    std::vector<int> Values;
    if (TextUtil::TryParseIntegerList(Text, 2, &Values) == false)
    {
        return false;
    }

    *Result = Point{Values[0], Values[1]};
    return true;
}

std::string Point::ToString() const
{
    return std::to_string(X) + ", " + std::to_string(Y);
}

bool Size::TryParse(const std::string& Text, Size* const Result)
{
    std::vector<int> Values;
    if (TextUtil::TryParseIntegerList(Text, 2, &Values) == false)
    {
        return false;
    }

    *Result = Size{Values[0], Values[1]};
    return true;
}

std::string Size::ToString() const
{
    return std::to_string(Width) + ", " + std::to_string(Height);
}

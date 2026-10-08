#include "Universe/SystemListing.h"

#include <algorithm>

#include "Config/TextUtil.h"

SystemListing SystemListing::Build(const UniverseData& Data)
{
    std::vector<int> Order;
    for (size_t Index = 0; Index < Data.Count(); Index++)
    {
        Order.push_back(static_cast<int>(Index));
    }

    std::sort(Order.begin(), Order.end(), [&Data](const int Left, const int Right)
    {
        return TextUtil::CompareIgnoreCase(Data.Get(Left).Name, Data.Get(Right).Name) < 0;
    });

    SystemListing Result;
    for (const int Index : Order)
    {
        Result.Names.push_back(Data.Get(Index).Name);
        Result.Regions.push_back(Data.Get(Index).Region);
        Result.RegionNames.push_back(Data.Get(Index).Region);
    }

    std::sort(Result.RegionNames.begin(), Result.RegionNames.end(), [](const std::string& Left, const std::string& Right)
    {
        return TextUtil::CompareIgnoreCase(Left, Right) < 0;
    });
    Result.RegionNames.erase(std::unique(Result.RegionNames.begin(), Result.RegionNames.end()), Result.RegionNames.end());
    return Result;
}

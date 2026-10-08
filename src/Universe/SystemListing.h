#pragma once

#include <string>
#include <vector>

#include "Universe/UniverseData.h"

// Alphabetical system names with the region of each (Regions[Index] belongs to Names[Index]), plus the distinct region names
struct SystemListing
{
    static SystemListing Build(const UniverseData& Data);

    std::vector<std::string> Names;
    std::vector<std::string> Regions;
    std::vector<std::string> RegionNames;
};

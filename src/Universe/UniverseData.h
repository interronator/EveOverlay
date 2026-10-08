#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

struct SolarSystem
{
    std::string Name;
    std::string Region;
    std::string SovHolder;
    double Security = 0.0;
    std::vector<int> Neighbours;
};

class UniverseData
{
public:
    static constexpr int NOT_FOUND = -1;

    bool Load(const std::filesystem::path& CsvPath);
    int Find(const std::string& Name) const;
    const SolarSystem& Get(const int Index) const;
    size_t Count() const;

private:
    static constexpr size_t FIELD_NAME = 0;
    static constexpr size_t FIELD_REGION = 1;
    static constexpr size_t FIELD_SECURITY = 2;
    static constexpr size_t FIELD_SOV = 3;
    static constexpr size_t FIELD_GATES = 5;
    static constexpr size_t FIELD_COUNT = 6;

    static std::string StripByteOrderMark(const std::string& Text);

    void AddLink(const int From, const int To);
    void LinkOneWay(const int From, const int To);

    std::vector<SolarSystem> Systems;
    std::unordered_map<std::string, int> NameToIndex;
};

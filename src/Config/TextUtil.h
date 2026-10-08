#pragma once

#include <string>
#include <vector>

class TextUtil
{
public:
    static std::string ToUtf8(const std::wstring& Text);
    static std::wstring FromUtf8(const std::string& Text);
    static std::string Trim(const std::string& Text);
    static std::vector<std::string> Split(const std::string& Text, const char Separator);
    static bool EqualsIgnoreCase(const std::string& Left, const std::string& Right);
    static int CompareIgnoreCase(const std::string& Left, const std::string& Right);
    static std::string ToLower(std::string Text);
    static std::wstring ToLower(std::wstring Text);
    static bool TryParseInt(const std::string& Text, int* const Value, const int Base = 10);
    static bool TryParseIntegerList(const std::string& Text, const size_t Count, std::vector<int>* const Values);
};

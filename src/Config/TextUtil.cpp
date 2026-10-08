#include "Config/TextUtil.h"

#include <cctype>
#include <charconv>
#include <cwctype>

#include <Windows.h>

std::string TextUtil::ToUtf8(const std::wstring& Text)
{
    if (Text.empty() == true)
    {
        return std::string();
    }

    const int Length = ::WideCharToMultiByte(CP_UTF8, 0, Text.data(), static_cast<int>(Text.size()), nullptr, 0, nullptr, nullptr);
    std::string Result(static_cast<size_t>(Length), '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, Text.data(), static_cast<int>(Text.size()), Result.data(), Length, nullptr, nullptr);
    return Result;
}

std::wstring TextUtil::FromUtf8(const std::string& Text)
{
    if (Text.empty() == true)
    {
        return std::wstring();
    }

    const int Length = ::MultiByteToWideChar(CP_UTF8, 0, Text.data(), static_cast<int>(Text.size()), nullptr, 0);
    std::wstring Result(static_cast<size_t>(Length), L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0, Text.data(), static_cast<int>(Text.size()), Result.data(), Length);
    return Result;
}

std::string TextUtil::Trim(const std::string& Text)
{
    const char* const WHITESPACE = " \t\r\n";
    const size_t First = Text.find_first_not_of(WHITESPACE);
    if (First == std::string::npos)
    {
        return std::string();
    }

    const size_t Last = Text.find_last_not_of(WHITESPACE);
    return Text.substr(First, Last - First + 1);
}

std::vector<std::string> TextUtil::Split(const std::string& Text, const char Separator)
{
    std::vector<std::string> Parts;

    size_t Start = 0;
    while (true)
    {
        const size_t End = Text.find(Separator, Start);
        if (End == std::string::npos)
        {
            Parts.push_back(Trim(Text.substr(Start)));
            return Parts;
        }

        Parts.push_back(Trim(Text.substr(Start, End - Start)));
        Start = End + 1;
    }
}

bool TextUtil::EqualsIgnoreCase(const std::string& Left, const std::string& Right)
{
    return ::_stricmp(Left.c_str(), Right.c_str()) == 0;
}

bool TextUtil::TryParseInt(const std::string& Text, int* const Value, const int Base)
{
    const std::string Trimmed = Trim(Text);
    if (Trimmed.empty() == true)
    {
        return false;
    }

    const char* const Begin = Trimmed.data();
    const char* const End = Begin + Trimmed.size();
    const std::from_chars_result ParseResult = std::from_chars(Begin, End, *Value, Base);
    return ParseResult.ec == std::errc() && ParseResult.ptr == End;
}

bool TextUtil::TryParseIntegerList(const std::string& Text, const size_t Count, std::vector<int>* const Values)
{
    const std::vector<std::string> Parts = Split(Text, ',');
    if (Parts.size() != Count)
    {
        return false;
    }

    Values->clear();
    for (const std::string& Part : Parts)
    {
        int Value = 0;
        if (TryParseInt(Part, &Value) == false)
        {
            return false;
        }

        Values->push_back(Value);
    }

    return true;
}

int TextUtil::CompareIgnoreCase(const std::string& Left, const std::string& Right)
{
    return ::_stricmp(Left.c_str(), Right.c_str());
}

std::string TextUtil::ToLower(std::string Text)
{
    for (char& Character : Text)
    {
        Character = static_cast<char>(::tolower(static_cast<unsigned char>(Character)));
    }

    return Text;
}

std::wstring TextUtil::ToLower(std::wstring Text)
{
    for (wchar_t& Character : Text)
    {
        Character = static_cast<wchar_t>(::towlower(Character));
    }

    return Text;
}

#pragma once

// Help text for the labelled rows and buttons of the settings pages, looked up by the visible label
class Tooltips
{
public:
    static const char* Find(const char* const Label);

private:
    struct Entry
    {
        const char* Label;
        const char* Text;
    };

    static const Entry ENTRIES[];
};

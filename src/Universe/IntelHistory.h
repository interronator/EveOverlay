#pragma once

#include <string>

struct IntelHistoryEntry
{
    std::string Time;
    std::string System;
    std::string Channel;
    std::string Sender;
    std::string Text;
    int Jumps = 0;
    bool Priority = false;
};

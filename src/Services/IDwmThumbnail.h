#pragma once

#include <Windows.h>

class IDwmThumbnail
{
public:
    virtual ~IDwmThumbnail() = default;

    virtual void Register(HWND Destination, HWND Source) = 0;
    virtual void Unregister() = 0;
    virtual void Move(int Left, int Top, int Right, int Bottom) = 0;
    virtual void Update() = 0;
    virtual bool IsRegistered() const = 0;
};

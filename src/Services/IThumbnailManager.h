#pragma once

#include <string>
#include <vector>

#include "Application/Signal.h"
#include "Config/Geometry.h"
#include "Config/ThumbnailArrangement.h"

class IThumbnailManager
{
public:
    virtual ~IThumbnailManager() = default;

    virtual void Start() = 0;
    virtual void Stop() = 0;
    virtual void UpdateThumbnailsSize() = 0;
    virtual void UpdateThumbnailFrames() = 0;
    virtual void ArrangeThumbnails(const ThumbnailArrangement& Arrangement) = 0;

    // Registers the client, cycle group and global hotkeys again after any of them changed
    virtual void UpdateHotkeys() = 0;

    // Added titles, removed titles
    Signal<const std::vector<std::wstring>&, const std::vector<std::wstring>&> ThumbnailListUpdated;
    Signal<Size> ThumbnailActiveSizeUpdated;
};

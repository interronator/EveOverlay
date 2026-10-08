#include "UI/Tooltips.h"

#include <cstring>

const Tooltips::Entry Tooltips::ENTRIES[] = {
    {"Minimize to system tray", "Closing or minimizing the window hides it in the system tray instead of the taskbar. Use the tray icon to bring it back or exit."},
    {"Track client locations", "Remember where each EVE client window is and put it back there when the client reopens."},
    {"Unique layout for each EVE client", "Keep a separate set of preview positions for every EVE client you are playing, instead of one shared layout."},
    {"Light theme", "Switch the settings window between the dark and the light colour scheme. The Intel Watcher map keeps its own look."},
    {"Keep this window on top", "Keep this settings window above other windows. Off by default."},
    {"Previews always on top", "Keep the preview windows above other windows, including the game."},
    {"Hide preview of active EVE client", "Don't show a preview of the client you are currently playing."},
    {"Hide previews when EVE client is not active", "Hide every preview while neither an EVE client nor a preview has focus."},
    {"Minimize inactive EVE clients", "Minimize a client when you switch to another one. Priority clients are left alone."},

    {"Opacity", "How see-through the previews are. Hovering a preview makes it fully opaque."},
    {"Thumbnail width", "Width of every preview in pixels."},
    {"Thumbnail height", "Height of every preview in pixels."},
    {"Snap to the nearest thumbnail", "When you release a dragged preview, pull it against the closest corner of another preview. Only works while preview frames are off."},
    {"Move all thumbnails together", "Dragging one preview moves all of them, and every preview is highlighted while you drag."},

    {"Enable Thumbnail Organizer", "Turn the organizer on to arrange previews into grids and to use layout presets."},
    {"Detect open clients", "Use the number of EVE clients that are open right now. Turn off, or open no clients, to type a number yourself."},
    {"Number of clients", "How many clients to plan the layout for. Use this to prepare layouts and presets without opening the game."},
    {"Clients", "Open EVE clients whose previews are not force-hidden."},
    {"Spacing between previews", "Gap in pixels between neighbouring previews."},
    {"Start X", "Screen position of the left edge of the first column. Can be negative for monitors left of the main one."},
    {"Start Y", "Screen position of the top edge of the first row. Can be negative for monitors above the main one."},

    {"Zoom on hover", "Enlarge a preview while the mouse pointer is over it."},
    {"Zoom factor", "How many times larger a hovered preview becomes."},

    {"Show overlay", "Draw the client window title (the character name) on each preview."},
    {"Show frames", "Give the previews a normal window border so they can be resized and moved like windows. Turns snapping off."},
    {"Highlight active client", "Draw a coloured border around the preview of the client you are playing."},
    {"Highlight color", "Colour of the highlight border. Also used while moving all thumbnails together."},

    {"Show map", "Show the Intel Watcher map window while an EVE client is open."},
    {"Smart map (only show reported routes)", "Only draw systems and routes that have recently been reported in the intel channel."},
    {"Region", "Narrow the system list to one region."},
    {"System", "The system the map is centred on. Type in the box to filter the list."},
    {"Jumps", "How many jumps away from the chosen system the map reaches."},
    {"Intel channel", "Name of the in-game chat channel to read intel reports from. Channels that report a system are remembered in the list."},
    {"Map size", "Size of the map window in pixels."},
    {"Map always on top", "Keep the map above other windows."},
    {"Highlight time (seconds)", "How long a reported system keeps flashing on the map."},
    {"Play a sound when a system in range is reported", "Play the alert sound whenever intel reports a system within the jump range."},
    {"Volume", "Loudness of the alert sound."},
    {"Sound", "Choose the alert sound, go back to the built-in one, or play it to check the volume."},

    {"Account nickname", "A friendly name for the account that owns the selected user file, shown in the file lists."},
    {"Settings folder", "The EVE settings folder whose profiles are synced."},
    {"User file", "The user settings profile to copy to your other accounts."},
    {"Character file", "The character settings profile to copy to your other accounts."},
};

const char* Tooltips::Find(const char* const Label)
{
    for (const Entry& Current : ENTRIES)
    {
        if (std::strcmp(Current.Label, Label) == 0)
        {
            return Current.Text;
        }
    }

    return nullptr;
}

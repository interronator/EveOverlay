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
    {"Intel channel", "Chat channel(s) to read intel reports from. Open the dropdown to tick channels found in your chat logs, or type a name. Several channels can be watched at once, separated by commas."},
    {"Map size", "Size of the map window in pixels."},
    {"Map always on top", "Keep the map above other windows."},
    {"Highlight time (seconds)", "How long a reported system keeps flashing on the map."},
    {"Play a sound when a system in range is reported", "Play the alert sound whenever intel reports a system within the jump range."},
    {"Volume", "Loudness of the alert sound."},
    {"Sound", "Choose the alert sound, go back to the built-in one, or play it to check the volume."},
    {"Start where my previews are", "When most of your previews are already grouped together, Apply layout starts the grid at that group instead of the Start X and Y below, and keeps it on screen. Start X and Y are used when no group is found."},
    {"Copy the ship list to the clipboard", "After a scan is read, put the ships and how many there are on the clipboard, one per line like 2x Hurricane, ready to paste into chat. This replaces what was on the clipboard."},
    {"Ship list", "Copy the ships from the last scan to the clipboard, one per line like 2x Hurricane."},
    {"Use Character local", "Move the map's centre to the system a character is in, read from that character's Local chat log. Wormhole systems are not on the map and are ignored."},
    {"Character to use", "Whose Local chat log to read. The list fills in from the EVE clients you open. Any character follows whichever logged-in character moved last."},
    {"Quieter sound for distant systems", "Play the alert at full volume for your own system and quieter the further away the reported system is, down to 40% at the edge of the map."},
    {"Ignore clear reports and questions", "Skip a system that is followed by clr, clear or nv, and ignore any message that is a question or asks for a status."},
    {"Priority keywords", "Words or phrases, separated by commas. A report containing one of them plays the priority sound at full volume and is marked in the list below."},
    {"Priority sound", "Sound for reports that contain a priority keyword. Leave on the alert sound to just get the full-volume version."},

    {"Show or hide all previews", "Press this to hide every preview at once, and again to bring them back."},
    {"Minimize all clients", "Minimize every EVE client except the priority ones."},
    {"Show the timer window", "Show a small panel over the game with your running timers and the d-scan age."},
    {"Show d-scan age", "Count up from your last directional scan in the timer window."},
    {"Mark d-scan taken", "Press this right after a d-scan to start the age counter from zero."},
    {"Start quick timer", "Press this to start a countdown of the quick timer length."},
    {"Quick timer length (seconds)", "How long the quick timer counts down."},
    {"Play a sound when a timer ends", "Play the timer sound when a countdown reaches zero."},
    {"Timer volume", "Loudness of the timer sound."},
    {"Timer sound", "Choose the sound for finished timers, or play it to check the volume."},
    {"Read copied d-scans automatically", "When you copy the results of a directional scan, show a summary over the game. Only text shaped like a scan is looked at, and nothing leaves your computer."},
    {"Show the summary for (seconds)", "How long the d-scan summary stays on screen."},
    {"Start the scan age when a d-scan is read", "Reading a d-scan also resets the scan age counter in the timer window."},
    {"Lock previews in place", "Stop the right mouse button from moving or resizing the previews, so a stray drag cannot ruin your layout. Clicking to switch client still works."},
    {"Flash a preview when its character is attacked", "Watch the game logs and make a character's preview blink red when that character takes damage or is warp scrambled. The client you are playing is never flashed."},
    {"Incoming damage", "Alert when the character is being shot at, by players or by NPCs."},
    {"Warp scramble or disruption", "Alert when something tries to scramble or disrupt the character's warp drive."},
    {"Flash duration (seconds)", "How long the preview keeps blinking after the last attack."},
    {"Play a sound for attacks", "Play the attack sound when a preview starts flashing. At most once every few seconds."},
    {"Attack volume", "Loudness of the attack sound."},
    {"Attack sound", "Choose the sound for attacks, or play it to check the volume."},

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

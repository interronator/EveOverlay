<#
.SYNOPSIS
    End-to-end QA for EveOverlay against fake EVE clients (tests/qa/FakeClient.cpp, built as ExeFile.exe).

.DESCRIPTION
    Drives the real app with real mouse and keyboard input, so it needs an interactive desktop and takes over the
    mouse for a few seconds per section. Every click is preceded by a check that the cursor is over the thumbnail
    under test. The config file next to EveOverlay.exe is overwritten and deleted. Do not run it while real EVE
    clients are open: any process named ExeFile.exe is stopped.

    Build first:  cmake --build --preset release
    Run:          powershell -File tests\qa\Run-Qa.ps1 [-Section all|input|snap|hide|resize|minimize|layout|rename|leak]
#>
param(
    [string]$Section = "all",
    [string]$BuildDirectory = (Join-Path $PSScriptRoot "..\..\build"),
    [string]$BinariesDirectory = (Join-Path $PSScriptRoot "..\..\Binaries"),
    [string]$Configuration = "Release"
)

$AppPath = (Resolve-Path (Join-Path $BinariesDirectory "$Configuration\EveOverlay.exe")).Path
$ConfigPath = Join-Path (Split-Path $AppPath) "Eve Overlay.json"
$FakePath = (Resolve-Path (Join-Path $BuildDirectory "qa\$Configuration\ExeFile.exe")).Path

Add-Type -TypeDefinition @"
using System; using System.Text; using System.Runtime.InteropServices;
public class Qa {
 [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X, Y; }
 [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
 delegate bool Cb(IntPtr h, IntPtr l);
 [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
 [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
 [DllImport("user32.dll")] public static extern bool GetCursorPos(out POINT p);
 [DllImport("user32.dll")] public static extern void mouse_event(uint f, int dx, int dy, uint d, UIntPtr e);
 [DllImport("user32.dll")] public static extern void keybd_event(byte vk, byte scan, uint flags, UIntPtr e);
 [DllImport("user32.dll")] public static extern IntPtr WindowFromPoint(POINT p);
 [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
 [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
 [DllImport("user32.dll")] public static extern bool IsIconic(IntPtr h);
 [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
 [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
 [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
 [DllImport("user32.dll")] public static extern uint GetGuiResources(IntPtr proc, uint flags);
 [DllImport("user32.dll")] static extern bool EnumWindows(Cb c, IntPtr l);
 [DllImport("user32.dll", CharSet=CharSet.Unicode)] static extern int GetClassNameW(IntPtr h, StringBuilder s, int n);
 [DllImport("user32.dll", CharSet=CharSet.Unicode)] static extern int GetWindowTextW(IntPtr h, StringBuilder s, int n);
 [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern IntPtr SendMessageW(IntPtr h, uint m, IntPtr w, string text);
 [DllImport("user32.dll")] public static extern IntPtr SendMessageW(IntPtr h, uint m, IntPtr w, IntPtr l);
 [DllImport("user32.dll", EntryPoint="FindWindowW", CharSet=CharSet.Unicode)] public static extern IntPtr Find(string c, string t);

 public static IntPtr Thumb(string title) { IntPtr found = IntPtr.Zero;
   EnumWindows((h, l) => { var c = new StringBuilder(128); GetClassNameW(h, c, 128); if (c.ToString() != "EveOverlayThumbnail") return true;
     var t = new StringBuilder(256); GetWindowTextW(h, t, 256); if (t.ToString() == title) { found = h; return false; } return true; }, IntPtr.Zero); return found; }
 public static int Count(string cls) { int n = 0; EnumWindows((h, l) => { var c = new StringBuilder(128); GetClassNameW(h, c, 128); if (c.ToString() == cls) n++; return true; }, IntPtr.Zero); return n; }
 public static string ForegroundTitle() { var s = new StringBuilder(256); GetWindowTextW(GetForegroundWindow(), s, 256); return s.ToString(); }
 public static int[] Outer(IntPtr h) { RECT r; GetWindowRect(h, out r); return new int[] { r.L, r.T, r.R - r.L, r.B - r.T }; }
 public static int[] Client(IntPtr h) { RECT r; GetClientRect(h, out r); return new int[] { r.R, r.B }; }
 public static bool Over(IntPtr thumb, int x, int y) { POINT p; p.X = x; p.Y = y; return WindowFromPoint(p) == thumb; }
 public static int HitTest(IntPtr h, int x, int y) { return (int)SendMessageW(h, 0x84, IntPtr.Zero, (IntPtr)((y << 16) | (x & 0xFFFF))); }
 public static void ForceForeground(IntPtr h) { keybd_event(0x12, 0, 0, UIntPtr.Zero); SetForegroundWindow(h); keybd_event(0x12, 0, 2, UIntPtr.Zero); }
 public static void Key(byte vk, bool up) { keybd_event(vk, 0, up ? 2u : 0u, UIntPtr.Zero); }
}
"@
[void][Qa]::SetProcessDPIAware()

$LeftDown = 2; $LeftUp = 4; $RightDown = 8; $RightUp = 16
$AlphaSlot = @{ X = 600; Y = 300 }
$BetaSlot = @{ X = 1100; Y = 300 }
$LayoutJson = "`"FlatLayout`": { `"EVE - Alpha`": `"$($AlphaSlot.X), $($AlphaSlot.Y)`", `"EVE - Beta`": `"$($BetaSlot.X), $($BetaSlot.Y)`" }"
$ParkPoint = @{ X = 10; Y = 700 }

$script:Failed = 0
$script:Passed = 0
$script:App = $null
$script:Fakes = @()

function Wait($Milliseconds) { Start-Sleep -Milliseconds $Milliseconds }
function Check($Condition, $Message)
{
    if ($Condition) { $script:Passed++; "  ok    $Message" } else { $script:Failed++; "  FAIL  $Message" }
}
function Stop-All { Get-Process EveOverlay, ExeFile -ErrorAction SilentlyContinue | Stop-Process -Force; Wait 500 }
function Start-Fake($Title) { $process = Start-Process $FakePath -ArgumentList $Title -PassThru; Wait 400; $process }
function Begin-Scenario($Json, $Titles)
{
    Stop-All
    $Json | Set-Content $ConfigPath -Encoding ascii
    [void][Qa]::SetCursorPos($ParkPoint.X, $ParkPoint.Y)
    $script:Fakes = @(); foreach ($title in $Titles) { $script:Fakes += Start-Fake $title }
    Wait 700
    $script:App = Start-Process $AppPath -PassThru
    Wait 3500
}
function Close-App
{
    $frame = [Qa]::Find("EveOverlayMainFrame", "Eve Overlay")
    [void][Qa]::SendMessageW($frame, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero)
    [void]$script:App.WaitForExit(5000)
}
function Read-Config { Get-Content $ConfigPath -Raw | ConvertFrom-Json }
function Click-Thumbnail($Thumbnail, $OffsetX = 100, $OffsetY = 80)
{
    $rect = [Qa]::Outer($Thumbnail); $x = $rect[0] + $OffsetX; $y = $rect[1] + $OffsetY
    if (-not [Qa]::Over($Thumbnail, $x, $y)) { return $false }
    [void][Qa]::SetCursorPos($x, $y); Wait 300; [void][Qa]::SetCursorPos($x + 2, $y + 2); Wait 400
    [Qa]::mouse_event($LeftDown, 0, 0, 0, [UIntPtr]::Zero); [Qa]::mouse_event($LeftUp, 0, 0, 0, [UIntPtr]::Zero)
    return $true
}
function Park-Cursor { [void][Qa]::SetCursorPos($ParkPoint.X, $ParkPoint.Y) }

$origin = New-Object Qa+POINT; [void][Qa]::GetCursorPos([ref]$origin)
"EveOverlay QA  (app: $AppPath)"

try
{
    if ($Section -eq "all" -or $Section -eq "input")
    {
        "[input] hover zoom, click, ctrl+click, hotkey, drag, client removal"
        Begin-Scenario "{ $LayoutJson, `"ThumbnailsOpacity`": 1.0, `"EnableThumbnailZoom`": true, `"ThumbnailZoomFactor`": 2, `"ThumbnailZoomAnchor`": 0, `"ClientHotkey`": { `"EVE - Alpha`": `"Control, Shift, F9`" } }" @("EVE - Alpha", "EVE - Beta")
        $alpha = [Qa]::Thumb("EVE - Alpha"); $beta = [Qa]::Thumb("EVE - Beta")
        Check (($alpha -ne [IntPtr]::Zero) -and ($beta -ne [IntPtr]::Zero)) "both thumbnails exist"
        $a = [Qa]::Outer($alpha)
        Check (($a[0] -eq $AlphaSlot.X) -and ($a[1] -eq $AlphaSlot.Y) -and ($a[2] -eq 384) -and ($a[3] -eq 216)) "Alpha placed from its saved layout at 384x216 (got $($a -join ','))"

        $x = $a[0] + 100; $y = $a[1] + 80
        if ([Qa]::Over($alpha, $x, $y))
        {
            [void][Qa]::SetCursorPos($x, $y); Wait 300; [void][Qa]::SetCursorPos($x + 3, $y + 3); Wait 600
            $zoomed = [Qa]::Outer($alpha)
            Check (($zoomed[2] -eq 768) -and ($zoomed[3] -eq 432)) "hover zooms to 768x432 (got $($zoomed[2])x$($zoomed[3]))"
            Check (($zoomed[0] -eq $a[0]) -and ($zoomed[1] -eq $a[1])) "NW anchor keeps the origin"
            [Qa]::mouse_event($LeftDown, 0, 0, 0, [UIntPtr]::Zero); [Qa]::mouse_event($LeftUp, 0, 0, 0, [UIntPtr]::Zero); Wait 600
            Check ([Qa]::ForegroundTitle() -eq "EVE - Alpha") "left click brings the Alpha client to the front"
            Park-Cursor; Wait 800
            $restored = [Qa]::Outer($alpha)
            Check (($restored[2] -eq 384) -and ($restored[3] -eq 216)) "leaving restores 384x216"
        }
        else { Check $false "Alpha thumbnail under the test cursor point" }

        $fakeBeta = [Qa]::Find("FakeEve", "EVE - Beta")
        [Qa]::Key(0x11, $false); Wait 100
        $clicked = Click-Thumbnail $beta
        Wait 100; [Qa]::Key(0x11, $true); Wait 700
        Check ($clicked -and [Qa]::IsIconic($fakeBeta)) "ctrl+click minimizes the Beta client"
        Park-Cursor; Wait 500

        [Qa]::ForceForeground([Qa]::Find("FakeEve", "EVE - Beta")); Wait 800
        [Qa]::Key(0x11, $false); [Qa]::Key(0x10, $false); [Qa]::Key(0x78, $false); Wait 100
        [Qa]::Key(0x78, $true); [Qa]::Key(0x10, $true); [Qa]::Key(0x11, $true); Wait 800
        Check ([Qa]::ForegroundTitle() -eq "EVE - Alpha") "hotkey Ctrl+Shift+F9 activates Alpha"

        $b = [Qa]::Outer($beta); $bx = $b[0] + 100; $by = $b[1] + 80
        if ([Qa]::Over($beta, $bx, $by))
        {
            [void][Qa]::SetCursorPos($bx, $by); Wait 300; [void][Qa]::SetCursorPos($bx + 1, $by + 1); Wait 400
            [Qa]::mouse_event($RightDown, 0, 0, 0, [UIntPtr]::Zero); Wait 100
            for ($step = 1; $step -le 10; $step++) { [void][Qa]::SetCursorPos($bx + 1 + $step * 10, $by + 1 + $step * 5); Wait 40 }
            [Qa]::mouse_event($RightUp, 0, 0, 0, [UIntPtr]::Zero); Wait 300
            $moved = [Qa]::Outer($beta)
            Check ((($moved[0] - $b[0]) -eq 100) -and (($moved[1] - $b[1]) -eq 50)) "right-drag moves the thumbnail by (100,50)"
            Park-Cursor; Wait 3500
            Check ((Read-Config).FlatLayout.'EVE - Beta' -eq "$($moved[0]), $($moved[1])") "dropped location is saved to the config"
        }
        else { Check $false "Beta thumbnail under the test cursor point" }

        $script:Fakes[1].Kill(); Wait 2500
        Check ([Qa]::Thumb("EVE - Beta") -eq [IntPtr]::Zero) "thumbnail disappears when its client closes"
        Park-Cursor
    }

    if ($Section -eq "all" -or $Section -eq "snap")
    {
        "[snap] release near a neighbour's corner"
        Begin-Scenario "{ `"FlatLayout`": { `"EVE - Alpha`": `"$($AlphaSlot.X), $($AlphaSlot.Y)`", `"EVE - Beta`": `"$($AlphaSlot.X + 500), $($AlphaSlot.Y)`" }, `"ThumbnailsOpacity`": 1.0 }" @("EVE - Alpha", "EVE - Beta")
        $beta = [Qa]::Thumb("EVE - Beta"); $b = [Qa]::Outer($beta); $bx = $b[0] + 100; $by = $b[1] + 80
        if ([Qa]::Over($beta, $bx, $by))
        {
            [void][Qa]::SetCursorPos($bx, $by); Wait 300; [void][Qa]::SetCursorPos($bx + 1, $by + 1); Wait 400
            [Qa]::mouse_event($RightDown, 0, 0, 0, [UIntPtr]::Zero); Wait 100
            for ($step = 1; $step -le 10; $step++) { [void][Qa]::SetCursorPos($bx + 1 - $step * 10, $by + 1 + $step); Wait 40 }
            [Qa]::mouse_event($RightUp, 0, 0, 0, [UIntPtr]::Zero); Wait 300
            Park-Cursor; Wait 3500
            $snapped = [Qa]::Outer($beta)
            Check (($snapped[0] -eq ($AlphaSlot.X + 384)) -and ($snapped[1] -eq $AlphaSlot.Y)) "thumbnail snapped to Alpha's right edge (got $($snapped[0]),$($snapped[1]))"
            Check ((Read-Config).FlatLayout.'EVE - Beta' -eq "$($AlphaSlot.X + 384), $($AlphaSlot.Y)") "snapped location is saved"
        }
        else { Check $false "Beta thumbnail under the test cursor point" }
    }

    if ($Section -eq "all" -or $Section -eq "hide")
    {
        "[hide] hide previews when no client is active"
        Begin-Scenario "{ `"HideThumbnailsOnLostFocus`": true, $LayoutJson }" @("EVE - Alpha")
        $fakeAlpha = [Qa]::Find("FakeEve", "EVE - Alpha"); $alpha = [Qa]::Thumb("EVE - Alpha")
        [Qa]::ForceForeground($fakeAlpha); Wait 1500
        Check ([Qa]::IsWindowVisible($alpha)) "visible while the client is in front"
        (New-Object -ComObject WScript.Shell).AppActivate("Program Manager") | Out-Null; Wait 3000
        Check (-not [Qa]::IsWindowVisible($alpha)) "hidden once focus is elsewhere"
        [Qa]::ForceForeground($fakeAlpha); Wait 2000
        Check ([Qa]::IsWindowVisible($alpha)) "shown again when the client regains focus"
    }

    if ($Section -eq "all" -or $Section -eq "resize")
    {
        "[resize] drag-resizing a framed thumbnail resizes all of them"
        Begin-Scenario "{ `"ShowThumbnailFrames`": true, $LayoutJson, `"ThumbnailsOpacity`": 1.0 }" @("EVE - Alpha", "EVE - Beta")
        $alpha = [Qa]::Thumb("EVE - Alpha"); $beta = [Qa]::Thumb("EVE - Beta"); $a = [Qa]::Outer($alpha)
        Check (($a[2] -gt 384) -and ([Qa]::Client($alpha)[0] -eq 384)) "frames add a border but keep the 384 pixel client width"
        $cx = $a[0] + $a[2] - 3; $cy = $a[1] + $a[3] - 3
        if (([Qa]::HitTest($alpha, $cx, $cy) -eq 17) -and [Qa]::Over($alpha, $cx, $cy))
        {
            [void][Qa]::SetCursorPos($cx, $cy); Wait 300
            [Qa]::mouse_event($LeftDown, 0, 0, 0, [UIntPtr]::Zero); Wait 200
            for ($step = 1; $step -le 10; $step++) { [void][Qa]::SetCursorPos($cx + $step * 6, $cy + $step * 3); Wait 50 }
            [Qa]::mouse_event($LeftUp, 0, 0, 0, [UIntPtr]::Zero); Wait 1500
            $clientAlpha = [Qa]::Client($alpha); $clientBeta = [Qa]::Client($beta)
            Check (($clientAlpha[0] -gt 384) -and ($clientAlpha[1] -gt 216)) "Alpha grew to $($clientAlpha -join 'x')"
            Check (($clientAlpha[0] -eq $clientBeta[0]) -and ($clientAlpha[1] -eq $clientBeta[1])) "Beta follows to the same size"
            Park-Cursor; Wait 500
            Close-App
            Check ((Read-Config).ThumbnailSize -eq "$($clientAlpha[0]), $($clientAlpha[1])") "the new size is persisted on exit"
        }
        else { Check $false "bottom-right corner grabbed" }
    }

    if ($Section -eq "all" -or $Section -eq "minimize")
    {
        "[minimize] minimize inactive clients"
        Begin-Scenario "{ `"MinimizeInactiveClients`": true, $LayoutJson, `"ThumbnailsOpacity`": 1.0 }" @("EVE - Alpha", "EVE - Beta")
        $fakeAlpha = [Qa]::Find("FakeEve", "EVE - Alpha"); $fakeBeta = [Qa]::Find("FakeEve", "EVE - Beta"); $beta = [Qa]::Thumb("EVE - Beta")
        [Qa]::ForceForeground($fakeAlpha); Wait 1500
        Check (-not [Qa]::IsIconic($fakeAlpha)) "Alpha is active and not minimized"
        Check (Click-Thumbnail $beta) "Beta thumbnail clicked"
        Wait 1500
        Check ([Qa]::ForegroundTitle() -eq "EVE - Beta") "the clicked client gets the foreground (not the settings window)"
        Check ([Qa]::IsIconic($fakeAlpha)) "the previously active client is minimized"
        Check (-not [Qa]::IsIconic($fakeBeta)) "the new client stays up"
        Park-Cursor
    }

    if ($Section -eq "all" -or $Section -eq "layout")
    {
        "[layout] client layout tracking"
        Begin-Scenario "{ `"EnableClientLayoutTracking`": true, `"ClientLayout`": { `"EVE - Alpha`": { `"X`": 300, `"Y`": 350, `"Width`": 520, `"Height`": 410, `"IsMaximized`": false } }, $LayoutJson }" @("EVE - Alpha", "EVE - Beta")
        $r = [Qa]::Outer([Qa]::Find("FakeEve", "EVE - Alpha"))
        Check (($r[0] -eq 300) -and ($r[1] -eq 350) -and ($r[2] -eq 520) -and ($r[3] -eq 410)) "Alpha window moved to its saved layout ($($r -join ','))"
        Check (Click-Thumbnail ([Qa]::Thumb("EVE - Beta"))) "Beta thumbnail clicked"
        Wait 1200; Park-Cursor
        Close-App
        Check ($null -ne (Read-Config).ClientLayout.'EVE - Beta') "activating a client records its window layout"
    }

    if ($Section -eq "all" -or $Section -eq "rename")
    {
        "[rename] a client logging in (title change)"
        Begin-Scenario "{ `"FlatLayout`": { `"EVE - Pilot`": `"800, 500`" }, `"ClientHotkey`": { `"EVE - Pilot`": `"Control, Shift, F8`" } }" @("EVE")
        $login = [Qa]::Thumb("EVE"); $r = [Qa]::Outer($login)
        Check (($r[0] -eq 5) -and ($r[1] -eq 5)) "logged-out client sits at the default (5,5)"
        [void][Qa]::SendMessageW([Qa]::Find("FakeEve", "EVE"), 0x000C, [IntPtr]::Zero, "EVE - Pilot"); Wait 2000
        $pilot = [Qa]::Thumb("EVE - Pilot")
        Check ($pilot -ne [IntPtr]::Zero) "thumbnail takes the new title"
        if ($pilot -ne [IntPtr]::Zero) { $r = [Qa]::Outer($pilot); Check (($r[0] -eq 800) -and ($r[1] -eq 500)) "and moves to that character's saved location" }
        [Qa]::ForceForeground([Qa]::Find("Shell_TrayWnd", $null)); Wait 500
        [Qa]::Key(0x11, $false); [Qa]::Key(0x10, $false); [Qa]::Key(0x77, $false); Wait 100
        [Qa]::Key(0x77, $true); [Qa]::Key(0x10, $true); [Qa]::Key(0x11, $true); Wait 800
        Check ([Qa]::ForegroundTitle() -eq "EVE - Pilot") "the hotkey of the new title is registered"
    }

    if ($Section -eq "all" -or $Section -eq "leak")
    {
        "[leak] 20 clients appear and disappear"
        Begin-Scenario "{ `"FlatLayout`": { `"EVE - Keeper`": `"600, 300`" } }" @("EVE - Keeper")
        $handle = $script:App.Handle; Wait 1000
        $gdi0 = [Qa]::GetGuiResources($handle, 0); $user0 = [Qa]::GetGuiResources($handle, 1); $handles0 = (Get-Process -Id $script:App.Id).HandleCount
        for ($index = 1; $index -le 20; $index++) { $fake = Start-Fake "EVE - Churn $index"; Wait 1300; $fake.Kill(); Wait 900 }
        Wait 2000
        $gdi1 = [Qa]::GetGuiResources($handle, 0); $user1 = [Qa]::GetGuiResources($handle, 1); $handles1 = (Get-Process -Id $script:App.Id).HandleCount
        Check ([Qa]::Count("EveOverlayThumbnail") -eq 1) "only the surviving client's thumbnail remains"
        Check ($gdi1 -le $gdi0 + 3) "GDI objects do not grow ($gdi0 -> $gdi1)"
        Check ($user1 -le $user0 + 3) "USER objects do not grow ($user0 -> $user1)"
        Check ($handles1 -le $handles0 + 10) "kernel handles do not grow ($handles0 -> $handles1)"
    }
}
finally
{
    [void][Qa]::SetCursorPos($origin.X, $origin.Y)
    Stop-All
    [System.IO.File]::Delete($ConfigPath)
}

""
"passed: $script:Passed   failed: $script:Failed"
exit $script:Failed

#include "UI/DScanOverlay.h"

#include <algorithm>
#include <utility>

#include "Config/TextUtil.h"

DScanOverlay::DScanOverlay(std::filesystem::path DataPath, std::filesystem::path SettingsFilePath)
    : Panel(std::move(SettingsFilePath), L"DScanPanel", PANEL_WIDTH)
    , CatalogPath(std::move(DataPath))
{
}

void DScanOverlay::Configure(const DScanOverlayOptions& Options)
{
    if (Options.AutoRead == true && Settings.AutoRead == false)
    {
        // A scan that was already on the clipboard when the feature was switched on is not read
        LastClipboardSequence = ::GetClipboardSequenceNumber();
    }

    Settings = Options;
    if (TickerStarted == false)
    {
        TickerStarted = true;
        Ticker.Start(TICK_INTERVAL_MS, [this]()
        {
            Tick();
        });
    }

    Refresh();
}

void DScanOverlay::SetClientsOpen(const bool IsOpen)
{
    ClientsOpen = IsOpen;
    Refresh();
}

const DScanResult& DScanOverlay::GetLastResult() const
{
    return LastResult;
}

bool DScanOverlay::HasResult() const
{
    return ResultReady;
}

bool DScanOverlay::IsCatalogLoaded() const
{
    return ShipCatalog::GetShared(CatalogPath).Count() > 0;
}

bool DScanOverlay::TryGetClipboardText(std::string* const Text, bool* const Opened)
{
    if (::OpenClipboard(nullptr) == FALSE)
    {
        return false;
    }

    if (Opened != nullptr)
    {
        *Opened = true;
    }

    bool Read = false;
    const HANDLE Data = ::GetClipboardData(CF_UNICODETEXT);
    if (Data != nullptr)
    {
        const wchar_t* const Wide = static_cast<const wchar_t*>(::GlobalLock(Data));
        if (Wide != nullptr)
        {
            *Text = TextUtil::ToUtf8(Wide);
            ::GlobalUnlock(Data);
            Read = true;
        }
    }

    ::CloseClipboard();
    return Read;
}

bool DScanOverlay::ReadClipboardNow()
{
    std::string Text;
    return TryGetClipboardText(&Text) == true && Accept(Text) == true;
}

bool DScanOverlay::Accept(const std::string& Text)
{
    DScanResult Result = DScanAnalyzer::Analyze(ShipCatalog::GetShared(CatalogPath), Text);
    if (Result.LooksLikeScan == false)
    {
        return false;
    }

    LastResult = std::move(Result);
    ResultReady = true;
    HideAtTick = ::GetTickCount64() + static_cast<ULONGLONG>(std::max(5, Settings.ShowSeconds)) * 1000ULL;
    Refresh();
    ScanRead.Emit();
    return true;
}

void DScanOverlay::Tick()
{
    if (Settings.AutoRead == true)
    {
        const DWORD Sequence = ::GetClipboardSequenceNumber();
        if (Sequence != LastClipboardSequence)
        {
            std::string Text;
            // The clipboard can be busy for a moment; the same change is tried again on the next tick
            bool Opened = false;
            const bool HasText = TryGetClipboardText(&Text, &Opened);
            if (Opened == true)
            {
                LastClipboardSequence = Sequence;
            }

            if (HasText == true)
            {
                Accept(Text);
            }
        }
    }

    Refresh();
}

void DScanOverlay::Refresh()
{
    const bool Showing = ResultReady == true && ::GetTickCount64() < HideAtTick && ClientsOpen == true;
    Panel.Update(Showing == true ? BuildRows() : std::vector<PanelRow>(), Showing);
}

std::vector<PanelRow> DScanOverlay::BuildRows() const
{
    std::vector<PanelRow> Rows;

    PanelRow Heading;
    Heading.Heading = true;
    Heading.Label = "D-Scan: " + std::to_string(LastResult.Lines) + " objects";
    Heading.Red = 130;
    Heading.Green = 200;
    Heading.Blue = 255;
    Rows.push_back(std::move(Heading));

    for (const DScanClassCount& Entry : LastResult.Classes)
    {
        PanelRow Row;
        Row.Label = DScanAnalyzer::GetClassLabel(Entry.Class);
        Row.Value = std::to_string(Entry.Count);
        Rows.push_back(std::move(Row));
    }

    if (LastResult.Flags.empty() == true)
    {
        return Rows;
    }

    PanelRow Watch;
    Watch.Heading = true;
    Watch.Label = "Watch for";
    Watch.Red = 255;
    Watch.Green = 170;
    Watch.Blue = 90;
    Rows.push_back(std::move(Watch));

    for (size_t Index = 0; Index < LastResult.Flags.size() && Index < MAXIMUM_FLAG_ROWS; Index++)
    {
        const DScanFlag& Flag = LastResult.Flags[Index];
        PanelRow Row;
        Row.Label = (Flag.Count > 1 ? std::to_string(Flag.Count) + "x " : std::string()) + Flag.TypeName + " (" + Flag.Group + ")";
        Row.Value = DScanAnalyzer::FormatDistance(Flag.NearestMeters);
        Row.Red = 255;
        Row.Green = Flag.Severity >= 80 ? 110 : 190;
        Row.Blue = Flag.Severity >= 80 ? 90 : 90;
        Rows.push_back(std::move(Row));
    }

    return Rows;
}

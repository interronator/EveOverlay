#include "UI/DScanOverlay.h"

#include <algorithm>
#include <cstring>
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

bool DScanOverlay::TrySetClipboardText(const std::string& Text)
{
    const std::wstring Wide = TextUtil::FromUtf8(Text);
    const SIZE_T Bytes = (Wide.size() + 1) * sizeof(wchar_t);
    const HGLOBAL Memory = ::GlobalAlloc(GMEM_MOVEABLE, Bytes);
    if (Memory == nullptr)
    {
        return false;
    }

    void* const Destination = ::GlobalLock(Memory);
    if (Destination == nullptr)
    {
        ::GlobalFree(Memory);
        return false;
    }

    std::memcpy(Destination, Wide.c_str(), Bytes);
    ::GlobalUnlock(Memory);

    if (::OpenClipboard(nullptr) == FALSE)
    {
        ::GlobalFree(Memory);
        return false;
    }

    ::EmptyClipboard();
    const bool Stored = ::SetClipboardData(CF_UNICODETEXT, Memory) != nullptr;
    ::CloseClipboard();
    if (Stored == false)
    {
        ::GlobalFree(Memory);
    }

    return Stored;
}

bool DScanOverlay::CopyShipListToClipboard()
{
    const std::string List = DScanAnalyzer::FormatShipList(LastResult);
    if (ResultReady == false || List.empty() == true || TrySetClipboardText(List) == false)
    {
        return false;
    }

    // Our own copy must not be mistaken for a new scan to read
    LastClipboardSequence = ::GetClipboardSequenceNumber();
    return true;
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
    if (Settings.CopyShipList == true)
    {
        CopyShipListToClipboard();
    }

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

    if (LastResult.Ships.empty() == false)
    {
        PanelRow Ships;
        Ships.Heading = true;
        Ships.Label = "Ships on scan";
        Ships.Red = 130;
        Ships.Green = 200;
        Ships.Blue = 255;
        Rows.push_back(std::move(Ships));

        for (size_t Index = 0; Index < LastResult.Ships.size() && Index < MAXIMUM_SHIP_ROWS; Index++)
        {
            PanelRow Row;
            Row.Label = LastResult.Ships[Index].TypeName;
            Row.Value = std::to_string(LastResult.Ships[Index].Count);
            Rows.push_back(std::move(Row));
        }

        if (LastResult.Ships.size() > MAXIMUM_SHIP_ROWS)
        {
            int Hidden = 0;
            for (size_t Index = MAXIMUM_SHIP_ROWS; Index < LastResult.Ships.size(); Index++)
            {
                Hidden += LastResult.Ships[Index].Count;
            }

            PanelRow More;
            More.Label = "Other ships";
            More.Value = std::to_string(Hidden);
            Rows.push_back(std::move(More));
        }
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

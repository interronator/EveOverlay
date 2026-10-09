#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <unordered_set>

#include "Services/AlertSound.h"
#include "Services/GameLogWatcher.h"
#include "Services/UpdateChecker.h"
#include "Universe/DScanAnalyzer.h"
#include "Universe/ChatLogWatcher.h"
#include "Universe/IntelParser.h"
#include "Universe/UniverseLayout.h"

class Checker
{
public:
    void Expect(const bool Condition, const char* const Description)
    {
        if (Condition == true)
        {
            return;
        }

        FailureCount++;
        std::printf("FAIL: %s\n", Description);
    }

    int FailureCount = 0;
};

void WriteUtf16(const std::filesystem::path& Path, const std::wstring& Text, const bool Append)
{
    std::ofstream Stream(Path, std::ios::binary | (Append == true ? std::ios::app : std::ios::trunc));
    Stream.write(reinterpret_cast<const char*>(Text.data()), static_cast<std::streamsize>(Text.size() * sizeof(wchar_t)));
}

int main(const int ArgumentCount, const char* const Arguments[])
{
    Checker Check;
    const char* const CsvPath = ArgumentCount > 1 ? Arguments[1] : "universeData/systems.csv";

    UniverseData Data;
    Check.Expect(Data.Load(CsvPath) == true, "csv loads");
    Check.Expect(Data.Count() == 5528, "5528 systems");

    const int Jita = Data.Find("jita");
    Check.Expect(Jita != UniverseData::NOT_FOUND, "Jita found case-insensitively");
    Check.Expect(Data.Find("Nowhere-Land") == UniverseData::NOT_FOUND, "unknown system not found");

    for (size_t Index = 0; Index < Data.Count(); Index++)
    {
        for (const int Neighbour : Data.Get(static_cast<int>(Index)).Neighbours)
        {
            const std::vector<int>& Back = Data.Get(Neighbour).Neighbours;
            if (std::find(Back.begin(), Back.end(), static_cast<int>(Index)) == Back.end())
            {
                Check.Expect(false, "links are symmetric");
                break;
            }
        }
    }

    const UniverseNeighborhood Zero = UniverseNeighborhood::Build(Data, Jita, 0);
    Check.Expect(Zero.Nodes.size() == 1 && Zero.Edges.empty() == true, "0 jumps is only the centre");

    const UniverseNeighborhood One = UniverseNeighborhood::Build(Data, Jita, 1);
    Check.Expect(One.Nodes.size() == 8, "Jita has 7 direct neighbours");
    Check.Expect(UniverseNeighborhood::Build(Data, UniverseData::NOT_FOUND, 3).Nodes.empty() == true, "invalid centre yields empty map");

    const UniverseNeighborhood Three = UniverseNeighborhood::Build(Data, Jita, 3);
    int MaxJumps = 0;
    for (const NeighborhoodNode& Node : Three.Nodes)
    {
        MaxJumps = std::max(MaxJumps, Node.Jumps);
    }
    Check.Expect(MaxJumps == 3, "3 jump map reaches depth 3");
    Check.Expect(Three.Nodes.size() > One.Nodes.size(), "3 jump map is larger than 1 jump map");

    const std::vector<LayoutPoint> Points = UniverseLayout::Compute(Three);
    Check.Expect(Points.size() == Three.Nodes.size(), "one point per node");
    double MinGap = 1e9;
    bool AllFinite = true;
    for (size_t First = 0; First < Points.size(); First++)
    {
        AllFinite = AllFinite && std::isfinite(Points[First].X) && std::isfinite(Points[First].Y);
        for (size_t Second = First + 1; Second < Points.size(); Second++)
        {
            MinGap = std::min(MinGap, std::hypot(Points[First].X - Points[Second].X, Points[First].Y - Points[Second].Y));
        }
    }
    Check.Expect(AllFinite == true, "layout is finite");
    Check.Expect(Points[0].X == 0.0 && Points[0].Y == 0.0, "centre pinned at origin");
    std::printf("Jita 3 jumps: %zu nodes, %zu edges, min gap %.4f\n", Three.Nodes.size(), Three.Edges.size(), MinGap);
    Check.Expect(MinGap > 0.005, "no overlapping nodes");

    std::unordered_set<int> Candidates;
    for (const NeighborhoodNode& Node : Three.Nodes)
    {
        Candidates.insert(Node.SystemIndex);
    }

    const std::vector<int> Spotted = IntelParser::FindSystems(Data, "Jita clr, hostile in new caldari + Perimeter. Amarr?", Candidates);
    Check.Expect(Spotted.size() == 3, "finds Jita, New Caldari (multi word) and Perimeter, ignores Amarr outside range");
    Check.Expect(IntelParser::FindSystems(Data, "jita jita JITA", Candidates).size() == 1, "duplicates collapse");
    Check.Expect(IntelParser::FindSystems(Data, "jItA NEW cAlDaRi PERIMETER", Candidates).size() == 3, "mixed case names all match");
    Check.Expect(Data.Find("JITA") == Jita && Data.Find("  jiTa ") == Jita, "lookup ignores case and padding");
    Check.Expect(IntelParser::FindSystems(Data, "nothing to see", Candidates).empty() == true, "no systems in plain chat");

    Check.Expect(IntelParser::FindSystems(Data, "Jita clr, Perimeter red", Candidates, true).size() == 1, "a system reported clear is skipped but the rest of the line still counts");
    Check.Expect(IntelParser::FindSystems(Data, "Jita CLEAR", Candidates, true).empty() == true, "clear in capitals is skipped");
    Check.Expect(IntelParser::FindSystems(Data, "Jita nv", Candidates, true).empty() == true, "nv means no visual");
    Check.Expect(IntelParser::FindSystems(Data, "Jita no visual", Candidates, true).empty() == true, "no visual is skipped");
    Check.Expect(IntelParser::FindSystems(Data, "anyone in Jita?", Candidates, true).empty() == true, "a question is not a report");
    Check.Expect(IntelParser::FindSystems(Data, "Jita status", Candidates, true).empty() == true, "a status request is not a report");
    Check.Expect(IntelParser::FindSystems(Data, "Jita red clear of Perimeter", Candidates, true).size() == 2, "a clear word that does not follow a system leaves it alone");
    Check.Expect(IntelParser::FindSystems(Data, "Jita clr", Candidates, false).size() == 1, "clear reports still count when the filter is off");

    const std::vector<std::string> Keywords = IntelParser::ParseKeywordList(" Bubble, gate camp;CYNO,, bubble ");
    Check.Expect(Keywords.size() == 3 && Keywords[0] == "bubble" && Keywords[1] == "gate camp" && Keywords[2] == "cyno", "keyword list is split, lowered and de-duplicated");
    Check.Expect(IntelParser::ContainsKeyword("Jita has a BUBBLE up", Keywords) == true, "keyword matches ignoring case");
    Check.Expect(IntelParser::ContainsKeyword("big gate camp at Jita", Keywords) == true, "multi word keyword matches");
    Check.Expect(IntelParser::ContainsKeyword("bubbles everywhere", Keywords) == false, "keyword must be a whole word");
    Check.Expect(IntelParser::ContainsKeyword("Jita red", Keywords) == false, "no keyword, no match");
    Check.Expect(IntelParser::ContainsKeyword("anything", std::vector<std::string>()) == false, "empty keyword list never matches");

    Check.Expect(ChatLogWatcher::ParseChannelChange("Channel changed to Local : C-N4OD") == "C-N4OD", "system read from a Local channel change");
    Check.Expect(ChatLogWatcher::ParseChannelChange("Channel MOTD: hello") .empty() == true, "other system messages are not a channel change");
    Check.Expect(ChatLogWatcher::ExtractChannelName("Local_20261008_173041_2117795859.txt") == "Local", "channel name from file name");
    Check.Expect(ChatLogWatcher::ExtractChannelName("I. Ftn_Intel_20261007_185314_123.txt") == "I. Ftn_Intel", "channel name keeps its own underscores");
    Check.Expect(ChatLogWatcher::ExtractChannelName("nonsense.txt").empty() == true, "unexpected file names give no channel");

    const std::filesystem::path TempDirectory = std::filesystem::temp_directory_path() / "UniverseProbeChat";
    std::filesystem::remove_all(TempDirectory);
    std::filesystem::create_directories(TempDirectory);
    const std::filesystem::path LogPath = TempDirectory / L"I. Ftn Intel_20261007_185314_123.txt";
    WriteUtf16(LogPath, L"\xFEFF        ---\r\n\r\n  Channel Name:    I. Ftn Intel\r\n[ 2026.10.07 18:53:18 ] Old Pilot > Jita old news\r\n", false);

    ChatLogWatcher Watcher;
    Watcher.SetDirectory(TempDirectory);
    Watcher.SetChannel("intel");
    Check.Expect(Watcher.Poll().empty() == true, "existing history is not replayed");

    WriteUtf16(LogPath, L"[ 2026.10.07 18:54:00 ] Some Pilot > Perimeter red\r\n[ 2026.10.07 18:54:01 ] EVE System > Channel changed\r\n[ 2026.10.07 18:54:02 ] Half Pilot > Half writ", true);
    const std::vector<ChatMessage> Fresh = Watcher.Poll();
    Check.Expect(Fresh.size() == 1 && Fresh[0].Sender == "Some Pilot" && Fresh[0].Text == "Perimeter red", "new complete line read, system and partial lines skipped");

    WriteUtf16(LogPath, L"ten\r\n", true);
    const std::vector<ChatMessage> Completed = Watcher.Poll();
    Check.Expect(Completed.size() == 1 && Completed[0].Text == "Half written", "partial line delivered once completed");

    WriteUtf16(TempDirectory / L"Local_20261007_185314_123.txt", L"[ 2026.10.07 18:54:09 ] X > Jita\r\n", false);
    Check.Expect(Watcher.Poll().empty() == true, "other channels ignored");

    WriteUtf16(TempDirectory / L"Fresh Intel_20261007_190000_456.txt", L"\xFEFF[ 2026.10.07 19:00:01 ] Y > Jita\r\n", false);
    Check.Expect(Watcher.Poll().size() == 1, "new session file is read from its start");

    // Real EVE logs put a byte order mark before every line
    const std::filesystem::path RealLogPath = TempDirectory / L"I. Ftn Intel_20261007_230017_2112372278.txt";
    WriteUtf16(RealLogPath, L"\xFEFF" L"[ 2026.10.07 23:00:21 ] EVE System > Channel MOTD: Fountain Intel Channel\r\n", false);
    ChatLogWatcher RealWatcher;
    RealWatcher.SetDirectory(TempDirectory);
    RealWatcher.SetChannel("I. Ftn Intel");
    RealWatcher.Poll();
    WriteUtf16(RealLogPath, L"\xFEFF" L"[ 2026.10.07 23:06:44 ] Meher Andaz > B-DBYQ and DY- are camped  Captain Quasar Fizzpop eyes in B-D with combat probes towards J5A gate.\r\n"
        L"\xFEFF" L"[ 2026.10.07 23:07:35 ] Kaletha > DY-F70  extremedeath  gallen2  imbad23  Exequror Navy Issue  Warp Disrupt Probe\r\n", true);
    const std::vector<ChatMessage> RealMessages = RealWatcher.Poll();
    Check.Expect(RealMessages.size() == 2 && RealMessages[0].Sender == "Meher Andaz" && RealMessages[1].Sender == "Kaletha", "lines with a leading byte order mark are parsed");

    // The game keeps the log open while writing, so the directory entry's size lags behind the real one
    ChatLogWatcher LiveWatcher;
    LiveWatcher.SetDirectory(TempDirectory);
    LiveWatcher.SetChannel("I. Ftn Intel");
    LiveWatcher.Poll();
    std::ofstream LiveFile(RealLogPath, std::ios::binary | std::ios::app);
    const std::wstring LiveLine = L"\xFEFF" L"[ 2026.10.07 23:09:00 ] Pilot > Jita\r\n";
    LiveFile.write(reinterpret_cast<const char*>(LiveLine.data()), static_cast<std::streamsize>(LiveLine.size() * sizeof(wchar_t)));
    LiveFile.flush();
    const std::vector<ChatMessage> LiveMessages = LiveWatcher.Poll();
    Check.Expect(LiveMessages.size() == 1 && LiveMessages[0].Text == "Jita", "line written by a process that still has the file open is seen");
    LiveFile.write(reinterpret_cast<const char*>(LiveLine.data()), static_cast<std::streamsize>(LiveLine.size() * sizeof(wchar_t)));
    LiveFile.flush();
    Check.Expect(LiveWatcher.Poll().size() == 1, "second line from the open file is seen once");
    LiveFile.close();

    {
        const std::vector<std::string> Found = ChatLogWatcher::FindChannels(TempDirectory, std::chrono::hours(24 * 30));
        const bool HasFtn = std::find(Found.begin(), Found.end(), "I. Ftn Intel") != Found.end();
        const bool HasFresh = std::find(Found.begin(), Found.end(), "Fresh Intel") != Found.end();
        const bool HasLocal = std::find(Found.begin(), Found.end(), "Local") != Found.end();
        Check.Expect(HasFtn == true && HasFresh == true && HasLocal == true,"channel names are found, without the date, time and listener id");
        Check.Expect(std::count(Found.begin(), Found.end(), std::string("I. Ftn Intel")) == 1, "a channel with several logs is listed once");
        Check.Expect(ChatLogWatcher::FindChannels(TempDirectory, std::chrono::hours(0)).empty() == true && ChatLogWatcher::FindChannels(TempDirectory / "missing", std::chrono::hours(24)).empty() == true, "nothing found when too old or missing");
    }

    // Several channels at once, each message tagged with its channel and the character that listens
    WriteUtf16(TempDirectory / L"Alpha Intel_20261008_100000_111.txt", L"\xFEFF\r\n  Channel Name:    Alpha Intel\r\n  Listener:        Pilot One\r\n  Session started: 2026.10.08 10:00:00\r\n", false);
    WriteUtf16(TempDirectory / L"Beta Intel_20261008_100000_222.txt", L"\xFEFF\r\n  Channel Name:    Beta Intel\r\n  Listener:        Pilot Two\r\n  Session started: 2026.10.08 10:00:00\r\n", false);
    ChatLogWatcher MultiWatcher;
    MultiWatcher.SetDirectory(TempDirectory);
    MultiWatcher.SetChannel("Alpha Intel, beta intel");
    MultiWatcher.Poll();
    WriteUtf16(TempDirectory / L"Alpha Intel_20261008_100000_111.txt", L"\xFEFF" L"[ 2026.10.08 10:01:00 ] A > Jita\r\n", true);
    WriteUtf16(TempDirectory / L"Beta Intel_20261008_100000_222.txt", L"\xFEFF" L"[ 2026.10.08 10:01:01 ] B > Perimeter\r\n", true);
    const std::vector<ChatMessage> MultiMessages = MultiWatcher.Poll();
    Check.Expect(MultiMessages.size() == 2, "both channels in a list are followed");
    if (MultiMessages.size() == 2)
    {
        const ChatMessage& AlphaMessage = MultiMessages[0].Sender == "A" ? MultiMessages[0] : MultiMessages[1];
        Check.Expect(AlphaMessage.Channel == "Alpha Intel" && AlphaMessage.Listener == "Pilot One" && AlphaMessage.Time == "2026.10.08 10:01:00", "message carries channel, listener and time");
    }

    // Local: the current system comes from the newest channel change, and only an exact channel name matches
    WriteUtf16(TempDirectory / L"Local_20261008_100000_111.txt", L"\xFEFF\r\n  Listener:        Pilot One\r\n\xFEFF" L"[ 2026.10.08 10:00:05 ] EVE System > Channel changed to Local : Jita\r\n\xFEFF" L"[ 2026.10.08 10:30:00 ] EVE System > Channel changed to Local : Perimeter\r\n", false);
    WriteUtf16(TempDirectory / L"Local_20261008_100000_222.txt", L"\xFEFF\r\n  Listener:        Pilot Two\r\n\xFEFF" L"[ 2026.10.08 10:10:00 ] EVE System > Channel changed to Local : New Caldari\r\n", false);
    WriteUtf16(TempDirectory / L"Local Notes_20261008_100000_333.txt", L"\xFEFF" L"[ 2026.10.08 11:00:00 ] EVE System > Channel changed to Local : Amarr\r\n", false);
    ChatLogWatcher LocalWatcher;
    LocalWatcher.SetDirectory(TempDirectory);
    LocalWatcher.SetExactChannelMatch(true);
    LocalWatcher.SetIncludeSystemMessages(true);
    LocalWatcher.SetChannel("Local");
    Check.Expect(LocalWatcher.FindCurrentSystem() == "Perimeter", "newest channel change across characters wins, other channels ignored");

    ChatLogWatcher PilotWatcher;
    PilotWatcher.SetDirectory(TempDirectory);
    PilotWatcher.SetExactChannelMatch(true);
    PilotWatcher.SetIncludeSystemMessages(true);
    PilotWatcher.SetChannel("Local");
    PilotWatcher.SetListener("pilot two");
    Check.Expect(PilotWatcher.FindCurrentSystem() == "New Caldari", "a chosen character reads only its own Local log, ignoring case");
    PilotWatcher.SetListener("Pilot One");
    Check.Expect(PilotWatcher.FindCurrentSystem() == "Perimeter", "switching the chosen character switches the system");
    PilotWatcher.SetListener("Nobody");
    Check.Expect(PilotWatcher.FindCurrentSystem().empty() == true, "a character with no Local log gives no system");
    PilotWatcher.SetListener("");
    Check.Expect(PilotWatcher.FindCurrentSystem() == "Perimeter", "no chosen character goes back to the newest across characters");

    WriteUtf16(TempDirectory / L"Local_20261008_100000_222.txt", L"\xFEFF" L"[ 2026.10.08 10:40:00 ] EVE System > Channel changed to Local : Jita\r\n", true);
    LocalWatcher.Poll();
    WriteUtf16(TempDirectory / L"Local_20261008_100000_222.txt", L"\xFEFF" L"[ 2026.10.08 10:50:00 ] EVE System > Channel changed to Local : New Caldari\r\n", true);
    const std::vector<ChatMessage> LocalMessages = LocalWatcher.Poll();
    Check.Expect(LocalMessages.size() == 1 && LocalMessages[0].Sender == "EVE System" && ChatLogWatcher::ParseChannelChange(LocalMessages[0].Text) == "New Caldari", "system messages are delivered for Local when asked for");

    // Game logs, using lines copied from real logs
    const std::string ScrambleLine = "[ 2024.06.13 04:58:43 ] (combat) <color=0xffffffff><b>Warp scramble attempt</b> <color=0x77ffffff><font size=10>from</font> <color=0xffffffff><b><font size=12><color=0xFFFFB900> <u><b>Pontifex</b></u></color></font><font size=12><color=0xFFFEFF6F> [<b>NERV</b>]</color></font> [<b>HO.YO</b>]  [F RUE]<color=0xFFFFFFFF><b> -</b> <color=0x77ffffff><font size=10>to <b><color=0xffffffff></font>";
    const std::string DisruptLine = "[ 2024.06.13 04:58:49 ] (combat) <color=0xffffffff><b>Warp disruption attempt</b> <color=0x77ffffff><font size=10>from</font> <color=0xffffffff><b><font size=12><color=0xFFFFB900> <u><b>Atron</b></u></color></font>";
    const std::string DamageLine = "[ 2024.06.17 16:46:55 ] (combat) <color=0xffcc0000><b>43</b> <color=0x77ffffff><font size=10>from</font> <b><color=0xffffffff>Enforcer Drone</b><font size=10><color=0x77ffffff> - Heavy Missile - Hits";
    const std::string OutgoingLine = "[ 2024.06.17 16:47:01 ] (combat) <color=0xff00ffff><b>77</b> <color=0x77ffffff><font size=10>to</font> <b><color=0xffffffff>Enforcer Drone</b>";
    GameLogEventKind ParsedKind = GameLogEventKind::Damage;
    std::string ParsedSource;
    Check.Expect(GameLogWatcher::TryParseLine(ScrambleLine, &ParsedKind, &ParsedSource) == true && ParsedKind == GameLogEventKind::WarpScramble && ParsedSource.starts_with("Pontifex") == true, "warp scramble recognised with its source");
    Check.Expect(GameLogWatcher::TryParseLine(DisruptLine, &ParsedKind, &ParsedSource) == true && ParsedKind == GameLogEventKind::WarpDisruption && ParsedSource == "Atron", "warp disruption recognised");
    Check.Expect(GameLogWatcher::TryParseLine(DamageLine, &ParsedKind, &ParsedSource) == true && ParsedKind == GameLogEventKind::Damage && ParsedSource == "Enforcer Drone", "incoming damage recognised, weapon dropped from the source");
    Check.Expect(GameLogWatcher::TryParseLine(OutgoingLine, &ParsedKind, &ParsedSource) == false, "outgoing damage is not an attack");
    Check.Expect(GameLogWatcher::TryParseLine("[ 2024.06.13 04:52:00 ] (notify) Ship stopping", &ParsedKind, &ParsedSource) == false, "notifications are not attacks");
    Check.Expect(GameLogWatcher::StripMarkup("<b>Hello</b>   <color=0xffffffff>there </b>") == "Hello there", "markup is removed and blanks folded");

    const std::filesystem::path GameLogDirectory = TempDirectory / "Gamelogs";
    std::filesystem::create_directories(GameLogDirectory);
    const std::filesystem::path GameLogPath = GameLogDirectory / "20261008_100000_111.txt";
    const std::string GameLogHeader = "------------------------------\r\n  Gamelog\r\n  Listener: Pilot One\r\n  Session Started: 2026.10.08 10:00:00\r\n------------------------------\r\n";
    {
        std::ofstream Stream(GameLogPath, std::ios::binary | std::ios::trunc);
        Stream << GameLogHeader << DamageLine << "\r\n";
    }

    GameLogWatcher GameLogs;
    GameLogs.SetDirectory(GameLogDirectory);
    GameLogs.SetScanInterval(0);
    Check.Expect(GameLogs.Poll().empty() == true, "old fights in an existing game log are not replayed");
    {
        std::ofstream Stream(GameLogPath, std::ios::binary | std::ios::app);
        Stream << ScrambleLine << "\r\n" << OutgoingLine << "\r\n" << "[ 2026.10.08 10:05:00 ] (notify) hello\r\n" << DisruptLine;
    }

    const std::vector<GameLogEvent> GameEvents = GameLogs.Poll();
    Check.Expect(GameEvents.size() == 1 && GameEvents[0].Character == "Pilot One" && GameEvents[0].Kind == GameLogEventKind::WarpScramble, "new attack lines are reported for the listening character, the unfinished line waits");
    {
        std::ofstream Stream(GameLogPath, std::ios::binary | std::ios::app);
        Stream << "\r\n";
    }

    const std::vector<GameLogEvent> FinishedLine = GameLogs.Poll();
    Check.Expect(FinishedLine.size() == 1 && FinishedLine[0].Kind == GameLogEventKind::WarpDisruption, "the unfinished line is reported once it is complete");

    {
        std::ofstream Stream(GameLogDirectory / "20261008_110000_222.txt", std::ios::binary | std::ios::trunc);
        Stream << "  Gamelog\r\n  Listener: Pilot Two\r\n" << DamageLine << "\r\n";
    }

    const std::vector<GameLogEvent> NewSession = GameLogs.Poll();
    Check.Expect(NewSession.size() == 1 && NewSession[0].Character == "Pilot Two", "a log created after the start is read from its beginning");

    {
        UniverseData Bridged;
        Bridged.Load(CsvPath);
        const int Origin = Bridged.Find("Jita");
        const int Far = Bridged.Find("DY-F70");
        const int Before = UniverseNeighborhood::Build(Bridged, Origin, 1).Nodes.size() == 0 ? 0 : static_cast<int>(UniverseNeighborhood::Build(Bridged, Origin, 1).Nodes.size());
        const int Added = Bridged.AddBridges("Jita \xC2\xBB DY-F70\nDY-F70 <-> Perimeter\nnonsense\nJita -> Nowhere-Land\n  \nJita - Jita\n");
        Check.Expect(Added == 2, "two valid bridge lines are added, bad ones skipped");
        const std::vector<int>& Linked = Bridged.Get(Origin).Neighbours;
        Check.Expect(std::find(Linked.begin(), Linked.end(), Far) != Linked.end(), "a bridge makes a distant system a direct neighbour");
        Check.Expect(UniverseNeighborhood::Build(Bridged, Origin, 1).Nodes.size() > static_cast<size_t>(Before), "the bridge widens the one jump map");
    }

    Check.Expect(UpdateChecker::IsNewer("v1.0.2", "1.0.1") == true && UpdateChecker::IsNewer("v1.10.0", "1.9.9") == true, "a higher version is newer, compared by number");
    Check.Expect(UpdateChecker::IsNewer("v1.0.1", "1.0.1") == false && UpdateChecker::IsNewer("1.0", "1.0.0") == false && UpdateChecker::IsNewer("v1.0.0", "1.0.1") == false, "the same or older version is not newer");
    Check.Expect(UpdateChecker::IsNewer("latest", "1.0.1") == false && UpdateChecker::IsNewer("v2.0.0", "unknown") == false, "versions that cannot be read are never newer");
    std::string ReleaseTag;
    std::string ReleaseUrl;
    Check.Expect(UpdateChecker::ParseRelease("{\"tag_name\":\"v1.2.0\",\"html_url\":\"https://example.test/r\",\"other\":1}", &ReleaseTag, &ReleaseUrl) == true && ReleaseTag == "v1.2.0" && ReleaseUrl == "https://example.test/r", "release tag and page read from the response");
    Check.Expect(UpdateChecker::ParseRelease("{\"message\":\"Not Found\"}", &ReleaseTag, &ReleaseUrl) == false && UpdateChecker::ParseRelease("not json", &ReleaseTag, &ReleaseUrl) == false, "a response without a release is rejected");

    // D-scan reading, against the real ship table
    ShipCatalog Ships;
    const std::filesystem::path ShipPath = std::filesystem::path(CsvPath).parent_path() / "types.tsv";
    Check.Expect(Ships.Load(ShipPath) == true && Ships.Count() > 5000, "ship table loads");
    Check.Expect(Ships.FindById(22430) != nullptr && Ships.FindById(22430)->Name == "Sin" && Ships.FindById(22430)->Group == "Black Ops", "ship found by id");
    Check.Expect(Ships.FindByName("  pontifex ") != nullptr && Ships.FindByName("pontifex")->Id == 37481, "ship found by name, ignoring case and padding");
    Check.Expect(Ships.FindById(-5) == nullptr && Ships.FindByName("Not A Ship") == nullptr, "unknown ships are not found");

    const std::string Scan =
        "22430\tPilot Ship\tSin\t3.2 AU\n"
        "24702\tHurricane\tHurricane\t1,234 km\n"
        "24702\tHurricane\tHurricane\t500 m\n"
        "37481\tPontifex\tPontifex\t-\n"
        "25654\tEnforcer Drone\tEnforcer Drone\t12 km\r\n"
        "670\tCapsule\tCapsule\t2 AU\n"
        "99999999\tMystery\tMystery\t5 km\n";
    const DScanResult Summary = DScanAnalyzer::Analyze(Ships, Scan);
    Check.Expect(Summary.LooksLikeScan == true && Summary.Lines == 7 && Summary.Recognized == 6, "scan lines are counted and recognised");

    std::map<ShipClass, int> ClassTotals;
    for (const DScanClassCount& Entry : Summary.Classes)
    {
        ClassTotals[Entry.Class] = Entry.Count;
    }

    Check.Expect(ClassTotals[ShipClass::Combat] == 4 && ClassTotals[ShipClass::OtherShip] == 1 && ClassTotals[ShipClass::Npc] == 1 && ClassTotals[ShipClass::Unknown] == 1, "ships are sorted into classes");
    Check.Expect(Summary.Flags.size() == 2 && Summary.Flags[0].TypeName == "Sin" && Summary.Flags[1].TypeName == "Pontifex", "worrying ships are flagged, worst first");
    Check.Expect(Summary.Flags.size() == 2 && std::abs(Summary.Flags[0].NearestMeters / 149597870700.0 - 3.2) < 0.001 && Summary.Flags[1].NearestMeters < 0.0, "flag keeps the distance when the scan gave one");
    Check.Expect(DScanAnalyzer::Analyze(Ships, "hello\nthere\nthis is chat").LooksLikeScan == false, "ordinary text is not a scan");
    Check.Expect(DScanAnalyzer::Analyze(Ships, "").LooksLikeScan == false, "an empty clipboard is not a scan");
    Check.Expect(DScanAnalyzer::Analyze(Ships, "1\t2\t3\t4\n5\t6\t7\t8\n").LooksLikeScan == false, "numbers that are not ships are not a scan");

    double Meters = 0.0;
    Check.Expect(DScanAnalyzer::TryParseDistance("2.3 AU", &Meters) == true && std::abs(Meters / 149597870700.0 - 2.3) < 0.0001, "au distance");
    Check.Expect(DScanAnalyzer::TryParseDistance("2,3 AU", &Meters) == true && std::abs(Meters / 149597870700.0 - 2.3) < 0.0001, "au distance with a decimal comma");
    Check.Expect(DScanAnalyzer::TryParseDistance("1,234 km", &Meters) == true && Meters == 1234000.0, "km distance with a thousands separator");
    Check.Expect(DScanAnalyzer::TryParseDistance("500 m", &Meters) == true && Meters == 500.0, "metre distance");
    Check.Expect(DScanAnalyzer::TryParseDistance("-", &Meters) == false && DScanAnalyzer::TryParseDistance("far away", &Meters) == false, "no distance given");
    Check.Expect(DScanAnalyzer::FormatDistance(3.2 * 149597870700.0) == "3.2 AU" && DScanAnalyzer::FormatDistance(1234000.0) == "1234 km" && DScanAnalyzer::FormatDistance(500.0) == "500 m" && DScanAnalyzer::FormatDistance(-1.0) == "?", "distances are shown briefly");

    std::unordered_set<int> AllSystems;
    for (size_t Index = 0; Index < Data.Count(); Index++)
    {
        AllSystems.insert(static_cast<int>(Index));
    }

    if (RealMessages.size() == 2)
    {
        const std::vector<int> FirstHits = IntelParser::FindSystems(Data, RealMessages[0].Text, AllSystems);
        const std::vector<int> SecondHits = IntelParser::FindSystems(Data, RealMessages[1].Text, AllSystems);
        Check.Expect(std::find(FirstHits.begin(), FirstHits.end(), Data.Find("B-DBYQ")) != FirstHits.end(), "B-DBYQ found in real intel line");
        Check.Expect(std::find(SecondHits.begin(), SecondHits.end(), Data.Find("DY-F70")) != SecondHits.end(), "DY-F70 found in real intel line");
    }

    std::filesystem::remove_all(TempDirectory);

    const std::vector<std::filesystem::path> Bundled = AlertSound::GetBundledSounds();
    std::vector<std::wstring> BundledNames;
    for (const std::filesystem::path& Sound : Bundled)
    {
        BundledNames.push_back(Sound.filename().wstring());
    }

    for (const wchar_t* const Expected : {L"defaultWarning.wav"})
    {
        Check.Expect(std::find(BundledNames.begin(), BundledNames.end(), Expected) != BundledNames.end(), "bundled alert sound is installed next to the exe");
    }

    std::printf("%s\n", Check.FailureCount == 0 ? "OK" : "FAILED");
    return Check.FailureCount;
}

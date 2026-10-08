#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_set>

#include "Services/AlertSound.h"
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

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

#include <map>

#include "Services/CharacterNameResolver.h"
#include "Services/ProfileSyncer.h"

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

void WriteText(const std::filesystem::path& Path, const std::string& Text)
{
    std::ofstream Stream(Path, std::ios::binary | std::ios::trunc);
    Stream << Text;
}

std::string ReadText(const std::filesystem::path& Path)
{
    std::ifstream Stream(Path, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(Stream)), std::istreambuf_iterator<char>());
}

int main()
{
    Checker Check;

    Check.Expect(ProfileSyncer::Classify(L"core_user_123.dat") == ProfileSyncer::FileKind::User, "user file classified");
    Check.Expect(ProfileSyncer::Classify(L"C:\\Users\\Char\\CORE_CHAR_9.DAT") == ProfileSyncer::FileKind::Character, "character file classified case-insensitively, folder name ignored");
    Check.Expect(ProfileSyncer::Classify(L"core_user__.dat") == ProfileSyncer::FileKind::Unknown, "default user profile rejected");
    Check.Expect(ProfileSyncer::Classify(L"core_char__.dat") == ProfileSyncer::FileKind::Unknown, "default character profile rejected");
    Check.Expect(ProfileSyncer::Classify(L"core_public__.dat") == ProfileSyncer::FileKind::Unknown, "public profile rejected");
    Check.Expect(ProfileSyncer::Classify(L"core_user_12.ini") == ProfileSyncer::FileKind::Unknown, "wrong extension rejected");
    Check.Expect(ProfileSyncer::Classify(L"core_char_('char', None, 'dat').dat") == ProfileSyncer::FileKind::Unknown, "non numeric id rejected");

    Check.Expect(ProfileSyncer::GetIdentifier(L"C:\\x\\core_char_2112372278.dat") == 2112372278LL, "character id read");
    Check.Expect(ProfileSyncer::GetIdentifier(L"core_user__.dat") == 0, "default profile has no id");

    const std::map<long long, std::string> Parsed = CharacterNameResolver::ParseNames(
        R"([{"category":"character","id":2112372278,"name":"Test Pilot"},{"category":"corporation","id":98000001,"name":"Some Corp"},{"category":"character","id":"bad","name":"Bad"}])");
    Check.Expect(Parsed.size() == 1 && Parsed.at(2112372278LL) == "Test Pilot", "only valid characters parsed");
    Check.Expect(CharacterNameResolver::ParseNames("not json").empty() == true, "garbage response ignored");
    Check.Expect(CharacterNameResolver::ParseNames(R"({"error":"x"})").empty() == true, "error object ignored");

    const std::filesystem::path Root =std::filesystem::temp_directory_path() / L"SyncerProbe";
    std::filesystem::remove_all(Root);
    std::filesystem::create_directories(Root);

    WriteText(Root / L"core_user_1.dat", "master-user");
    WriteText(Root / L"core_char_1.dat", "master-char");
    WriteText(Root / L"core_user_2.dat", "old-user-2");
    WriteText(Root / L"core_user_3.dat", "old-user-3");
    WriteText(Root / L"core_char_2.dat", "old-char-2");
    WriteText(Root / L"core_user__.dat", "default-user");
    WriteText(Root / L"prefs.ini", "ini");

    Check.Expect(ProfileSyncer::ListFiles(Root, ProfileSyncer::FileKind::User).size() == 3, "three user files listed");
    Check.Expect(ProfileSyncer::ListFiles(Root, ProfileSyncer::FileKind::Character).size() == 2, "two character files listed");

    const std::filesystem::path EveRoot = Root / L"eve";
    std::filesystem::create_directories(EveRoot / L"install_a" / L"settings_Default");
    std::filesystem::create_directories(EveRoot / L"install_a" / L"settings_Empty");
    std::filesystem::create_directories(EveRoot / L"install_a" / L"other");
    WriteText(EveRoot / L"install_a" / L"settings_Default" / L"core_user_7.dat", "x");
    WriteText(EveRoot / L"install_a" / L"other" / L"core_user_8.dat", "x");
    const std::vector<std::filesystem::path> Folders = ProfileSyncer::FindSettingsFolders(EveRoot);
    Check.Expect(Folders.size() == 1 && Folders[0].filename() == L"settings_Default", "only settings folders holding profiles are found");

    Check.Expect(ProfileSyncer::FindUndoPoint(Root).Found == false, "no undo point before any sync");
    Check.Expect(ProfileSyncer::Undo(Root).Succeeded == false, "undo with nothing to undo fails");

    ProfileSyncer::Result Outcome = ProfileSyncer::Sync(Root / L"prefs.ini");
    Check.Expect(Outcome.Succeeded == false, "a file that is neither an account nor a character file cannot be the master");
    Check.Expect(std::filesystem::exists(Root / L"OriginalFiles") == false, "a rejected sync creates no backup");

    // The old tool left loose files and the app leaves used backups behind; neither may count as an undo point
    std::filesystem::create_directories(Root / L"OriginalFiles" / L"20200101-000000-undone");
    std::filesystem::create_directories(Root / L"OriginalFiles" / L"not-a-backup");
    WriteText(Root / L"OriginalFiles" / L"core_user_99.dat", "legacy");
    Check.Expect(ProfileSyncer::FindUndoPoint(Root).Found == false, "used backups, junk folders and legacy files are not undo points");

    Outcome = ProfileSyncer::Sync(Root / L"core_user_1.dat");
    Check.Expect(Outcome.Succeeded == true, "sync succeeds");
    Check.Expect(Outcome.FilesSynced == 2, "two accounts synced");
    Check.Expect(ReadText(Root / L"core_user_2.dat") == "master-user", "user 2 overwritten");
    Check.Expect(ReadText(Root / L"core_user_3.dat") == "master-user", "user 3 overwritten");
    Check.Expect(ReadText(Root / L"core_user_1.dat") == "master-user", "master untouched");
    Check.Expect(ReadText(Root / L"core_char_1.dat") == "master-char", "character master untouched");
    Check.Expect(ReadText(Root / L"core_char_2.dat") == "old-char-2", "character files are never synced");
    Check.Expect(ReadText(Root / L"core_user__.dat") == "default-user", "default profile untouched");
    Check.Expect(ReadText(Root / L"prefs.ini") == "ini", "ini untouched");

    const ProfileSyncer::UndoPoint FirstPoint = ProfileSyncer::FindUndoPoint(Root);
    Check.Expect(FirstPoint.Found == true && FirstPoint.FileCount == 2, "sync leaves an undo point holding both originals");
    Check.Expect(ReadText(FirstPoint.Folder / L"core_user_2.dat") == "old-user-2", "backup holds the original");
    Check.Expect(ProfileSyncer::FormatStamp("20261007-185400") == "2026-10-07 18:54:00", "stamp formatted");
    Check.Expect(ProfileSyncer::FormatStamp("20261007-185400-2") == "2026-10-07 18:54:00", "stamp with sequence formatted");

    WriteText(Root / L"core_user_1.dat", "master-v2");
    Outcome = ProfileSyncer::Sync(Root / L"core_user_1.dat");
    Check.Expect(Outcome.Succeeded == true, "second sync succeeds");
    Check.Expect(ReadText(Root / L"core_user_2.dat") == "master-v2", "second sync overwrites again");

    const ProfileSyncer::UndoPoint SecondPoint = ProfileSyncer::FindUndoPoint(Root);
    Check.Expect(SecondPoint.Folder != FirstPoint.Folder, "a second sync never reuses a backup folder, even within one second");
    Check.Expect(ReadText(FirstPoint.Folder / L"core_user_2.dat") == "old-user-2", "the first backup is intact");
    Check.Expect(ReadText(SecondPoint.Folder / L"core_user_2.dat") == "master-user", "the second backup holds the state before the second sync");

    Outcome = ProfileSyncer::Undo(Root);
    Check.Expect(Outcome.Succeeded == true && Outcome.FilesSynced == 2, "undo restores both accounts");
    Check.Expect(ReadText(Root / L"core_user_2.dat") == "master-user", "first undo goes back one sync");
    Check.Expect(ReadText(Root / L"core_char_2.dat") == "old-char-2", "undo leaves character files alone");
    Check.Expect(std::filesystem::exists(SecondPoint.Folder) == false, "the used backup is no longer offered");
    Check.Expect(std::filesystem::exists(SecondPoint.Folder.string() + "-undone") == true, "the used backup is kept");

    Outcome = ProfileSyncer::Undo(Root);
    Check.Expect(Outcome.Succeeded == true, "second undo succeeds");
    Check.Expect(ReadText(Root / L"core_user_2.dat") == "old-user-2", "second undo restores the original");
    Check.Expect(ReadText(Root / L"core_user_3.dat") == "old-user-3", "second undo restores every account");
    Check.Expect(ReadText(Root / L"core_user_1.dat") == "master-v2", "undo never touches accounts that were not replaced");
    Check.Expect(ProfileSyncer::FindUndoPoint(Root).Found == false, "nothing is left to undo");
    Check.Expect(ReadText(Root / L"OriginalFiles" / L"core_user_99.dat") == "legacy", "legacy files are left alone");

    WriteText(Root / L"core_char_3.dat", "old-char-3");
    Outcome = ProfileSyncer::Sync(Root / L"core_char_1.dat");
    Check.Expect(Outcome.Succeeded == true && Outcome.FilesSynced == 2, "a character sync replaces the other two characters");
    Check.Expect(ReadText(Root / L"core_char_2.dat") == "master-char" && ReadText(Root / L"core_char_3.dat") == "master-char", "other characters overwritten");
    Check.Expect(ReadText(Root / L"core_char_1.dat") == "master-char", "character master untouched");
    Check.Expect(ReadText(Root / L"core_user_2.dat") == "old-user-2" && ReadText(Root / L"core_user_1.dat") == "master-v2", "a character sync leaves account files alone");

    const ProfileSyncer::UndoPoint CharacterPoint = ProfileSyncer::FindUndoPoint(Root);
    Check.Expect(CharacterPoint.Found == true && CharacterPoint.FileCount == 2, "a character sync leaves an undo point holding both originals");
    Check.Expect(ReadText(CharacterPoint.Folder / L"core_char_2.dat") == "old-char-2", "character backup holds the original");

    Outcome = ProfileSyncer::Undo(Root);
    Check.Expect(Outcome.Succeeded == true && Outcome.FilesSynced == 2, "undo restores both characters");
    Check.Expect(ReadText(Root / L"core_char_2.dat") == "old-char-2" && ReadText(Root / L"core_char_3.dat") == "old-char-3", "characters back to their originals");
    Check.Expect(ReadText(Root / L"core_user_2.dat") == "old-user-2", "character undo leaves account files alone");

    std::filesystem::remove_all(Root);
    if (Check.FailureCount == 0)
    {
        std::printf("ALL PASSED\n");
        return 0;
    }

    std::printf("%d FAILED\n", Check.FailureCount);
    return 1;
}

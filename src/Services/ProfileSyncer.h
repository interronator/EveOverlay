#pragma once

#include <filesystem>
#include <string>
#include <vector>

// Copies one master user (account) file over every other user file in the EVE settings folder, keeping a backup of each overwritten file
class ProfileSyncer
{
public:
    enum class FileKind
    {
        Unknown,
        User,
        Character
    };

    struct Result
    {
        bool Succeeded = false;
        int FilesSynced = 0;
        std::string Message;
    };

    static FileKind Classify(const std::filesystem::path& FilePath);

    // The numeric id in core_user_<id>.dat / core_char_<id>.dat, or 0 for any other file
    static long long GetIdentifier(const std::filesystem::path& FilePath);

    static std::vector<std::filesystem::path> ListFiles(const std::filesystem::path& Directory, const FileKind Kind);

    // EVE keeps profiles in <root>\<install>\settings_<name>; only folders that hold at least one profile file are returned
    static std::vector<std::filesystem::path> FindSettingsFolders(const std::filesystem::path& Root);

    struct UndoPoint
    {
        bool Found = false;
        std::filesystem::path Folder;
        std::string Stamp;
        int FileCount = 0;
    };

    static Result Sync(const std::filesystem::path& UserMaster);

    // The newest backup in <Directory>\OriginalFiles that has not been undone yet; read from disk so it survives restarts
    static UndoPoint FindUndoPoint(const std::filesystem::path& Directory);

    // Restores the newest backup over the settings files, then marks it as used so the next undo goes one sync further back
    static Result Undo(const std::filesystem::path& Directory);

    // 20261007-185400 (or 20261007-185400-2) as 2026-10-07 18:54:00
    static std::string FormatStamp(const std::string& Stamp);

private:
    static constexpr const wchar_t* USER_PREFIX = L"core_user_";
    static constexpr const wchar_t* CHARACTER_PREFIX = L"core_char_";
    static constexpr const wchar_t* EXTENSION = L".dat";
    static constexpr const char* BACKUP_FOLDER = "OriginalFiles";
    static constexpr const char* UNDONE_SUFFIX = "-undone";
    static constexpr size_t STAMP_LENGTH = 15;
    static constexpr size_t MAX_IDENTIFIER_DIGITS = 18;

    struct BackupName
    {
        bool Valid = false;
        std::string Stamp;
        int Sequence = 0;
    };

    static std::wstring ToLower(std::wstring Text);

    // Accepts yyyymmdd-hhmmss with an optional -<number> suffix, and nothing else (so "-undone" folders and the old tool's files are ignored)
    static BackupName ParseBackupName(const std::string& Name);

    // A same-second sync, or an undone backup with the same stamp, must not reuse a folder
    static std::filesystem::path ChooseBackupDirectory(const std::filesystem::path& Directory);

    // Rejects the default profiles (core_user__.dat) and anything whose id is not a plain number
    static bool HasNumericSuffix(const std::wstring& Name, const std::wstring& Prefix);

    static bool IsValidMaster(const std::filesystem::path& FilePath, const FileKind Kind);
    static bool CollectTargets(const std::filesystem::path& Directory, const std::filesystem::path& UserMaster, std::vector<std::filesystem::path>& Targets);
};

#include "Services/ProfileSyncer.h"

#include "Application/Logger.h"
#include "Config/TextUtil.h"

#include <algorithm>
#include <ctime>
#include <cwctype>
#include <system_error>

ProfileSyncer::FileKind ProfileSyncer::Classify(const std::filesystem::path& FilePath)
{
    const std::wstring Name = TextUtil::ToLower(FilePath.filename().wstring());
    if (HasNumericSuffix(Name, USER_PREFIX) == true)
    {
        return FileKind::User;
    }

    if (HasNumericSuffix(Name, CHARACTER_PREFIX) == true)
    {
        return FileKind::Character;
    }

    return FileKind::Unknown;
}

long long ProfileSyncer::GetIdentifier(const std::filesystem::path& FilePath)
{
    if (Classify(FilePath) == FileKind::Unknown)
    {
        return 0;
    }

    const std::wstring Stem = FilePath.stem().wstring();
    const std::wstring Digits = Stem.substr(Stem.find_last_of(L'_') + 1);
    if (Digits.size() > MAX_IDENTIFIER_DIGITS)
    {
        return 0;
    }

    long long Identifier = 0;
    for (const wchar_t Digit : Digits)
    {
        Identifier = Identifier * 10 + (Digit - L'0');
    }

    return Identifier;
}

std::vector<std::filesystem::path> ProfileSyncer::ListFiles(const std::filesystem::path& Directory, const FileKind Kind)
{
    std::vector<std::filesystem::path> Files;
    std::error_code ErrorCode;
    std::filesystem::directory_iterator Current(Directory, ErrorCode);
    for (; ErrorCode.value() == 0 && Current != std::filesystem::directory_iterator(); Current.increment(ErrorCode))
    {
        if (Classify(Current->path()) != Kind || Current->is_regular_file(ErrorCode) == false)
        {
            continue;
        }

        Files.push_back(Current->path());
    }

    std::sort(Files.begin(), Files.end());
    return Files;
}

std::vector<std::filesystem::path> ProfileSyncer::FindSettingsFolders(const std::filesystem::path& Root)
{
    std::vector<std::filesystem::path> Folders;
    std::error_code ErrorCode;
    std::filesystem::directory_iterator Install(Root, ErrorCode);
    for (; ErrorCode.value() == 0 && Install != std::filesystem::directory_iterator(); Install.increment(ErrorCode))
    {
        if (Install->is_directory(ErrorCode) == false)
        {
            continue;
        }

        std::error_code InnerError;
        std::filesystem::directory_iterator Settings(Install->path(), InnerError);
        for (; InnerError.value() == 0 && Settings != std::filesystem::directory_iterator(); Settings.increment(InnerError))
        {
            const std::wstring Name = TextUtil::ToLower(Settings->path().filename().wstring());
            if (Name.starts_with(L"settings_") == false || Settings->is_directory(InnerError) == false)
            {
                continue;
            }

            if (ListFiles(Settings->path(), FileKind::User).empty() == true && ListFiles(Settings->path(), FileKind::Character).empty() == true)
            {
                continue;
            }

            Folders.push_back(Settings->path());
        }
    }

    std::sort(Folders.begin(), Folders.end());
    return Folders;
}

ProfileSyncer::Result ProfileSyncer::Sync(const std::filesystem::path& Master)
{
    Result Outcome;
    const FileKind Kind = Classify(Master);
    if (Kind == FileKind::Unknown || IsValidMaster(Master, Kind) == false)
    {
        Outcome.Message = "The chosen file is missing or is not a core_user_<number>.dat or core_char_<number>.dat file.";
        return Outcome;
    }

    const std::string Noun = Kind == FileKind::User ? "accounts" : "characters";
    const std::filesystem::path Directory = Master.parent_path();
    std::vector<std::filesystem::path> Targets;
    if (CollectTargets(Directory, Master, Kind, Targets) == false)
    {
        Outcome.Message = "Could not read the settings folder.";
        return Outcome;
    }

    if (Targets.empty() == true)
    {
        Outcome.Message = "No other " + Noun + " files were found in the folder.";
        return Outcome;
    }

    const std::filesystem::path BackupDirectory = ChooseBackupDirectory(Directory);
    std::error_code ErrorCode;
    std::filesystem::create_directories(BackupDirectory, ErrorCode);
    if (ErrorCode.value() != 0)
    {
        Outcome.Message = "Could not create the backup folder, nothing was changed.";
        return Outcome;
    }

    for (const std::filesystem::path& Target : Targets)
    {
        std::filesystem::copy_file(Target, BackupDirectory / Target.filename(), std::filesystem::copy_options::overwrite_existing, ErrorCode);
        if (ErrorCode.value() != 0)
        {
            Logger::Error("Profile sync could not back up " + Logger::DescribePath(Target) + ": " + ErrorCode.message());
            std::filesystem::remove_all(BackupDirectory, ErrorCode);
            Outcome.Message = "Could not back up " + Target.filename().string() + ", nothing was changed.";
            return Outcome;
        }
    }

    for (const std::filesystem::path& Target : Targets)
    {
        std::filesystem::copy_file(Master, Target, std::filesystem::copy_options::overwrite_existing, ErrorCode);
        if (ErrorCode.value() != 0)
        {
            Logger::Error("Profile sync could not overwrite " + Logger::DescribePath(Target) + ": " + ErrorCode.message());
            Outcome.Message = "Could not overwrite " + Target.filename().string() + " after " + std::to_string(Outcome.FilesSynced) + " files. Use Undo to restore them.";
            return Outcome;
        }

        Outcome.FilesSynced++;
    }

    Outcome.Succeeded = true;
    Outcome.Message = "Synced " + std::to_string(Outcome.FilesSynced) + " " + Noun + ". Use Undo to restore the originals.";
    return Outcome;
}

ProfileSyncer::UndoPoint ProfileSyncer::FindUndoPoint(const std::filesystem::path& Directory)
{
    UndoPoint Newest;
    BackupName NewestName;
    std::error_code ErrorCode;
    std::filesystem::directory_iterator Current(Directory / BACKUP_FOLDER, ErrorCode);
    for (; ErrorCode.value() == 0 && Current != std::filesystem::directory_iterator(); Current.increment(ErrorCode))
    {
        if (Current->is_directory(ErrorCode) == false)
        {
            continue;
        }

        const BackupName Name = ParseBackupName(Current->path().filename().string());
        if (Name.Valid == false)
        {
            continue;
        }

        const bool IsNewer = Newest.Found == false || Name.Stamp > NewestName.Stamp || (Name.Stamp == NewestName.Stamp && Name.Sequence > NewestName.Sequence);
        if (IsNewer == false)
        {
            continue;
        }

        Newest.Found = true;
        Newest.Folder = Current->path();
        Newest.Stamp = Current->path().filename().string();
        NewestName = Name;
    }

    if (Newest.Found == true)
    {
        Newest.FileCount = static_cast<int>(ListFiles(Newest.Folder, FileKind::User).size() + ListFiles(Newest.Folder, FileKind::Character).size());
    }

    return Newest;
}

ProfileSyncer::Result ProfileSyncer::Undo(const std::filesystem::path& Directory)
{
    Result Outcome;
    const UndoPoint Point = FindUndoPoint(Directory);
    if (Point.Found == false)
    {
        Outcome.Message = "There is nothing to undo.";
        return Outcome;
    }

    std::vector<std::filesystem::path> Backups = ListFiles(Point.Folder, FileKind::User);
    const std::vector<std::filesystem::path> CharacterBackups = ListFiles(Point.Folder, FileKind::Character);
    Backups.insert(Backups.end(), CharacterBackups.begin(), CharacterBackups.end());

    std::error_code ErrorCode;
    for (const std::filesystem::path& Backup : Backups)
    {
        std::filesystem::copy_file(Backup, Directory / Backup.filename(), std::filesystem::copy_options::overwrite_existing, ErrorCode);
        if (ErrorCode.value() != 0)
        {
            Logger::Error("Profile undo could not restore " + Logger::DescribePath(Backup) + ": " + ErrorCode.message());
            Outcome.Message = "Could not restore " + Backup.filename().string() + " after " + std::to_string(Outcome.FilesSynced) + " files. The backup was kept; try Undo again.";
            return Outcome;
        }

        Outcome.FilesSynced++;
    }

    std::filesystem::path UsedFolder = Point.Folder;
    UsedFolder += UNDONE_SUFFIX;
    std::filesystem::rename(Point.Folder, UsedFolder, ErrorCode);

    Outcome.Succeeded = true;
    Outcome.Message = "Restored " + std::to_string(Outcome.FilesSynced) + " files to how they were before the sync of " + FormatStamp(Point.Stamp) + " UTC.";
    if (ErrorCode.value() != 0)
    {
        Outcome.Message += " The backup folder could not be marked as used.";
    }

    return Outcome;
}

std::string ProfileSyncer::FormatStamp(const std::string& Stamp)
{
    if (Stamp.size() < STAMP_LENGTH)
    {
        return Stamp;
    }

    return Stamp.substr(0, 4) + "-" + Stamp.substr(4, 2) + "-" + Stamp.substr(6, 2) + " " + Stamp.substr(9, 2) + ":" + Stamp.substr(11, 2) + ":" + Stamp.substr(13, 2);
}

ProfileSyncer::BackupName ProfileSyncer::ParseBackupName(const std::string& Name)
{
    BackupName Parsed;
    if (Name.size() < STAMP_LENGTH)
    {
        return Parsed;
    }

    for (size_t Index = 0; Index < STAMP_LENGTH; Index++)
    {
        const bool IsSeparator = Index == 8;
        const bool IsDigit = Name[Index] >= '0' && Name[Index] <= '9';
        if ((IsSeparator == true && Name[Index] != '-') || (IsSeparator == false && IsDigit == false))
        {
            return Parsed;
        }
    }

    if (Name.size() > STAMP_LENGTH)
    {
        if (Name[STAMP_LENGTH] != '-' || Name.size() == STAMP_LENGTH + 1 || Name.size() > STAMP_LENGTH + 5)
        {
            return Parsed;
        }

        for (size_t Index = STAMP_LENGTH + 1; Index < Name.size(); Index++)
        {
            if (Name[Index] < '0' || Name[Index] > '9')
            {
                return Parsed;
            }

            Parsed.Sequence = Parsed.Sequence * 10 + (Name[Index] - '0');
        }
    }

    Parsed.Valid = true;
    Parsed.Stamp = Name.substr(0, STAMP_LENGTH);
    return Parsed;
}

std::filesystem::path ProfileSyncer::ChooseBackupDirectory(const std::filesystem::path& Directory)
{
    const std::time_t Now = std::time(nullptr);
    std::tm LocalTime = {};
    ::gmtime_s(&LocalTime, &Now);

    char Buffer[32] = {};
    std::strftime(Buffer, sizeof(Buffer), "%Y%m%d-%H%M%S", &LocalTime);

    const std::filesystem::path Root = Directory / BACKUP_FOLDER;
    std::string Name = Buffer;
    std::error_code ErrorCode;
    for (int Sequence = 2; std::filesystem::exists(Root / Name, ErrorCode) == true || std::filesystem::exists(Root / (Name + UNDONE_SUFFIX), ErrorCode) == true; Sequence++)
    {
        Name = std::string(Buffer) + "-" + std::to_string(Sequence);
    }

    return Root / Name;
}

bool ProfileSyncer::HasNumericSuffix(const std::wstring& Name, const std::wstring& Prefix)
{
    const std::wstring Extension = EXTENSION;
    if (Name.starts_with(Prefix) == false || Name.ends_with(Extension) == false || Name.size() <= Prefix.size() + Extension.size())
    {
        return false;
    }

    const std::wstring Identifier = Name.substr(Prefix.size(), Name.size() - Prefix.size() - Extension.size());
    for (const wchar_t Character : Identifier)
    {
        if (Character < L'0' || Character > L'9')
        {
            return false;
        }
    }

    return true;
}

bool ProfileSyncer::IsValidMaster(const std::filesystem::path& FilePath, const FileKind Kind)
{
    std::error_code ErrorCode;
    return Classify(FilePath) == Kind && std::filesystem::is_regular_file(FilePath, ErrorCode) == true;
}

bool ProfileSyncer::CollectTargets(const std::filesystem::path& Directory, const std::filesystem::path& Master, const FileKind Kind, std::vector<std::filesystem::path>& Targets)
{
    std::error_code ErrorCode;
    std::filesystem::directory_iterator Current(Directory, ErrorCode);
    if (ErrorCode.value() != 0)
    {
        return false;
    }

    const std::wstring MasterName = TextUtil::ToLower(Master.filename().wstring());
    for (; Current != std::filesystem::directory_iterator(); Current.increment(ErrorCode))
    {
        if (ErrorCode.value() != 0)
        {
            return false;
        }

        if (Current->is_regular_file(ErrorCode) == false || Classify(Current->path()) != Kind)
        {
            continue;
        }

        if (TextUtil::ToLower(Current->path().filename().wstring()) == MasterName)
        {
            continue;
        }

        Targets.push_back(Current->path());
    }

    return ErrorCode.value() == 0;
}

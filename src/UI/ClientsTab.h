#pragma once

#include <string>
#include <vector>

#include "Application/Signal.h"
#include "UI/ITabPage.h"

class ClientsTab : public ITabPage
{
public:
    // Title, disabled
    Signal<const std::wstring&, bool> ThumbnailStateChanged;

    const std::string& GetTitle() const override;
    const std::string& GetDescription() const override;
    void Draw() override;
    void LoadFromConfiguration(const ThumbnailConfiguration&) override;
    void StoreToConfiguration(ThumbnailConfiguration&) const override;

    void AddThumbnail(const std::wstring& ClientTitle, const bool IsDisabled);
    void RemoveThumbnail(const std::wstring& ClientTitle);
    void SetThumbnailDisabled(const size_t Index, const bool IsDisabled);
    bool IsThumbnailDisabled(const size_t Index) const;
    int GetEnabledThumbnailCount() const;
    int GetThumbnailCount() const;

private:
    struct Client
    {
        std::wstring Title;
        std::string Label;
        bool Disabled;
    };

    const std::string Title = "Active Clients";
    const std::string Description = "Running EVE clients and their previews.";

    std::vector<Client> Clients;
};

#pragma once

#include <string>

#include "Services/UpdateChecker.h"
#include "UI/ITabPage.h"

class AboutTab : public ITabPage
{
public:
    AboutTab();

    const std::string& GetTitle() const override;
    const std::string& GetDescription() const override;
    void Draw() override;
    void LoadFromConfiguration(const ThumbnailConfiguration&) override;
    void StoreToConfiguration(ThumbnailConfiguration&) const override;

private:
    static void DrawBullet(const char* const Text);

    const std::string Title = "About";
    const std::string Description = "Version and project information.";

    std::string Name;
    std::string Version;
    UpdateChecker Updates;
};

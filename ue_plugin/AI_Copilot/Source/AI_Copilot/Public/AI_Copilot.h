#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FUICommandList;
class SDockTab;
class FSpawnTabArgs;

class FAI_CopilotModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    void RegisterTabSpawner();
    void RegisterMenus();
    void OpenCopilotTab();

    TSharedRef<SDockTab> SpawnCopilotTab(const FSpawnTabArgs& SpawnTabArgs);

private:
    TSharedPtr<FUICommandList> PluginCommands;
};

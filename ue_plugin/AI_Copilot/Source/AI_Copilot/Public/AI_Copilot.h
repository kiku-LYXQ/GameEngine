#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Framework/Commands/UICommandList.h"

class FSpawnTabArgs;
class SDockTab;

class FAI_CopilotModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    void RegisterMenus();
    void RegisterTabSpawner();
    TSharedRef<SDockTab> SpawnCopilotTab(const FSpawnTabArgs& SpawnTabArgs);
    void OpenCopilotTab();

private:
    TSharedPtr<FUICommandList> PluginCommands;
};

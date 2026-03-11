#include "AI_Copilot.h"
#include "AI_CopilotCommands.h"
#include "CopilotPanel.h"

#include "Framework/Commands/UIAction.h"
#include "Framework/Docking/TabManager.h"
#include "LevelEditor.h"
#include "ToolMenus.h"
#include "ToolMenuEntry.h"
#include "ToolMenus/ToolMenuOwnerScoped.h"
#include "WorkspaceMenuStructureModule.h"
#include "Widgets/Docking/SDockTab.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateIcon.h"

#define LOCTEXT_NAMESPACE "FAI_CopilotModule"

static const FName AI_CopilotTabName("AI_Copilot");

void FAI_CopilotModule::StartupModule()
{
    FAICopilotCommands::Register();

    PluginCommands = MakeShared<FUICommandList>();
    PluginCommands->MapAction(
        FAICopilotCommands::Get().OpenCopilot,
        FExecuteAction::CreateRaw(this, &FAI_CopilotModule::OpenCopilotTab),
        FCanExecuteAction());

    RegisterTabSpawner();
    UToolMenus::RegisterStartupCallback(
        FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FAI_CopilotModule::RegisterMenus));
}

void FAI_CopilotModule::ShutdownModule()
{
    if (UToolMenus::IsToolMenuUIEnabled())
    {
        UToolMenus::UnRegisterStartupCallback(this);
        UToolMenus::UnregisterOwner(this);
    }

    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(AI_CopilotTabName);

    FAICopilotCommands::Unregister();
    PluginCommands.Reset();
}

void FAI_CopilotModule::RegisterTabSpawner()
{
    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
        AI_CopilotTabName,
        FOnSpawnTab::CreateRaw(this, &FAI_CopilotModule::SpawnCopilotTab))
        .SetDisplayName(LOCTEXT("AICopilotTabTitle", "AI Copilot"))
        .SetTooltipText(LOCTEXT("AICopilotTabTooltip", "Open the AI Copilot workspace"))
        .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.GameSettings"))
        .SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory());
}

void FAI_CopilotModule::RegisterMenus()
{
    if (!UToolMenus::IsToolMenuUIEnabled())
    {
        return;
    }

    FToolMenuOwnerScoped OwnerScoped(this);

    if (UToolMenu* WindowMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window"))
    {
        FToolMenuSection& WindowSection = WindowMenu->FindOrAddSection("WindowLayout");
        WindowSection.AddMenuEntry(
            "AI_Copilot_Main",
            LOCTEXT("AICopilotMenuLabel", "AI Copilot"),
            LOCTEXT("AICopilotMenuTooltip", "Open the AI Copilot panel."),
            FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.GameSettings"),
            FUIAction(FExecuteAction::CreateRaw(this, &FAI_CopilotModule::OpenCopilotTab)));
    }

    if (UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar"))
    {
        FToolMenuSection& ToolbarSection = ToolbarMenu->FindOrAddSection("AI_Copilot");
        ToolbarSection.AddEntry(FToolMenuEntry::InitToolBarButton(
            "AI_Copilot_Open",
            FUIAction(FExecuteAction::CreateRaw(this, &FAI_CopilotModule::OpenCopilotTab)),
            LOCTEXT("AICopilotToolbarLabel", "AI Copilot"),
            LOCTEXT("AICopilotToolbarTooltip", "Open the AI Copilot panel."),
            FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.GameSettings")));
    }
}

TSharedRef<SDockTab> FAI_CopilotModule::SpawnCopilotTab(const FSpawnTabArgs& SpawnTabArgs)
{
    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        .Label(LOCTEXT("AICopilotTabLabel", "AI Copilot"))
        [
            SNew(SCopilotPanel)
        ];
}

void FAI_CopilotModule::OpenCopilotTab()
{
    FGlobalTabmanager::Get()->TryInvokeTab(AI_CopilotTabName);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAI_CopilotModule, AI_Copilot)

#include "AI_Copilot.h"
#include "Misc/MessageDialog.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "FAI_CopilotModule"

void FAI_CopilotModule::StartupModule()
{
    FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("Startup", "AI Copilot module started."));
}

void FAI_CopilotModule::ShutdownModule()
{
    FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("Shutdown", "AI Copilot module shutting down."));
}

#undef LOCTEXT_NAMESPACE

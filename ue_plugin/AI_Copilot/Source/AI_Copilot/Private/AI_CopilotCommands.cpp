#include "AI_CopilotCommands.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "FAICopilotCommands"

FAICopilotCommands::FAICopilotCommands()
    : TCommands<FAICopilotCommands>(
          TEXT("AI_Copilot"),
          LOCTEXT("ContextName", "AI Copilot"),
          NAME_None,
          FAppStyle::GetAppStyleSetName())
{
}

void FAICopilotCommands::RegisterCommands()
{
    UI_COMMAND(
        OpenCopilot,
        "AI Copilot",
        "Open the AI Copilot panel.",
        EUserInterfaceActionType::Button,
        FInputChord());
}

#undef LOCTEXT_NAMESPACE

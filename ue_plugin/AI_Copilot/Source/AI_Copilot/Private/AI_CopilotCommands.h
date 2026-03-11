#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"

class FAICopilotCommands : public TCommands<FAICopilotCommands>
{
public:
    FAICopilotCommands();

    virtual void RegisterCommands() override;

public:
    TSharedPtr<FUICommandInfo> OpenCopilot;
};

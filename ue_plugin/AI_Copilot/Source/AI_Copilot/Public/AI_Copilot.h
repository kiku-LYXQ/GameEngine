#pragma once

#include "Modules/ModuleManager.h"

class FAI_CopilotModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};

#pragma once

#include "Modules/ModuleManager.h"

class PUERTSRUNTIMEPLUGIN_API FPuertsRuntimePluginModule final : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};

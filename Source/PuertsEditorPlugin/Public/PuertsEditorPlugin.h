#pragma once

#include "Modules/ModuleManager.h"

class PUERTSEDITORPLUGIN_API FPuertsEditorPluginModule final : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
    virtual bool SupportsDynamicReloading() override { return false; }
};

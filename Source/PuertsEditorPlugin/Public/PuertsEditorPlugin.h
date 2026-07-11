#pragma once

#include "Modules/ModuleManager.h"
#include "Templates/UniquePtr.h"

class FPuertsEditorPluginState;

class PUERTSEDITORPLUGIN_API FPuertsEditorPluginModule final : public IModuleInterface
{
public:
    FPuertsEditorPluginModule();
    virtual ~FPuertsEditorPluginModule() override;

    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
    virtual bool SupportsDynamicReloading() override { return false; }

    FPuertsEditorPluginState& GetState();

private:
    TUniquePtr<FPuertsEditorPluginState> State;
};

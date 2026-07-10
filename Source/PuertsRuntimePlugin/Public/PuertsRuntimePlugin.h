#pragma once

#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogPuertsRuntimePlugin, Log, All);

class UPuertsRuntimeGameInstanceSubsystem;
DECLARE_MULTICAST_DELEGATE_OneParam(FPuertsRuntimeInstanceEvent, UPuertsRuntimeGameInstanceSubsystem*);

class PUERTSRUNTIMEPLUGIN_API FPuertsRuntimePluginModule final : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    static FPuertsRuntimeInstanceEvent& OnRuntimeInstanceCreated();
    static FPuertsRuntimeInstanceEvent& OnRuntimeInstanceDestroyed();

private:
    static FPuertsRuntimeInstanceEvent RuntimeInstanceCreatedEvent;
    static FPuertsRuntimeInstanceEvent RuntimeInstanceDestroyedEvent;
};

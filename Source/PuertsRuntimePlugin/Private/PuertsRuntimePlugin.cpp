#include "PuertsRuntimePlugin.h"

DEFINE_LOG_CATEGORY(LogPuertsRuntimePlugin);

FPuertsRuntimeInstanceEvent FPuertsRuntimePluginModule::RuntimeInstanceCreatedEvent;
FPuertsRuntimeInstanceEvent FPuertsRuntimePluginModule::RuntimeInstanceDestroyedEvent;

void FPuertsRuntimePluginModule::StartupModule()
{
}

void FPuertsRuntimePluginModule::ShutdownModule()
{
    RuntimeInstanceCreatedEvent.Clear();
    RuntimeInstanceDestroyedEvent.Clear();
}

FPuertsRuntimeInstanceEvent& FPuertsRuntimePluginModule::OnRuntimeInstanceCreated()
{
    return RuntimeInstanceCreatedEvent;
}

FPuertsRuntimeInstanceEvent& FPuertsRuntimePluginModule::OnRuntimeInstanceDestroyed()
{
    return RuntimeInstanceDestroyedEvent;
}

IMPLEMENT_MODULE(FPuertsRuntimePluginModule, PuertsRuntimePlugin)

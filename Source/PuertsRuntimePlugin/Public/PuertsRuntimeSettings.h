#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "PuertsRuntimeSettings.generated.h"

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Puerts Runtime"))
class PUERTSRUNTIMEPLUGIN_API UPuertsRuntimeSettings final : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UPROPERTY(Config, EditAnywhere, Category = "Startup")
    bool bAutoStart = true;

    UPROPERTY(Config, EditAnywhere, Category = "Startup")
    FString EntryModule = TEXT("Main");

    UPROPERTY(Config, EditAnywhere, Category = "Modules")
    FString ProjectScriptRoot = TEXT("JavaScript");

    UPROPERTY(Config, EditAnywhere, Category = "Modules")
    FString PluginScriptRoot = TEXT("JavaScript");

    UPROPERTY(Config, EditAnywhere, Category = "Debug", meta = (ClampMin = "-1", ClampMax = "65535"))
    int32 DebugPort = -1;

    virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
};

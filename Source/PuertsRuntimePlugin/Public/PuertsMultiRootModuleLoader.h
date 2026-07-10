#pragma once

#include "CoreMinimal.h"
#include "JSModuleLoader.h"

struct PUERTSRUNTIMEPLUGIN_API FPuertsScriptRoot
{
    FString Name;
    FString AbsolutePath;
};

class PUERTSRUNTIMEPLUGIN_API FPuertsMultiRootModuleLoader final : public puerts::IJSModuleLoader
{
public:
    explicit FPuertsMultiRootModuleLoader(TArray<FPuertsScriptRoot> InRoots);

    virtual bool Search(
        const FString& RequiredDir, const FString& RequiredModule, FString& Path, FString& AbsolutePath) override;
    virtual bool Load(const FString& Path, TArray<uint8>& Content) override;
    virtual FString& GetScriptRoot() override;

    const TArray<FPuertsScriptRoot>& GetRoots() const { return Roots; }
    const FString& GetLastSearchDiagnostic() const { return LastSearchDiagnostic; }

    static FString CanonicalizePath(const FString& Path);

private:
    bool SearchFromDirectory(const FString& Directory, const FString& RequiredModule, bool bSearchNodeModules,
        FString& Path, FString& AbsolutePath) const;
    bool SearchNodeModulesUpward(
        const FString& RequiredDir, const FString& RequiredModule, FString& Path, FString& AbsolutePath) const;
    bool CheckCandidate(const FString& Candidate, FString& Path, FString& AbsolutePath) const;

    TArray<FPuertsScriptRoot> Roots;
    FString ScriptRoot = TEXT("JavaScript");
    FString LastSearchDiagnostic;
};

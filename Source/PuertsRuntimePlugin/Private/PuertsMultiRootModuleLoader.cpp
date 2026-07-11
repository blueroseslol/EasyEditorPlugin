#include "PuertsMultiRootModuleLoader.h"

#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/Paths.h"

namespace
{
bool IsRelativeRequest(const FString& Module)
{
    return Module.StartsWith(TEXT("./")) || Module.StartsWith(TEXT("../")) ||
        Module.StartsWith(TEXT(".\\")) || Module.StartsWith(TEXT("..\\"));
}

bool IsExplicitScriptExtension(const FString& Module)
{
    const FString Extension = FPaths::GetExtension(Module, false);
    return Extension.Equals(TEXT("js"), ESearchCase::IgnoreCase) ||
        Extension.Equals(TEXT("mjs"), ESearchCase::IgnoreCase) ||
        Extension.Equals(TEXT("cjs"), ESearchCase::IgnoreCase) ||
        Extension.Equals(TEXT("json"), ESearchCase::IgnoreCase);
}
} // namespace

FPuertsMultiRootModuleLoader::FPuertsMultiRootModuleLoader(TArray<FPuertsScriptRoot> InRoots)
    : Roots(MoveTemp(InRoots))
{
    for (FPuertsScriptRoot& Root : Roots)
    {
        Root.AbsolutePath = CanonicalizePath(Root.AbsolutePath);
    }
}

bool FPuertsMultiRootModuleLoader::Search(
    const FString& RequiredDir, const FString& RequiredModule, FString& Path, FString& AbsolutePath)
{
    Path.Reset();
    AbsolutePath.Reset();
    LastSearchDiagnostic.Reset();

    bool bFound = false;
    if (IsRelativeRequest(RequiredModule))
    {
        bFound = !RequiredDir.IsEmpty() &&
            SearchFromDirectory(RequiredDir, RequiredModule, false, Path, AbsolutePath);
    }
    else if (!FPaths::IsRelative(RequiredModule))
    {
        bFound = SearchFromDirectory(TEXT(""), RequiredModule, false, Path, AbsolutePath);
    }
    else
    {
        if (!RequiredDir.IsEmpty())
        {
            // Puerts passes package.json "main" values without a leading "./".
            // Resolve those against the package directory before applying bare-module lookup.
            bFound = SearchFromDirectory(RequiredDir, RequiredModule, false, Path, AbsolutePath);
            if (!bFound)
            {
                bFound = SearchNodeModulesUpward(RequiredDir, RequiredModule, Path, AbsolutePath);
            }
        }

        for (int32 RootIndex = 0; !bFound && RootIndex < Roots.Num(); ++RootIndex)
        {
            bFound = SearchFromDirectory(
                Roots[RootIndex].AbsolutePath, RequiredModule, true, Path, AbsolutePath);
        }
    }

    if (!bFound)
    {
        LastSearchDiagnostic = FString::Printf(TEXT("Cannot resolve '%s' from '%s'. Roots: %s"),
            *RequiredModule, *RequiredDir,
            *FString::JoinBy(Roots, TEXT(", "), [](const FPuertsScriptRoot& Root) {
                return Root.Name + TEXT("=") + Root.AbsolutePath;
            }));
    }

    return bFound;
}

bool FPuertsMultiRootModuleLoader::Load(const FString& Path, TArray<uint8>& Content)
{
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    TUniquePtr<IFileHandle> FileHandle(PlatformFile.OpenRead(*Path));
    if (!FileHandle)
    {
        return false;
    }

    const int64 Size = FileHandle->Size();
    if (Size < 0 || Size > MAX_int32)
    {
        return false;
    }

    Content.SetNumUninitialized(static_cast<int32>(Size));
    return Size == 0 || FileHandle->Read(Content.GetData(), Size);
}

FString& FPuertsMultiRootModuleLoader::GetScriptRoot()
{
    return ScriptRoot;
}

FString FPuertsMultiRootModuleLoader::CanonicalizePath(const FString& Path)
{
    FString Result = FPaths::ConvertRelativePathToFull(Path);
    FPaths::NormalizeFilename(Result);
    FPaths::CollapseRelativeDirectories(Result);
    return Result;
}

bool FPuertsMultiRootModuleLoader::SearchFromDirectory(const FString& Directory, const FString& RequiredModule,
    const bool bSearchNodeModules, FString& Path, FString& AbsolutePath) const
{
    const auto CheckInDirectory = [&](const FString& ModuleWithExtension) {
        const FString DirectCandidate = Directory.IsEmpty() ? ModuleWithExtension : Directory / ModuleWithExtension;
        if (CheckCandidate(DirectCandidate, Path, AbsolutePath))
        {
            return true;
        }

        return bSearchNodeModules && !FPaths::GetCleanFilename(Directory).Equals(TEXT("node_modules")) &&
            CheckCandidate(Directory / TEXT("node_modules") / ModuleWithExtension, Path, AbsolutePath);
    };

    if (IsExplicitScriptExtension(RequiredModule))
    {
        return CheckInDirectory(RequiredModule);
    }

    if (CheckInDirectory(RequiredModule + TEXT(".js")) ||
        CheckInDirectory(RequiredModule + TEXT(".mjs")) ||
        CheckInDirectory(RequiredModule + TEXT(".cjs")))
    {
        return true;
    }

#if defined(PUERTS_RUNTIME_WITH_V8_BYTECODE) && PUERTS_RUNTIME_WITH_V8_BYTECODE
    if (CheckInDirectory(RequiredModule + TEXT(".mbc")) ||
        CheckInDirectory(RequiredModule + TEXT(".cbc")))
    {
        return true;
    }
#endif

    return CheckInDirectory(RequiredModule / TEXT("package.json")) ||
        CheckInDirectory(RequiredModule / TEXT("index.js"));
}

bool FPuertsMultiRootModuleLoader::SearchNodeModulesUpward(const FString& RequiredDir,
    const FString& RequiredModule, FString& Path, FString& AbsolutePath) const
{
    FString Directory = CanonicalizePath(RequiredDir);
    while (!Directory.IsEmpty())
    {
        if (SearchFromDirectory(Directory / TEXT("node_modules"), RequiredModule, false, Path, AbsolutePath))
        {
            return true;
        }

        const FString Parent = FPaths::GetPath(Directory);
        if (Parent.IsEmpty() || Parent == Directory)
        {
            break;
        }
        Directory = Parent;
    }

    return false;
}

bool FPuertsMultiRootModuleLoader::CheckCandidate(
    const FString& Candidate, FString& Path, FString& AbsolutePath) const
{
    const FString CanonicalPath = CanonicalizePath(Candidate);
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    if (!PlatformFile.FileExists(*CanonicalPath))
    {
        return false;
    }

    Path = CanonicalPath;
    AbsolutePath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*CanonicalPath);
    FPaths::NormalizeFilename(AbsolutePath);
    return true;
}

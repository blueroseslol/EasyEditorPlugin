#pragma once

#include "PuertsMultiRootModuleLoader.h"

TArray<FPuertsScriptRoot> BuildPuertsEditorScriptRoots();
bool IsPuertsEditorReloadableSource(const FString& Path);

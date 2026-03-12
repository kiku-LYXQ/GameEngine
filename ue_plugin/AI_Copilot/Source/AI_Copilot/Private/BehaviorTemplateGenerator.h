#pragma once

#include "CoreMinimal.h"

struct FBehaviorSpecDefinition
{
    FString Archetype;
    FString ClassName;
    FString Description;
    TMap<FString, FString> ResourceSlots;
    TMap<FString, FString> BehaviorHooks;
    TArray<FString> RequiredComponents;
    TArray<FString> OptionalComponents;
    TMap<FString, FString> Metadata;
};

struct FBehaviorTemplateGenerationResult
{
    FString HeaderPath;
    FString SourcePath;
    FString BindingPlanPath;
    FString BindingPlanSummary;
    TArray<FString> Warnings;
    FString ManifestPath;
    FString SpecId;
};

struct FResourceSlotDescriptor
{
    FString SlotKey;
    FString PropertyName;
    FString TypeName;
    FString ForwardDeclaration;
    FString Description;
    bool bIsClassSlot;
};

class FBehaviorTemplateGenerator
{
public:
    static bool TryParseBehaviorSpec(const TSharedPtr<FJsonObject>& Json, FBehaviorSpecDefinition& OutSpec);
    static bool GenerateSkeleton(const FBehaviorSpecDefinition& Spec, FBehaviorTemplateGenerationResult& OutResult);
    static FString GetOutputDirectory();

private:
    static const TArray<FResourceSlotDescriptor>& GetResourceSlotDescriptors();
    static FString NormalizeClassName(const FString& Candidate);
    static FString DetermineBaseClass(const FString& Archetype);
    static FString BuildHeaderText(const FBehaviorSpecDefinition& Spec, const FString& ClassName, const FString& BaseClass);
    static FString BuildSourceText(const FBehaviorSpecDefinition& Spec, const FString& ClassName, const FString& BaseClass);
    static FString BuildBlueprintBindingPlan(const FBehaviorSpecDefinition& Spec, const FString& ClassName, const FString& BaseClass, const FString& SpecId);
    static FString BuildManifestText(const FBehaviorSpecDefinition& Spec, const FString& ClassName, const FString& BaseClass, const FString& SpecId, const TArray<FString>& GeneratedFiles);
    static FString ResolveSpecId(const FBehaviorSpecDefinition& Spec);
};

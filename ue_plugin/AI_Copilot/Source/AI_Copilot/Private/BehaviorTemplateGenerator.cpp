#include "BehaviorTemplateGenerator.h"

#include "Dom/JsonObject.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Templates/SubclassOf.h"

namespace
{
static TArray<FResourceSlotDescriptor> BuildResourceSlots()
{
    TArray<FBehaviorTemplateGenerator::FResourceSlotDescriptor> Slots;
    Slots.Add({TEXT("StaticMesh"), TEXT("StaticMeshSlot"), TEXT("UStaticMesh*"), TEXT("class UStaticMesh;"), TEXT("用于 Blueprint 绑定的 StaticMesh"), false});
    Slots.Add({TEXT("SkeletalMesh"), TEXT("SkeletalMeshSlot"), TEXT("USkeletalMesh*"), TEXT("class USkeletalMesh;"), TEXT("用于 SkeletalMesh 绑定"), false});
    Slots.Add({TEXT("AnimationMontage"), TEXT("MontageSlot"), TEXT("UAnimMontage*"), TEXT("class UAnimMontage;"), TEXT("AnimationMontage 用于动作切片"), false});
    Slots.Add({TEXT("NiagaraEffect"), TEXT("NiagaraSlot"), TEXT("UNiagaraSystem*"), TEXT("class UNiagaraSystem;"), TEXT("Niagara 特效插槽"), false});
    Slots.Add({TEXT("SoundCue"), TEXT("SoundCueSlot"), TEXT("USoundCue*"), TEXT("class USoundCue;"), TEXT("声音提示插槽"), false});
    Slots.Add({TEXT("Material"), TEXT("MaterialSlot"), TEXT("UMaterialInterface*"), TEXT("class UMaterialInterface;"), TEXT("用于覆盖材质"), false});
    Slots.Add({TEXT("OptionalBlueprintClass"), TEXT("BlueprintClassSlot"), TEXT("TSubclassOf<AActor>"), TEXT("class AActor;"), TEXT("可选 Blueprint 子类"), true});
    return Slots;
}

const TArray<FResourceSlotDescriptor>& ResourceSlotDescriptors()
{
    static TArray<FBehaviorTemplateGenerator::FResourceSlotDescriptor> Slots = BuildResourceSlots();
    return Slots;
}
}

namespace
{
static FString SanitizeIdentifier(const FString& Original)
{
    FString Result;
    Result.Reserve(Original.Len());
    for (TCHAR Ch : Original)
    {
        if (FChar::IsAlnum(Ch) || Ch == TEXT('_'))
        {
            Result.AppendChar(Ch);
        }
    }
    if (Result.IsEmpty())
    {
        Result = TEXT("BehaviorSpec");
    }
    return Result;
}

static FString NormalizeArchetype(const FString& Archetype)
{
    if (Archetype.IsEmpty())
    {
        return TEXT("GenericActor");
    }
    return Archetype;
}
}

bool FBehaviorTemplateGenerator::TryParseBehaviorSpec(const TSharedPtr<FJsonObject>& Json, FBehaviorSpecDefinition& OutSpec)
{
    if (!Json.IsValid())
    {
        return false;
    }

    OutSpec.Archetype = NormalizeArchetype(Json->GetStringField(TEXT("archetype")));
    OutSpec.ClassName = Json->GetStringField(TEXT("class_name"));
    OutSpec.Description = Json->GetStringField(TEXT("description"));

    const TSharedPtr<FJsonObject>* ResourceSlots = nullptr;
    if (Json->TryGetObjectField(TEXT("resource_slots"), ResourceSlots))
    {
        for (const auto& Pair : (*ResourceSlots)->Values)
        {
            if (Pair.Value.IsValid() && Pair.Value->Type == EJson::String)
            {
                OutSpec.ResourceSlots.Add(Pair.Key, Pair.Value->AsString());
            }
        }
    }

    const TSharedPtr<FJsonObject>* BehaviorHooks = nullptr;
    if (Json->TryGetObjectField(TEXT("behavior_hooks"), BehaviorHooks))
    {
        for (const auto& Pair : (*BehaviorHooks)->Values)
        {
            if (Pair.Value.IsValid() && Pair.Value->Type == EJson::String)
            {
                OutSpec.BehaviorHooks.Add(Pair.Key, Pair.Value->AsString());
            }
        }
    }

    const TArray<TSharedPtr<FJsonValue>>* Required = nullptr;
    if (Json->TryGetArrayField(TEXT("required_components"), Required))
    {
        for (const auto& Entry : *Required)
        {
            if (Entry.IsValid() && Entry->Type == EJson::String)
            {
                OutSpec.RequiredComponents.Add(Entry->AsString());
            }
        }
    }

    const TArray<TSharedPtr<FJsonValue>>* Optional = nullptr;
    if (Json->TryGetArrayField(TEXT("optional_components"), Optional))
    {
        for (const auto& Entry : *Optional)
        {
            if (Entry.IsValid() && Entry->Type == EJson::String)
            {
                OutSpec.OptionalComponents.Add(Entry->AsString());
            }
        }
    }

    const TSharedPtr<FJsonObject>* Metadata = nullptr;
    if (Json->TryGetObjectField(TEXT("metadata"), Metadata))
    {
        for (const auto& Pair : (*Metadata)->Values)
        {
            FString ValueString;
            if (Pair.Value.IsValid())
            {
                if (Pair.Value->Type == EJson::String)
                {
                    ValueString = Pair.Value->AsString();
                }
                else if (Pair.Value->Type == EJson::Number)
                {
                    ValueString = FString::SanitizeFloat(Pair.Value->AsNumber());
                }
            }
            OutSpec.Metadata.Add(Pair.Key, ValueString);
        }
    }

    return true;
}

FString FBehaviorTemplateGenerator::GetOutputDirectory()
{
    return FPaths::Combine(FPaths::ProjectDir(), TEXT("GeneratedBehaviorTemplates"));
}

bool FBehaviorTemplateGenerator::GenerateSkeleton(const FBehaviorSpecDefinition& Spec, FBehaviorTemplateGenerationResult& OutResult)
{
    const FString NormalizedClass = NormalizeClassName(Spec.ClassName.IsEmpty()
                                                           ? FString::Printf(TEXT("A%sBehavior"), *SanitizeIdentifier(Spec.Archetype))
                                                           : Spec.ClassName);
    const FString BaseClass = DetermineBaseClass(Spec.Archetype);
    const FString OutputDir = GetOutputDirectory();
    if (!IFileManager::Get().DirectoryExists(*OutputDir))
    {
        IFileManager::Get().MakeDirectory(*OutputDir, true);
    }

    const FString SpecId = ResolveSpecId(Spec);
    const FString HeaderName = NormalizedClass + TEXT(".h");
    const FString SourceName = NormalizedClass + TEXT(".cpp");
    const FString PlanName = NormalizedClass + TEXT("_BindingPlan.txt");
    const FString ManifestName = NormalizedClass + TEXT("_Manifest.txt");
    const FString HeaderPath = FPaths::Combine(OutputDir, HeaderName);
    const FString SourcePath = FPaths::Combine(OutputDir, SourceName);
    const FString PlanPath = FPaths::Combine(OutputDir, PlanName);
    const FString ManifestPath = FPaths::Combine(OutputDir, ManifestName);

    const FString HeaderContent = BuildHeaderText(Spec, NormalizedClass, BaseClass);
    const FString SourceContent = BuildSourceText(Spec, NormalizedClass, BaseClass);
    const FString PlanContent = BuildBlueprintBindingPlan(Spec, NormalizedClass, BaseClass, SpecId);
    const TArray<FString> GeneratedFiles = {HeaderPath, SourcePath, PlanPath, ManifestPath};
    const FString ManifestContent = BuildManifestText(Spec, NormalizedClass, BaseClass, SpecId, GeneratedFiles);

    if (!FFileHelper::SaveStringToFile(HeaderContent, *HeaderPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {
        OutResult.Warnings.Add(TEXT("无法写入 header 文件"));
        return false;
    }
    if (!FFileHelper::SaveStringToFile(SourceContent, *SourcePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {
        OutResult.Warnings.Add(TEXT("无法写入 source 文件"));
        return false;
    }
    if (!FFileHelper::SaveStringToFile(PlanContent, *PlanPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {
        OutResult.Warnings.Add(TEXT("无法写入 binding plan"));
        return false;
    }
    if (!FFileHelper::SaveStringToFile(ManifestContent, *ManifestPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {
        OutResult.Warnings.Add(TEXT("无法写入 manifest 文件"));
        return false;
    }

    OutResult.HeaderPath = HeaderPath;
    OutResult.SourcePath = SourcePath;
    OutResult.BindingPlanPath = PlanPath;
    OutResult.BindingPlanSummary = PlanContent;
    OutResult.ManifestPath = ManifestPath;
    OutResult.SpecId = SpecId;

    return true;
}

const TArray<FBehaviorTemplateGenerator::FResourceSlotDescriptor>& FBehaviorTemplateGenerator::GetResourceSlotDescriptors()
{
    return ResourceSlotDescriptors();
}

FString FBehaviorTemplateGenerator::NormalizeClassName(const FString& Candidate)
{
    FString Result = SanitizeIdentifier(Candidate);
    if (!Result.StartsWith(TEXT("A")))
    {
        Result = TEXT("A") + Result;
    }
    return Result;
}

FString FBehaviorTemplateGenerator::DetermineBaseClass(const FString& Archetype)
{
    if (Archetype.Equals(TEXT("Character"), ESearchCase::IgnoreCase))
    {
        return TEXT("ACharacter");
    }
    return TEXT("AActor");
}

FString FBehaviorTemplateGenerator::BuildHeaderText(const FBehaviorSpecDefinition& Spec, const FString& ClassName, const FString& BaseClass)
{
    FString Header;
    Header += TEXT("#pragma once\n\n");
    Header += TEXT("#include \"CoreMinimal.h\"\n");
    Header += FString::Printf(TEXT("#include \"GameFramework/%s.h\"\n"), *BaseClass);
    Header += TEXT("#include \"Templates/SubclassOf.h\"\n");
    Header += FString::Printf(TEXT("#include \"%s.generated.h\"\n\n"), *ClassName);

    TSet<FString> Forwards;
    for (const auto& Descriptor : GetResourceSlotDescriptors())
    {
        if (!Descriptor.ForwardDeclaration.IsEmpty())
        {
            Forwards.Add(Descriptor.ForwardDeclaration);
        }
    }
    for (const FString& Forward : Forwards)
    {
        Header += Forward + TEXT("\n");
    }
    Header += TEXT("\n");
    Header += TEXT("UCLASS(Blueprintable)\n");
    Header += FString::Printf(TEXT("class NO_API %s : public %s\n"), *ClassName, *BaseClass);
    Header += TEXT("{\n");
    Header += TEXT("    GENERATED_BODY()\n\n");
    Header += TEXT("public:\n");
    Header += FString::Printf(TEXT("    %s();\n\n"), *ClassName);
    Header += TEXT("protected:\n");
    Header += TEXT("    virtual void BeginPlay() override;\n\n");
    Header += TEXT("public:\n");
    Header += TEXT("    virtual void Tick(float DeltaSeconds) override;\n\n");

    for (const auto& Pair : Spec.BehaviorHooks)
    {
        const FString Name = SanitizeIdentifier(Pair.Key);
        if (Name.IsEmpty())
        {
            continue;
        }
        Header += FString::Printf(TEXT("    UFUNCTION(BlueprintCallable, Category=\"Behavior\") void Execute%sHook();\n"), *Name);
    }
    if (Spec.BehaviorHooks.Num())
    {
        Header += TEXT("\n");
    }

    Header += TEXT("    // Resource slots exposed for Blueprint binding\n");
    for (const auto& Descriptor : GetResourceSlotDescriptors())
    {
        const FString* SlotHint = Spec.ResourceSlots.Find(Descriptor.SlotKey);
        FString Hint;
        if (SlotHint && !SlotHint->IsEmpty())
        {
            Hint = FString::Printf(TEXT(" // %s (hint: %s)"), *Descriptor.Description, **SlotHint);
        }
        else
        {
            Hint = FString::Printf(TEXT(" // %s"), *Descriptor.Description);
        }
        Header += FString::Printf(TEXT("    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=\"Resources\") %s %s;%s\n"), *Descriptor.TypeName, *Descriptor.PropertyName, *Hint);
    }

    Header += TEXT("};\n");
    return Header;
}

FString FBehaviorTemplateGenerator::BuildSourceText(const FBehaviorSpecDefinition& Spec, const FString& ClassName, const FString& BaseClass)
{
    FString Source;
    Source += FString::Printf(TEXT("#include \"%s.h\"\n\n"), *ClassName);
    Source += TEXT("#include \"GameFramework/Actor.h\"\n\n");
    Source += FString::Printf(TEXT("%s::%s()\n"), *ClassName, *ClassName);
    Source += TEXT("{\n    PrimaryActorTick.bCanEverTick = true;\n}\n\n");
    Source += FString::Printf(TEXT("void %s::BeginPlay()\n"), *ClassName);
    Source += TEXT("{\n    Super::BeginPlay();\n    // Description: ");
    Source += Spec.Description.IsEmpty() ? TEXT("<no description>\n") : Spec.Description + TEXT("\n");
    Source += TEXT("}\n\n");
    Source += FString::Printf(TEXT("void %s::Tick(float DeltaSeconds)\n"), *ClassName);
    Source += TEXT("{\n    Super::Tick(DeltaSeconds);\n}\n\n");

    for (const auto& Pair : Spec.BehaviorHooks)
    {
        const FString Name = SanitizeIdentifier(Pair.Key);
        if (Name.IsEmpty())
        {
            continue;
        }
        Source += FString::Printf(TEXT("void %s::Execute%sHook()\n"), *ClassName, *Name);
        Source += TEXT("{\n    // TODO: implement hook: ");
        Source += Pair.Value.IsEmpty() ? Pair.Key : Pair.Value;
        Source += TEXT("\n}\n\n");
    }

    return Source;
}

FString FBehaviorTemplateGenerator::BuildBlueprintBindingPlan(const FBehaviorSpecDefinition& Spec, const FString& ClassName, const FString& BaseClass, const FString& SpecId)
{
    FString Plan;
    Plan += TEXT("Blueprint Binding Plan\n");
    Plan += TEXT("----------------------\n");
    Plan += FString::Printf(TEXT("Spec ID: %s\n\n"), *SpecId);
    Plan += TEXT("1. Blueprint Inheritance\n");
    Plan += FString::Printf(TEXT("   • Create Blueprint BP_%s that inherits from %s.\n"), *ClassName, *BaseClass);
    Plan += TEXT("   • Keep the generated C++ skeleton intact; Blueprint overrides should call the Execute<Hook>Hook helpers.\n");
    Plan += TEXT("   • OptionalBlueprintClass slot is reserved for injecting a variant Blueprint or actor subclass when needed.\n\n");
    Plan += TEXT("2. Resource Slot Guidance\n");
    for (const auto& Descriptor : GetResourceSlotDescriptors())
    {
        const FString Hint = Spec.ResourceSlots.FindRef(Descriptor.SlotKey);
        const FString BindingHint = Hint.IsEmpty() ? TEXT("待指定资产/子类") : Hint;
        Plan += FString::Printf(TEXT("   • %s (%s): %s. 绑定提示: %s\n"), *Descriptor.PropertyName, *Descriptor.TypeName, *Descriptor.Description, *BindingHint);
        if (Descriptor.bIsClassSlot)
        {
            Plan += TEXT("     · 该槽允许引用 Blueprint/Actor 子类，用于 runtime 变体或委托实例。\n");
        }
    }
    Plan += TEXT("\n");
    if (Spec.BehaviorHooks.Num())
    {
        Plan += TEXT("3. Behavior Hook Execution Outline\n");
        Plan += TEXT("   · Blueprint 事件图或行为树节点应调用 Execute<Hook>Hook，确保资源槽在执行前就绪。\n");
        Plan += TEXT("   · 资源槽 StaticMeshSlot、SkeletalMeshSlot、AnimationMontage、NiagaraEffect、SoundCue、Material、OptionalBlueprintClass 可在钩子中提供插值素材。\n");
        int32 Index = 0;
        for (const auto& Pair : Spec.BehaviorHooks)
        {
            const FString Name = SanitizeIdentifier(Pair.Key);
            if (Name.IsEmpty())
            {
                continue;
            }
            const FString Description = Pair.Value.IsEmpty() ? Pair.Key : Pair.Value;
            Plan += FString::Printf(TEXT("   %d) %s – %s\n"), ++Index, *Name, *Description);
            Plan += FString::Printf(TEXT("      · 在 Blueprint Graph 或动画通知中调用 Execute%sHook，并通过上面的资源槽设置视觉/音频/特效。\n"), *Name);
        }
        Plan += TEXT("\n");
    }
    if (Spec.RequiredComponents.Num() || Spec.OptionalComponents.Num())
    {
        Plan += TEXT("4. Component Checklist\n");
        if (Spec.RequiredComponents.Num())
        {
            Plan += TEXT("   • Required components:\n");
            for (const FString& Component : Spec.RequiredComponents)
            {
                Plan += FString::Printf(TEXT("     - %s\n"), *Component);
            }
        }
        if (Spec.OptionalComponents.Num())
        {
            Plan += TEXT("   • Optional components:\n");
            for (const FString& Component : Spec.OptionalComponents)
            {
                Plan += FString::Printf(TEXT("     - %s\n"), *Component);
            }
        }
        Plan += TEXT("\n");
    }
    if (Spec.Metadata.Num())
    {
        Plan += TEXT("5. Metadata & Routing\n");
        for (const auto& Pair : Spec.Metadata)
        {
            Plan += FString::Printf(TEXT("   - %s: %s\n"), *Pair.Key, *Pair.Value);
        }
        Plan += TEXT("\n");
    }
    Plan += TEXT("6. Verification Notes\n");
    Plan += TEXT("   • Copilot Panel 应记录 GeneratedBehaviorTemplates 目录下的 skeleton、binding plan、manifest 文件。\n");
    Plan += TEXT("   • 触发 curl http://127.0.0.1:7000/api/copilot/generate 并在日志中确认路径。\n");
    Plan += TEXT("   • 使用 curl http://127.0.0.1:7000/agents/status/health 确保服务可达。\n");
    return Plan;
}



FString FBehaviorTemplateGenerator::BuildManifestText(const FBehaviorSpecDefinition& Spec, const FString& ClassName, const FString& BaseClass, const FString& SpecId, const TArray<FString>& GeneratedFiles)
{
    FString Manifest;
    Manifest += TEXT("Behavior Template Manifest\n");
    Manifest += TEXT("--------------------------\n");
    Manifest += FString::Printf(TEXT("Spec ID: %s\n"), *SpecId);
    Manifest += FString::Printf(TEXT("Class: %s\n"), *ClassName);
    Manifest += FString::Printf(TEXT("Base Class: %s\n"), *BaseClass);
    Manifest += FString::Printf(TEXT("Blueprint: BP_%s\n\n"), *ClassName);

    Manifest += TEXT("Generated Files:\n");
    for (const FString& File : GeneratedFiles)
    {
        Manifest += FString::Printf(TEXT("   - %s\n"), *File);
    }

    Manifest += TEXT("\nResource Slot Reservations:\n");
    for (const auto& Descriptor : GetResourceSlotDescriptors())
    {
        const FString Hint = Spec.ResourceSlots.FindRef(Descriptor.SlotKey);
        const FString BindingHint = Hint.IsEmpty() ? TEXT("待指定") : Hint;
        Manifest += FString::Printf(TEXT("   - %s (%s): %s. 提示: %s\n"), *Descriptor.PropertyName, *Descriptor.TypeName, *Descriptor.Description, *BindingHint);
        if (Descriptor.bIsClassSlot)
        {
            Manifest += TEXT("     · 该槽保留给 Blueprint/Actor 子类，便于注入运行时变体。\n");
        }
    }

    Manifest += TEXT("\nBehavior Hooks Summary:\n");
    if (Spec.BehaviorHooks.Num())
    {
        int32 Index = 0;
        for (const auto& Pair : Spec.BehaviorHooks)
        {
            const FString Description = Pair.Value.IsEmpty() ? Pair.Key : Pair.Value;
            Manifest += FString::Printf(TEXT("   %d) %s – %s\n"), ++Index, *Pair.Key, *Description);
        }
    }
    else
    {
        Manifest += TEXT("   (无行为钩子)\n");
    }

    Manifest += TEXT("\nComponent Checklist:\n");
    if (Spec.RequiredComponents.Num())
    {
        Manifest += TEXT("   • Required components:\n");
        for (const FString& Component : Spec.RequiredComponents)
        {
            Manifest += FString::Printf(TEXT("     - %s\n"), *Component);
        }
    }
    if (Spec.OptionalComponents.Num())
    {
        Manifest += TEXT("   • Optional components:\n");
        for (const FString& Component : Spec.OptionalComponents)
        {
            Manifest += FString::Printf(TEXT("     - %s\n"), *Component);
        }
    }
    if (!Spec.RequiredComponents.Num() && !Spec.OptionalComponents.Num())
    {
        Manifest += TEXT("   (无组件要求)\n");
    }

    if (Spec.Metadata.Num())
    {
        Manifest += TEXT("\nMetadata:\n");
        for (const auto& Pair : Spec.Metadata)
        {
            Manifest += FString::Printf(TEXT("   - %s: %s\n"), *Pair.Key, *Pair.Value);
        }
    }

    Manifest += TEXT("\nVerification Commands:\n");
    Manifest += TEXT("   - curl http://127.0.0.1:7000/api/copilot/generate\n");
    Manifest += TEXT("   - curl http://127.0.0.1:7000/agents/status/health\n");

    Manifest += TEXT("\n");
    return Manifest;
}

FString FBehaviorTemplateGenerator::ResolveSpecId(const FBehaviorSpecDefinition& Spec)
{
    static const TArray<FString> CandidateKeys = {TEXT("spec_id"), TEXT("id"), TEXT("identifier")};
    for (const FString& Key : CandidateKeys)
    {
        if (const FString* Found = Spec.Metadata.Find(Key))
        {
            if (!Found->IsEmpty())
            {
                return *Found;
            }
        }
    }
    if (!Spec.ClassName.IsEmpty())
    {
        return Spec.ClassName;
    }
    if (!Spec.Archetype.IsEmpty())
    {
        return Spec.Archetype;
    }
    return TEXT("behavior_spec");
}

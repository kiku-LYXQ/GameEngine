#pragma once

#include "CoreMinimal.h"
#include "GameFramework/ACharacter.h"
#include "Templates/SubclassOf.h"
#include "ARangerSentinelBehavior.generated.h"

class UStaticMesh;
class USkeletalMesh;
class UAnimMontage;
class UNiagaraSystem;
class USoundCue;
class UMaterialInterface;
class AActor;

UCLASS(Blueprintable)
class NO_API ARangerSentinelBehavior : public ACharacter
{
    GENERATED_BODY()

public:
    ARangerSentinelBehavior();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable, Category="Behavior") void ExecuteOnSpawnHook();
    UFUNCTION(BlueprintCallable, Category="Behavior") void ExecuteOnTargetAcquiredHook();
    UFUNCTION(BlueprintCallable, Category="Behavior") void ExecuteOnRetreatHook();

    // Resource slots exposed for Blueprint binding
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resources") UStaticMesh* StaticMeshSlot; // 用于 Blueprint 绑定的 StaticMesh (hint: /Game/Characters/Ranger/SMesh_RangerBody)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resources") USkeletalMesh* SkeletalMeshSlot; // 用于 SkeletalMesh 绑定 (hint: /Game/Characters/Ranger/SK_Ranger)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resources") UAnimMontage* MontageSlot; // AnimationMontage 用于动作切片 (hint: /Game/Characters/Ranger/Montage_RangerAim)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resources") UNiagaraSystem* NiagaraSlot; // Niagara 特效插槽 (hint: /Game/FX/Ranger/ChargedTrail)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resources") USoundCue* SoundCueSlot; // 声音提示插槽 (hint: /Game/Audio/Ranger/ShotImpact)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resources") UMaterialInterface* MaterialSlot; // 用于覆盖材质 (hint: /Game/Materials/M_RangerArmor)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resources") TSubclassOf<AActor> BlueprintClassSlot; // 可选 Blueprint 子类 (hint: /Game/Blueprints/BP_RangerVariant)
};

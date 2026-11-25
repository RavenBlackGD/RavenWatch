// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractableObjectInterface.h"
#include "PotionPickup.generated.h"

UENUM(BlueprintType)
enum class EPotionEffect : uint8
{
    Health UMETA(DisplayName = "Health"),
    Speed UMETA(DisplayName = "Speed"),
    Mana UMETA(DisplayName = "Mana")
};

class USphereComponent;
class UStaticMeshComponent;
class ABasePlayer;

// Heals the player's health bar. The mesh and amount come from the enemy drop row.
UCLASS()
class RAVENWATCH_API APotionPickup : public AActor, public IInteractableObjectInterface
{
    GENERATED_BODY()

public:
    APotionPickup();

    // Called before BeginPlay so the overlap sees the row's mesh and heal amount.
    void PrepareDrop(UStaticMesh* Mesh, EPotionEffect InEffect, float InHealAmount, float InSpeedMultiplier, float InDuration, float InManaAmount);

    // IInteractableObjectInterface
    virtual void BeginFocus() override;
    virtual void EndFocus() override;
    virtual void BeginInteract() override;
    virtual void EndInteract() override;
    virtual void Interact() override;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Potion")
    USphereComponent* PickupCollision;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Potion")
    UStaticMeshComponent* PickupMesh;

    // Health restored on the bar.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Potion")
    EPotionEffect Effect;

    // Trauma kit. Added to the health bar.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Potion",
        meta = (ClampMin = "0.0", EditCondition = "Effect == EPotionEffect::Health", EditConditionHides))
    float HealAmount;

    // Syringe. 1.5 makes walk 900 and sprint 1350.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Potion",
        meta = (ClampMin = "0.0", EditCondition = "Effect == EPotionEffect::Speed", EditConditionHides))
    float SpeedMultiplier;

    // Arch Syringe length in seconds.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Potion",
        meta = (ClampMin = "0.0", EditCondition = "Effect == EPotionEffect::Speed", EditConditionHides))
    float EffectDuration;

    // Saline flask. Added to the player's mana pool.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Potion",
        meta = (ClampMin = "0.0", EditCondition = "Effect == EPotionEffect::Mana", EditConditionHides))
    float ManaAmount;

    // Overlap size. Matches the gear pickup.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Potion",
        meta = (ClampMin = "1.0"))
    float PickupRadius;

    // Scale of the dropped mesh. Shrink it here if the flask is huge.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Potion")
    FVector MeshScale;

    // Applies this potion to the player. False if the player could not use it.
    bool GivePotionTo(ABasePlayer* Player);
};
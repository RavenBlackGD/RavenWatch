#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "CharacterAnimation.generated.h"

// ReloadNow and ReloadEnded broadcast these.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReloadNotifySignature);

/**
 * Character animation instance.
 * Drives velocity and direction for the blend space from C++.
 */
UCLASS()
class RAVENWATCH_API UCharacterAnimation : public UAnimInstance
{
    GENERATED_BODY()

public:
    UCharacterAnimation();

    virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

    // Plays the fire anim in the ActionSlot
    UFUNCTION(BlueprintCallable, Category = Animation)
    void FireAnimation();

    // Plays the reload anim in the ActionSlot
    UFUNCTION(BlueprintCallable, Category = Animation)
    bool ReloadAnimation();

    // ReloadNow
    UPROPERTY(BlueprintAssignable, Category = Animation)
    FOnReloadNotifySignature OnReloadNow;

    // ReloadEnded
    UPROPERTY(BlueprintAssignable, Category = Animation)
    FOnReloadNotifySignature OnReloadEnded;

protected:
    // Speed for the blend space
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Default)
    float Velocity;

    // True while jumping / falling - stops the walk animation
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Default)
    bool bIsInAir;

    // Facing vs movement direction for the blend space
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Default)
    float Direction;

    // Fire animation asset
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Animation)
    class UAnimSequence* FireAsset;

    // Reload animation asset
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Animation)
    class UAnimSequence* ReloadAsset;

    // Slot name for upper body blend
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Animation)
    FName ActionSlotName;

};
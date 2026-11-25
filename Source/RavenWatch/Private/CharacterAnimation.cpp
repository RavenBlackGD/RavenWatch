#include "CharacterAnimation.h"
#include "KismetAnimationLibrary.h"
#include "Animation/AnimSequence.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UCharacterAnimation::UCharacterAnimation()
    : Velocity(0.0f)
    , Direction(0.0f)
    , bIsInAir(false)
    , FireAsset(nullptr)
    , ReloadAsset(nullptr)
    , ActionSlotName(TEXT("ActionSlot"))
{}

void UCharacterAnimation::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

    APawn* Pawn = TryGetPawnOwner();

    if (Pawn)
    {
        Velocity = Pawn->GetVelocity().Length();

        // Use Kismet Animation Library. AnimInstance has a different CalculateDirection.
        Direction = UKismetAnimationLibrary::CalculateDirection(Pawn->GetVelocity(), Pawn->GetActorRotation());

        bIsInAir = false;
        if (const ACharacter* Character = Cast<ACharacter>(Pawn))
        {
            if (const UCharacterMovementComponent* Move = Character->GetCharacterMovement())
            {
                bIsInAir = Move->IsFalling();
            }
        }
    }
    else
    {
        Velocity = 0.0f;
        bIsInAir = false;
        Direction = 0.0f;
    }
}

// Play fire as a dynamic montage in the ActionSlot so legs keep moving
void UCharacterAnimation::FireAnimation()
{
    if (FireAsset == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("CharacterAnimation: FireAnimation aborted. FireAsset is null. Set it on ABP defaults."));
        return;
    }

    PlaySlotAnimationAsDynamicMontage(FireAsset, ActionSlotName);
}

// Play reload, same as above
bool UCharacterAnimation::ReloadAnimation()
{
    if (ReloadAsset == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("CharacterAnimation: ReloadAnimation aborted. ReloadAsset is null. Set it on ABP defaults."));
        return false;
    }

    UAnimMontage* Montage = PlaySlotAnimationAsDynamicMontage(ReloadAsset, ActionSlotName);
    return Montage != nullptr;
}
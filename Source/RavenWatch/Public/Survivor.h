// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InteractableObjectInterface.h"
#include "AITypes.h"
#include "Navigation/PathFollowingComponent.h"
#include "GenericTeamAgentInterface.h"
#include "Survivor.generated.h"

class UHealthComponent;
class UAnimSequenceBase;
class UAnimMontage;
class ASurvivor;

// Where this survivor is in the rescue
UENUM(BlueprintType)
enum class ESurvivorState : uint8
{
    Trapped     UMETA(DisplayName = "Trapped"),
    StandingUp  UMETA(DisplayName = "Standing Up"),
    Fleeing     UMETA(DisplayName = "Fleeing"),
    Escaped     UMETA(DisplayName = "Escaped"),
    Lost        UMETA(DisplayName = "Lost")
};

// Fires once: true when they leave the map, false when they die
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSurvivorFinished, ASurvivor*, Survivor, bool, bEscaped);

// Fires once when they stand up and start running
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSurvivorFreed, ASurvivor*, Survivor);

UCLASS()
class RAVENWATCH_API ASurvivor : public ACharacter, public IInteractableObjectInterface, public IGenericTeamAgentInterface
{
    GENERATED_BODY()

public:
    // Sets default values for this character's properties
    ASurvivor();

    // Which side this survivor is on. Bullets ignore anything on the shooter's team.
    virtual FGenericTeamId GetGenericTeamId() const override;
    virtual void SetGenericTeamId(const FGenericTeamId& NewTeamId) override;

    // The game mode listens to this in step 2 to count saved and lost
    UPROPERTY(BlueprintAssignable, Category = Survivor)
    FSurvivorFinished OnSurvivorFinished;

    // The game mode listens to this to start an ambush
    UPROPERTY(BlueprintAssignable, Category = Survivor)
    FSurvivorFreed OnSurvivorFreed;

    UFUNCTION(BlueprintPure, Category = Survivor)
    ESurvivorState GetSurvivorState() const { return State; }

    virtual void BeginFocus() override;

    virtual void EndFocus() override;

    virtual void BeginInteract() override;

    virtual void EndInteract() override;

    virtual void Interact() override;

    // Blocks the player's bullets when Player Can Damage is off
    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

    // True until they escape or die
    UFUNCTION(BlueprintPure, Category = Survivor)
    bool IsAlive() const { return State != ESurvivorState::Escaped && State != ESurvivorState::Lost; }

    // Called by the game mode on missions where enemies hunt survivors. Lets them be hurt from the start.
    void SetHuntedByEnemies(bool bHunted);

    // Called by enemies chasing or shooting this survivor, and when they take damage
    void MarkThreatened();

    // True if an enemy went after them in the last Threatened Hold Time seconds
    UFUNCTION(BlueprintPure, Category = Survivor)
    bool IsThreatened() const;

protected:

    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Health)
    UHealthComponent* HealthComponent;

    // Seconds the player holds E. Under 0.1 frees them instantly.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Rescue, meta = (ClampMin = "0.0"))
    float RescueHoldTime;

    // Shown after [E] on the HUD
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Rescue)
    FText RescuePrompt;

    // True means nothing can hurt them. Ambush missions turn this off.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Rescue)
    bool bInvulnerable;

    // Team number. 0 matches the player, so the player's bullets pass through. 255 lets the player hurt them.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Team, meta = (ClampMax = "255"))
    uint8 TeamId;

    // Seconds the HUD keeps showing them in danger after the last threat. Keep above the enemy Think Interval.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Rescue, meta = (ClampMin = "0.1"))
    float ThreatenedHoldTime;

    // Run speed once freed
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Escape, meta = (ClampMin = "0.0"))
    float RunSpeed;

    // Where to run. Leave empty to use the level's nearest actor tagged Escape Point Tag, then the Player Start.
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = Escape)
    AActor* EscapePoint;

    // With no Escape Point set, run to the nearest actor with this tag
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Escape)
    FName EscapePointTag;

    // How close to the escape point counts as out of the map
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Escape, meta = (ClampMin = "0.0"))
    float EscapeRadius;

    // Loops while trapped. Leave empty to use the Anim Blueprint idle.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Animation)
    UAnimSequenceBase* TrappedAnim;

    // Plays once when freed, then they run. Leave empty to run right away.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Animation)
    UAnimSequenceBase* StandUpAnim;

    // Plays once when they die
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Animation)
    UAnimSequenceBase* DeathAnim;

    // Seconds the body stays after death. 0 keeps it forever.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Animation, meta = (ClampMin = "0.0"))
    float CorpseLifeSpan;

    // Slot node in the Anim Blueprint that plays these clips over the whole body
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Animation)
    FName FullBodySlotName;

    // Seconds to blend from the trapped pose into the stand up
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Animation, meta = (ClampMin = "0.0"))
    float StandUpBlendTime;

    // Seconds into the stand up clip to start. Skips a start that does not match the trapped pose.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Animation, meta = (ClampMin = "0.0"))
    float StandUpStartTime;

    // Seconds to blend from the stand up into running
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Animation, meta = (ClampMin = "0.0"))
    float RunBlendTime;

    // Seconds to blend into the death clip
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Animation, meta = (ClampMin = "0.0"))
    float DeathBlendTime;

    // Loops while running to the exit. Each survivor picks one at random. Leave empty to use the Anim Blueprint run.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Animation)
    TArray<UAnimSequenceBase*> FleeAnims;

    // Speeds the flee clip up or down so the feet match Run Speed
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Animation, meta = (ClampMin = "0.1"))
    float FleePlayRate;

    // Seconds before switching to the next flee clip. 0 switches when the clip finishes.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Animation, meta = (ClampMin = "0.0"))
    float FleeSwitchInterval;

    // Seconds to crossfade from one flee clip to the next
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Animation, meta = (ClampMin = "0.0"))
    float FleeSwitchBlendTime;

    // Loops on the whole body while fleeing. When set, Flee Anims only move the head.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Animation)
    UAnimSequenceBase* FleeRunAnim;

    // Speed the run clip moves at, in cm/s. Used to match the feet to Run Speed. 0 plays at normal speed.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Animation, meta = (ClampMin = "0.0"))
    float FleeRunClipSpeed;

    // Slot in the Anim Blueprint that only drives the head and neck
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Animation)
    FName FleeLookSlotName;

private:

    // Current rescue state
    UPROPERTY(VisibleInstanceOnly, Category = Survivor)
    ESurvivorState State;

    // Set by the game mode when enemies on this mission go after survivors
    bool bHuntedByEnemies;

    // World time of the last threat
    float LastThreatenedTime;

    // Waits for the stand up animation before running
    FTimerHandle StandUpTimerHandle;

    // Switches between the flee clips while running
    FTimerHandle FleeSwitchTimerHandle;

    // Which flee clip plays next
    int32 FleeAnimIndex;

    // Plays the next flee clip and schedules the one after it
    void PlayNextFleeAnim();

    // Hands the body back to the Anim Blueprint and runs to the escape point
    void StartFleeing();

    // Counts as saved and leaves the map
    void Escape();

    // EscapePoint if set, otherwise the Player Start
    FVector FindEscapeLocation() const;

    // Bound to the AI controller when the run starts
    UFUNCTION()
    void HandleMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result);

    // Bound to HealthComponent->OnDeath
    UFUNCTION()
    void HandleDeath(float Ratio);

    // Plays a clip on the full body slot (or SlotOverride) with blending. Loops forever or holds the last frame if asked.
    UAnimMontage* PlayFullBody(UAnimSequenceBase* Anim, float BlendIn, float BlendOut, float StartTime, bool bLoop, bool bHoldLastFrame, float PlayRate = 1.0f, FName SlotOverride = NAME_None);
};
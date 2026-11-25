// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "BaseEnemyController.generated.h"

class ABaseEnemy;

// Every ThinkInterval, chase the player or stand and shoot
UCLASS()
class RAVENWATCH_API ABaseEnemyController : public AAIController
{
    GENERATED_BODY()

public:

    // Sets default values
    ABaseEnemyController();

    // Hunt a fleeing survivor, before the player, until it dies or leaves
    void SetForcedTarget(AActor* NewTarget, bool bReturnAfterPlayer = false);


protected:

    // Starts the think timer when we take control of an enemy
    virtual void OnPossess(APawn* InPawn) override;

    // Stops the think timer when we let go
    virtual void OnUnPossess() override;

    // Stops the think timer when removed
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // Where to point the gun when focused on an actor. The shelter uses its nearest built piece.
    virtual FVector GetFocalPointOnActor(const AActor* Actor) const override;

    // Seconds between decisions
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = AI)
    float ThinkInterval;

    // Ignore the player past this distance
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = AI)
    float SightRange;

    // Shoot within this distance if the player is visible
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = AI)
    float AttackRange;

    // How close MoveTo gets before stopping
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = AI)
    float AcceptanceRadius;

    // Half the view cone, in degrees.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = AI,
        meta = (ClampMin = "1.0", ClampMax = "180.0"))
    float SightHalfAngle;

    // Keep a seen player out to this distance.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = AI,
        meta = (ClampMin = "0.0"))
    float LoseSightRange;

    // Seconds to chase the last seen spot after line of sight breaks
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = AI,
        meta = (ClampMin = "0.0"))
    float ForgetSeconds;

    // While hunting a forced target, fight the player instead if they come this close and are visible. 0 never switches.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = AI,
        meta = (ClampMin = "0.0"))
    float ForcedTargetBreakRange;

    // How far away an enemy notices a survivor. 0 uses Sight Range.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = AI,
        meta = (ClampMin = "0.0"))
    float SurvivorSightRange;

private:

    // The enemy we control
    UPROPERTY()
    ABaseEnemy* ControlledEnemy;

    // Timer handle for Think
    FTimerHandle ThinkTimer;

    // Player we have seen. Cleared when the forget time runs out.
    UPROPERTY()
    APawn* TrackedPlayer;

    // World time when the next wander walk may start
    float NextWanderTime;

    // True while a wander walk is in progress
    bool bWanderMoveActive;

    // Walk to a random reachable nav point, then pause.
    void Wander();

    // Where the player was the last time we could see them
    FVector LastSeenLocation;

    // World time when sight broke. Negative means we can currently see them.
    float LostSightTime;

    // Chase, shoot or stop
    void Think();

    // Pawn this enemy was sent to kill. Checked before the player.
    UPROPERTY()
    AActor* ForcedTarget;

    // Target to go back to after the player is lost. Empty for survivor ambushes.
    UPROPERTY()
    AActor* HomeTarget;

    // Chases or shoots ForcedTarget. False when there is none, so Think falls through to the player.
    bool ThinkForcedTarget();

    // Nearest living survivor in range with nothing in the way. Only on missions where enemies hunt survivors.
    AActor* FindVisibleSurvivor() const;

    // Point to shoot at on a target, seen from From
    FVector GetAimPoint(const AActor* Target, const FVector& From) const;

    // True when nothing but the target, or a piece of it, is between our eyes and its aim point
    bool HasClearShot(const AActor* Target) const;

    // True when the player is in range, inside the cone when required, and not behind cover
    bool CanSeePlayer(APawn* Player, float Range, bool bRequireCone) const;
};
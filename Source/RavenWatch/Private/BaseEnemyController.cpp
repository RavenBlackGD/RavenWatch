// Fill out your copyright notice in the Description page of Project Settings.

#include "BaseEnemyController.h"
#include "BaseEnemy.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "TimerManager.h"
#include "NavigationSystem.h"
#include "HealthComponent.h"
#include "Shelter.h"
#include "Survivor.h"
#include "RavenGameMode.h"
#include "Engine/World.h"

// Sets default values
ABaseEnemyController::ABaseEnemyController()
{
    ThinkInterval = 0.25f;
    SightRange = 1500.0f;
    AttackRange = 1200.0f;
    AcceptanceRadius = 100.0f;
    SightHalfAngle = 70.0f;
    LoseSightRange = 1700.0f;
    ForgetSeconds = 2.0f;
    ForcedTargetBreakRange = 500.0f;
    SurvivorSightRange = 0.0f;
    ForcedTarget = nullptr;
    HomeTarget = nullptr;
    ControlledEnemy = nullptr;
    TrackedPlayer = nullptr;
    LastSeenLocation = FVector::ZeroVector;
    NextWanderTime = 0.0f;
    bWanderMoveActive = false;
    LostSightTime = -1.0f;
}

// Store the enemy and start thinking
void ABaseEnemyController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    ControlledEnemy = Cast<ABaseEnemy>(InPawn);

    if (!ControlledEnemy)
    {
        return;
    }

    GetWorldTimerManager().SetTimer(ThinkTimer, this,
        &ABaseEnemyController::Think,
        FMath::Max(ThinkInterval, 0.05f), true);
}

// Stop thinking when we no longer control the enemy
void ABaseEnemyController::OnUnPossess()
{
    GetWorldTimerManager().ClearTimer(ThinkTimer);
    ControlledEnemy = nullptr;
    Super::OnUnPossess();
}

// Safety, stop the timer when removed
void ABaseEnemyController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    GetWorldTimerManager().ClearTimer(ThinkTimer);
    Super::EndPlay(EndPlayReason);
}

// One AI decision, runs every ThinkInterval
void ABaseEnemyController::Think()
{
    if (!IsValid(ControlledEnemy) || ControlledEnemy->IsDead())
    {
        TrackedPlayer = nullptr;
        LostSightTime = -1.0f;
        StopMovement();
        ClearFocus(EAIFocusPriority::Gameplay);
        return;
    }

    // Ambush enemies go after their survivor first
    if (ThinkForcedTarget())
    {
        return;
    }

    // Find the player
    APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
    UWorld* World = GetWorld();
    if (!IsValid(Player) || World == nullptr)
    {
        TrackedPlayer = nullptr;
        LostSightTime = -1.0f;
        StopMovement();
        ClearFocus(EAIFocusPriority::Gameplay);
        return;
    }

    // Player is dead. Drop the target and go back to wandering.
    const UHealthComponent* PlayerHealth = Player->FindComponentByClass<UHealthComponent>();
    if (PlayerHealth && PlayerHealth->GetHealthRatio() <= 0.0f)
    {
        if (IsValid(TrackedPlayer))
        {
            TrackedPlayer = nullptr;
            LostSightTime = -1.0f;
            StopMovement();
        }

        ClearFocus(EAIFocusPriority::Gameplay);
        Wander();
        return;
    }

    const float Distance = FVector::Dist(
        ControlledEnemy->GetActorLocation(), Player->GetActorLocation());
    const float LoseRange = FMath::Max(LoseSightRange, SightRange);
    const bool bHasTarget = IsValid(TrackedPlayer);
    const bool bSeesPlayer = CanSeePlayer(
        Player, bHasTarget ? LoseRange : SightRange, !bHasTarget);

    if (bSeesPlayer)
    {
        ControlledEnemy->SetWandering(false);

        TrackedPlayer = Player;
        LastSeenLocation = Player->GetActorLocation();
        LostSightTime = -1.0f;
        SetFocus(Player);

        if (Distance > AttackRange)
        {
            const EPathFollowingRequestResult::Type Result =
                MoveToActor(Player, AcceptanceRadius);

            if (Result == EPathFollowingRequestResult::Failed)
            {
                UE_LOG(LogTemp, Warning,
                    TEXT("BaseEnemyController: MoveTo failed. Check NavMesh."));
            }
            return;
        }

        StopMovement();
        ControlledEnemy->Attack();
        return;
    }

    if (!bHasTarget)
    {

        // No player to fight. Go after a survivor we can see, if this mission allows it.
        if (AActor* Survivor = FindVisibleSurvivor())
        {
            SetForcedTarget(Survivor);
            ThinkForcedTarget();
            return;
        }

        ClearFocus(EAIFocusPriority::Gameplay);
        Wander();
        return;
    }

    if (LostSightTime < 0.0f)
    {
        LostSightTime = World->GetTimeSeconds();
    }

    const bool bForgot = World->GetTimeSeconds() - LostSightTime > ForgetSeconds;
    if (bForgot || Distance > LoseRange)
    {
        TrackedPlayer = nullptr;
        LostSightTime = -1.0f;
        StopMovement();
        ClearFocus(EAIFocusPriority::Gameplay);
        return;
    }

    // Push the last seen spot, then give up when ForgetSeconds runs out
    SetFocalPoint(LastSeenLocation);
    MoveToLocation(LastSeenLocation, AcceptanceRadius);
}

bool ABaseEnemyController::CanSeePlayer(APawn* Player, float Range, bool bRequireCone) const
{
    if (!IsValid(ControlledEnemy) || !IsValid(Player))
    {
        return false;
    }

    FVector ToPlayer = Player->GetActorLocation() - ControlledEnemy->GetActorLocation();
    if (ToPlayer.Size() > Range)
    {
        return false;
    }

    if (bRequireCone)
    {
        const FVector Forward = ControlledEnemy->GetActorForwardVector();

        if (!ToPlayer.IsNearlyZero() && !Forward.IsNearlyZero())
        {
            const float FacingDot = FVector::DotProduct(
                Forward.GetSafeNormal(), ToPlayer.GetSafeNormal());
            const float Limit = FMath::Cos(FMath::DegreesToRadians(SightHalfAngle));
            if (FacingDot < Limit)
            {
                return false;
            }
        }
    }

    return LineOfSightTo(Player);
}

void ABaseEnemyController::Wander()
{
    UWorld* World = GetWorld();
    if (World == nullptr || !IsValid(ControlledEnemy) || !ControlledEnemy->bCanWander)
    {
        return;
    }

    const float Now = World->GetTimeSeconds();

    // Still walking to the last point
    if (GetMoveStatus() != EPathFollowingStatus::Idle)
    {
        bWanderMoveActive = true;
        return;
    }

    // Just arrived. Start the pause.
    if (bWanderMoveActive)
    {
        bWanderMoveActive = false;
        NextWanderTime = Now + FMath::FRandRange(
            ControlledEnemy->WanderWaitMin,
            FMath::Max(ControlledEnemy->WanderWaitMin, ControlledEnemy->WanderWaitMax));
    }

    if (Now < NextWanderTime)
    {
        return;
    }

    UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(World);
    if (NavSys == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("BaseEnemyController: Navigation system was null. Add a Nav Mesh Bounds Volume."));
        return;
    }

    FNavLocation Point;
    if (NavSys->GetRandomPointInNavigableRadius(
        ControlledEnemy->GetActorLocation(), ControlledEnemy->WanderRadius, Point))
        
    {
        ControlledEnemy->SetWandering(true);
        MoveToLocation(Point.Location, AcceptanceRadius);
    }
}

void ABaseEnemyController::SetForcedTarget(AActor* NewTarget, bool bReturnAfterPlayer)
{
    ForcedTarget = NewTarget;
    HomeTarget = bReturnAfterPlayer ? NewTarget : nullptr;

    if (IsValid(ControlledEnemy))
    {
        ControlledEnemy->SetWandering(false);
    }
}

bool ABaseEnemyController::ThinkForcedTarget()
{
    // Go back to the home target (the shelter) once the player has been lost
    if (!IsValid(ForcedTarget) && IsValid(HomeTarget) && !IsValid(TrackedPlayer))
    {
        ForcedTarget = HomeTarget;
        UE_LOG(LogTemp, Log, TEXT("BaseEnemyController: %s went back to %s."), *GetNameSafe(ControlledEnemy), *GetNameSafe(HomeTarget));
    }

    // Escaped survivors are destroyed, so this also covers "got away"
    if (!IsValid(ForcedTarget))
    {
        ForcedTarget = nullptr;
        return false;
    }

    const UHealthComponent* TargetHealth = ForcedTarget->FindComponentByClass<UHealthComponent>();
    if (TargetHealth && TargetHealth->GetHealthRatio() <= 0.0f)
    {
        // Target died or was destroyed. Go back to the normal player logic and don't return to it.
        if (HomeTarget == ForcedTarget)
        {
            HomeTarget = nullptr;
        }
        ForcedTarget = nullptr;
        StopMovement();
        return false;
    }

    // Turn on the player if they get close.
    APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
    if (IsValid(Player) && ForcedTargetBreakRange > 0.0f
        && CanSeePlayer(Player, ForcedTargetBreakRange, false))
    {
        UE_LOG(LogTemp, Log, TEXT("BaseEnemyController: %s switched from its target to the player."), *GetNameSafe(ControlledEnemy));

        // Track the player now, so the chase doesn't wait for them to be inside the view cone
        TrackedPlayer = Player;
        LastSeenLocation = Player->GetActorLocation();
        LostSightTime = -1.0f;
        ForcedTarget = nullptr;
        return false;
    }

    SetFocus(ForcedTarget);

    // Lets the HUD show this survivor is in danger
    if (ASurvivor* Survivor = Cast<ASurvivor>(ForcedTarget))
    {
        Survivor->MarkThreatened();
    }

    const FVector EnemyLocation = ControlledEnemy->GetActorLocation();
    const float Distance = FVector::Dist(EnemyLocation, GetAimPoint(ForcedTarget, EnemyLocation));

    if (Distance > AttackRange || !HasClearShot(ForcedTarget))
    {
        if (MoveToActor(ForcedTarget, AcceptanceRadius) == EPathFollowingRequestResult::Failed)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("BaseEnemyController: MoveTo forced target failed. Check NavMesh."));
        }
        return true;
    }

    StopMovement();
    ControlledEnemy->Attack();
    return true;
}

FVector ABaseEnemyController::GetFocalPointOnActor(const AActor* Actor) const
{
    // Point the gun at the nearest built part of the shelter, not its empty middle
    if (Actor && Actor->IsA<AShelter>() && IsValid(ControlledEnemy))
    {
        return GetAimPoint(Actor, ControlledEnemy->GetActorLocation());
    }
    return Super::GetFocalPointOnActor(Actor);
}

FVector ABaseEnemyController::GetAimPoint(const AActor* Target, const FVector& From) const
{
    if (const AShelter* Shelter = Cast<AShelter>(Target))
    {
        return Shelter->GetAimPoint(From);
    }
    return Target ? Target->GetActorLocation() : FVector::ZeroVector;
}

bool ABaseEnemyController::HasClearShot(const AActor* Target) const
{
    if (!IsValid(ControlledEnemy) || !IsValid(Target))
    {
        return false;
    }

    // Survivors and other pawns: the engine's check already handles them
    if (Target->IsA<APawn>())
    {
        return LineOfSightTo(Target);
    }

    FVector EyeLocation;
    FRotator EyeRotation;
    ControlledEnemy->GetActorEyesViewPoint(EyeLocation, EyeRotation);

    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(EnemyClearShot), false, ControlledEnemy);
    const bool bBlocked = GetWorld()->LineTraceSingleByChannel(
        Hit, EyeLocation, GetAimPoint(Target, EyeLocation), ECC_Visibility, Params);

    if (!bBlocked)
    {
        return true;
    }

    // Hitting the target, or any built piece of the shelter, is a clear shot
    const AActor* HitActor = Hit.GetActor();
    if (HitActor == Target)
    {
        return true;
    }
    if (const AShelter* Shelter = Cast<AShelter>(Target))
    {
        return Shelter->IsShelterPart(HitActor);
    }
    return false;
}

AActor* ABaseEnemyController::FindVisibleSurvivor() const
{
    const UWorld* World = GetWorld();
    const ARavenGameMode* GameMode = World ? World->GetAuthGameMode<ARavenGameMode>() : nullptr;
    if (GameMode == nullptr || !GameMode->DoEnemiesHuntSurvivors() || !IsValid(ControlledEnemy))
    {
        return nullptr;
    }

    TArray<ASurvivor*> Survivors;
    GameMode->GetLivingSurvivors(Survivors);

    const float Range = SurvivorSightRange > 0.0f ? SurvivorSightRange : SightRange;
    const FVector EnemyLocation = ControlledEnemy->GetActorLocation();
    AActor* Best = nullptr;
    float BestDistSq = FMath::Square(Range);

    for (ASurvivor* Survivor : Survivors)
    {
        const float DistSq = FVector::DistSquared(EnemyLocation, Survivor->GetActorLocation());
        if (DistSq <= BestDistSq && LineOfSightTo(Survivor))
        {
            BestDistSq = DistSq;
            Best = Survivor;
        }
    }

    return Best;
}
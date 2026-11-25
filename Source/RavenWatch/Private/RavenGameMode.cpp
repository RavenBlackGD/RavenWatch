// Fill out your copyright notice in the Description page of Project Settings.

#include "RavenGameMode.h"
#include "BaseEnemy.h"
#include "BaseEnemyController.h"
#include "Components/CapsuleComponent.h"
#include "NavigationSystem.h"
#include "TimerManager.h"
#include "BasePlayer.h"
#include "BaseSpawner.h"
#include "HealthComponent.h"
#include "Survivor.h"
#include "Shelter.h"
#include "ProcLevelGenerator.h"
#include "WBP_EndResults.h"
#include "Blueprint/UserWidget.h"
#include "Components/AudioComponent.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

ARavenGameMode::ARavenGameMode()
{
    EndRule = EMissionEndRule::ClearEnemies;
    MinSurvivorsSaved = 1;
    bEnemiesHuntSurvivors = false;
    TotalSurvivors = 0;
    SurvivorsSaved = 0;
    SurvivorsLost = 0;
    bAmbushSurvivors = false;
    AmbushEnemyClass = nullptr;
    AmbushChance = 1.0f;
    AmbushCountMin = 1;
    AmbushCountMax = 2;
    AmbushDelay = 1.5f;
    AmbushSpawnDistance = 1500.0f;
    ObjectiveEnemiesFormat = NSLOCTEXT("Objective", "Enemies", "Enemies remaining: {Remaining}");
    ObjectiveRescueFormat = NSLOCTEXT("Objective", "Rescue", "Survivors rescued: {Saved} / {Total}");
    ObjectiveLostFormat = NSLOCTEXT("Objective", "Lost", "Lost: {Lost}");
    SurvivorLostAlertFormat = NSLOCTEXT("Objective", "SurvivorLost", "A survivor was killed!   {Left} left to save");
    ObjectiveShelterStartFormat = NSLOCTEXT("Objective", "ShelterStart", "Hold E at the build site to begin the shelter");
    ObjectiveWaveIncomingFormat = NSLOCTEXT("Objective", "WaveIncoming", "Wave {Wave} of {Waves} in {Seconds}s");
    ObjectiveWaveActiveFormat = NSLOCTEXT("Objective", "WaveActive", "Wave {Wave} of {Waves}  |  Enemies: {Remaining}");
    ObjectiveShelterStatusFormat = NSLOCTEXT("Objective", "ShelterStatus", "Shelter {Health}%  |  Built {Built} / {Stages}");
    EnemySpawnTag = TEXT("EnemySpawn");
    SpawnScatterRadius = 300.0f;
    Shelter = nullptr;
    CurrentWaveIndex = -1;
    WaveSpawnsRemaining = 0;
    bWaveActive = false;
    bWavesFinished = false;
}

void ARavenGameMode::BeginPlay()
{
    Super::BeginPlay();

    LivingTargets = 0;
    TotalSurvivors = 0;
    SurvivorsSaved = 0;
    SurvivorsLost = 0;
    bObjectiveWon = false;
    bPlayerLost = false;
    bTrackingObjective = false;
    bMissionStarted = false;

    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("RavenGameMode: World was null."));
        return;
    }

    if (LevelMusic)
    {
        LevelMusicComponent = UGameplayStatics::SpawnSound2D(this, LevelMusic);
    }

    // A generated level starts the mission itself once its chunks and navmesh are ready
    if (TActorIterator<AProcLevelGenerator>(World))
    {
        UE_LOG(LogTemp, Log, TEXT("RavenGameMode: Waiting for the level generator."));
        return;
    }

    StartMission();
}

void ARavenGameMode::StartMission()
{
    if (bMissionStarted)
    {
        return;
    }
    bMissionStarted = true;

    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("RavenGameMode: World was null. Mission not started."));
        return;
    }

    for (TActorIterator<ABaseEnemy> It(World); It; ++It)
    {
        ABaseEnemy* Enemy = *It;
        if (Enemy && !Enemy->IsDead())
        {
            RegisterTarget(Enemy);
        }
    }

    for (TActorIterator<ABaseSpawner> It(World); It; ++It)
    {
        RegisterTarget(*It);
    }

    for (TActorIterator<ASurvivor> It(World); It; ++It)
    {
        RegisterSurvivor(*It);
    }

    for (TActorIterator<ABasePlayer> It(World); It; ++It)
    {
        ABasePlayer* Player = *It;
        if (Player == nullptr)
        {
            continue;
        }

        if (UHealthComponent* Health = Player->FindComponentByClass<UHealthComponent>())
        {
            Health->OnDeath.AddDynamic(this, &ARavenGameMode::HandlePlayerDeath);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("RavenGameMode: Player has no health component. Death will not open the lose menu."));
        }
        break;
    }

    bTrackingObjective = LivingTargets > 0;
    UE_LOG(LogTemp, Log, TEXT("RavenGameMode: Living targets at start: %d. Survivors at start: %d."), LivingTargets, TotalSurvivors);

    if ((EndRule == EMissionEndRule::RescueSurvivors || EndRule == EMissionEndRule::RescueAndClear) && TotalSurvivors == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("RavenGameMode: End rule needs survivors but none are in the level. The level cannot be won."));
    }

    if (MinSurvivorsSaved > TotalSurvivors && TotalSurvivors > 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("RavenGameMode: MinSurvivorsSaved (%d) is more than the survivors in the level (%d). Using %d."), MinSurvivorsSaved, TotalSurvivors, TotalSurvivors);
        MinSurvivorsSaved = TotalSurvivors;
    }

    if (EndRule == EMissionEndRule::DefendShelter)
    {
        StartDefend();
    }

    BroadcastObjective();

}

void ARavenGameMode::SetLevelMusicPaused(bool bPaused)
{
    if (LevelMusicComponent)
    {
        LevelMusicComponent->SetPaused(bPaused);
    }
}

void ARavenGameMode::RegisterTarget(AActor* Target)
{
    if (Target == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("RavenGameMode: RegisterTarget called with a null actor."));
        return;
    }

    if (CountedTargets.Contains(Target))
    {
        return;
    }

    UHealthComponent* Health = Target->FindComponentByClass<UHealthComponent>();
    if (Health == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("RavenGameMode: %s has no health component."), *Target->GetName());
        return;
    }

    CountedTargets.Add(Target);
    LivingTargets++;
    Health->OnDeath.AddDynamic(this, &ARavenGameMode::RemoveTarget);

    // Enemies that arrive later, like ambushes, still have to die for "Clear" rules
    bTrackingObjective = true;

    BroadcastObjective();

}

void ARavenGameMode::TargetDestroyed(AActor* Target)
{
    if (Target != nullptr)
    {
        CountedTargets.Remove(Target);
    }

    RecountTargets();
}

void ARavenGameMode::RemoveTarget(float Ratio)
{
    UE_LOG(LogTemp, Log, TEXT("RavenGameMode: Target down. Health ratio %f."), Ratio);
    RecountTargets();
}

void ARavenGameMode::RecountTargets()
{
    int32 StillAlive = 0;

    for (const TWeakObjectPtr<AActor>& Entry : CountedTargets)
    {
        AActor* Target = Entry.Get();
        if (Target == nullptr)
        {
            continue;
        }

        if (const ABaseEnemy* Enemy = Cast<ABaseEnemy>(Target))
        {
            if (Enemy->IsDead())
            {
                continue;
            }
        }

        const UHealthComponent* Health = Target->FindComponentByClass<UHealthComponent>();
        if (Health == nullptr || Health->GetHealthRatio() <= 0.0f)
        {
            continue;
        }

        StillAlive++;
    }

    LivingTargets = StillAlive;
    UE_LOG(LogTemp, Log, TEXT("RavenGameMode: Living targets %d."), LivingTargets);

    CheckObjective();
}
void ARavenGameMode::RegisterSurvivor(ASurvivor* Survivor)
{
    if (Survivor == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("RavenGameMode: RegisterSurvivor called with a null survivor."));
        return;
    }

    if (CountedSurvivors.Contains(Survivor))
    {
        return;
    }

    CountedSurvivors.Add(Survivor);
    TotalSurvivors++;
    Survivor->OnSurvivorFinished.AddDynamic(this, &ARavenGameMode::HandleSurvivorFinished);
    Survivor->OnSurvivorFreed.AddDynamic(this, &ARavenGameMode::HandleSurvivorFreed);

    if (bEnemiesHuntSurvivors)
    {
        Survivor->SetHuntedByEnemies(true);
    }
}

void ARavenGameMode::GetLivingSurvivors(TArray<ASurvivor*>& OutSurvivors) const
{
    OutSurvivors.Reset();
    for (const TWeakObjectPtr<AActor>& Ptr : CountedSurvivors)
    {
        ASurvivor* Survivor = Cast<ASurvivor>(Ptr.Get());
        if (Survivor && Survivor->IsAlive())
        {
            OutSurvivors.Add(Survivor);
        }
    }
}

void ARavenGameMode::HandleSurvivorFinished(ASurvivor* Survivor, bool bEscaped)
{
    if (bEscaped)
    {
        SurvivorsSaved++;
    }
    else
    {
        SurvivorsLost++;

        FFormatNamedArguments Args;
        Args.Add(TEXT("Lost"), SurvivorsLost);
        Args.Add(TEXT("Left"), FMath::Max(TotalSurvivors - SurvivorsSaved - SurvivorsLost, 0));
        OnMissionAlert.Broadcast(FText::Format(SurvivorLostAlertFormat, Args));
    }

    UE_LOG(LogTemp, Log, TEXT("RavenGameMode: Survivors saved %d, lost %d, of %d."), SurvivorsSaved, SurvivorsLost, TotalSurvivors);

    CheckObjective();
}

void ARavenGameMode::HandleSurvivorFreed(ASurvivor* Survivor)
{
    if (!bAmbushSurvivors || Survivor == nullptr)
    {
        return;
    }

    // Ambush missions let survivors be hurt once they start running
    Survivor->SetCanBeDamaged(true);

    if (FMath::FRand() >= AmbushChance)
    {
        UE_LOG(LogTemp, Log, TEXT("RavenGameMode: %s was not ambushed."), *Survivor->GetName());
        return;
    }

    if (AmbushDelay <= 0.0f)
    {
        SpawnAmbush(Survivor);
        return;
    }

    FTimerHandle AmbushTimer;
    GetWorldTimerManager().SetTimer(AmbushTimer,
        FTimerDelegate::CreateUObject(this, &ARavenGameMode::SpawnAmbush, TWeakObjectPtr<ASurvivor>(Survivor)),
        AmbushDelay, false);
}

void ARavenGameMode::SpawnAmbush(TWeakObjectPtr<ASurvivor> SurvivorPtr)
{
    ASurvivor* Survivor = SurvivorPtr.Get();

    // Already escaped, already dead, or the level is over
    if (Survivor == nullptr || Survivor->GetSurvivorState() != ESurvivorState::Fleeing || bObjectiveWon || bPlayerLost)
    {
        return;
    }

    if (AmbushEnemyClass == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("RavenGameMode: AmbushEnemyClass is none. Set it on the game mode Blueprint."));
        return;
    }

    UWorld* World = GetWorld();
    UNavigationSystemV1* NavSys = World ? UNavigationSystemV1::GetCurrent(World) : nullptr;
    if (NavSys == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("RavenGameMode: No navigation system. Ambush skipped."));
        return;
    }

    // Lift spawns by the enemy's capsule so they stand on the floor
    const ACharacter* EnemyDefaults = AmbushEnemyClass->GetDefaultObject<ACharacter>();
    const float HalfHeight = (EnemyDefaults && EnemyDefaults->GetCapsuleComponent())
        ? EnemyDefaults->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()
        : 0.0f;

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

    const FVector SurvivorLocation = Survivor->GetActorLocation();
    const int32 Count = FMath::RandRange(AmbushCountMin, FMath::Max(AmbushCountMin, AmbushCountMax));
    int32 Spawned = 0;

    for (int32 Index = 0; Index < Count; ++Index)
    {
        // Prefer a point at least half the distance away so they do not pop in on top of the survivor
        FNavLocation Point;
        bool bFound = false;
        for (int32 Try = 0; Try < 5; ++Try)
        {
            if (NavSys->GetRandomReachablePointInRadius(SurvivorLocation, AmbushSpawnDistance, Point))
            {
                bFound = true;
                if (FVector::Dist(Point.Location, SurvivorLocation) >= AmbushSpawnDistance * 0.5f)
                {
                    break;
                }
            }
        }

        if (!bFound)
        {
            continue;
        }

        const FVector Location = Point.Location + FVector(0.0f, 0.0f, HalfHeight);
        const FRotator Facing(0.0f, (SurvivorLocation - Location).Rotation().Yaw, 0.0f);

        ABaseEnemy* Enemy = World->SpawnActor<ABaseEnemy>(AmbushEnemyClass, Location, Facing, Params);
        if (Enemy == nullptr)
        {
            continue;
        }

        if (Enemy->GetController() == nullptr)
        {
            Enemy->SpawnDefaultController();
        }

        ABaseEnemyController* EnemyAI = Cast<ABaseEnemyController>(Enemy->GetController());
        if (EnemyAI == nullptr)
        {
            UE_LOG(LogTemp, Warning, TEXT("RavenGameMode: Ambush enemy has no BaseEnemyController. Removed."));
            Enemy->Destroy();
            continue;
        }

        EnemyAI->SetForcedTarget(Survivor);
        RegisterTarget(Enemy);
        Spawned++;
    }

    UE_LOG(LogTemp, Log, TEXT("RavenGameMode: Ambush of %d on %s."), Spawned, *Survivor->GetName());
}

FText ARavenGameMode::GetObjectiveText() const
{
    FFormatNamedArguments Args;
    Args.Add(TEXT("Remaining"), FMath::Max(LivingTargets, 0));
    Args.Add(TEXT("Saved"), SurvivorsSaved);
    Args.Add(TEXT("Total"), TotalSurvivors);
    Args.Add(TEXT("Lost"), SurvivorsLost);
    Args.Add(TEXT("Needed"), MinSurvivorsSaved);

    const FText Enemies = FText::Format(ObjectiveEnemiesFormat, Args);

    FText Rescue = FText::Format(ObjectiveRescueFormat, Args);
    if (SurvivorsLost > 0)
    {
        Rescue = FText::Format(NSLOCTEXT("Objective", "RescueWithLost", "{0}   {1}"),
            Rescue, FText::Format(ObjectiveLostFormat, Args));
    }

    switch (EndRule)
    {
    case EMissionEndRule::RescueSurvivors:
        return Rescue;

    case EMissionEndRule::RescueAndClear:
        return FText::Format(NSLOCTEXT("Objective", "Both", "{0}\n{1}"), Rescue, Enemies);

    case EMissionEndRule::DefendShelter:
        return GetDefendObjectiveText();

    case EMissionEndRule::ClearEnemies:
    default:
        return Enemies;
    }
}

void ARavenGameMode::BroadcastObjective()
{
    OnObjectiveChanged.Broadcast(GetObjectiveText());
}

void ARavenGameMode::CheckObjective()
{

    BroadcastObjective();

    if (bObjectiveWon || bPlayerLost)
    {
        return;
    }

    // Enemies count as cleared only if the level started with some
    const bool bEnemiesCleared = bTrackingObjective && LivingTargets <= 0;

    // Every survivor has escaped or died
    const bool bSurvivorsFinished = TotalSurvivors > 0 && (SurvivorsSaved + SurvivorsLost) >= TotalSurvivors;
    const bool bEnoughSaved = SurvivorsSaved >= MinSurvivorsSaved;

    bool bWin = false;
    bool bLose = false;

    switch (EndRule)
    {
    case EMissionEndRule::ClearEnemies:
        bWin = bEnemiesCleared;
        break;

    case EMissionEndRule::RescueSurvivors:
        bWin = bSurvivorsFinished && bEnoughSaved;
        bLose = bSurvivorsFinished && !bEnoughSaved;
        break;

    case EMissionEndRule::RescueAndClear:

        // A level with no enemies only needs the rescue
        bWin = bSurvivorsFinished && bEnoughSaved && (bEnemiesCleared || !bTrackingObjective);
        bLose = bSurvivorsFinished && !bEnoughSaved;
        break;

    case EMissionEndRule::DefendShelter:

        // Losing is handled by HandleShelterDestroyed and player death
        bWin = bWavesFinished;
        break;
    }

    if (bLose)
    {
        UE_LOG(LogTemp, Log, TEXT("RavenGameMode: Too many survivors lost. Saved %d, needed %d."), SurvivorsSaved, MinSurvivorsSaved);

        // bPlayerLost also means the mission failed
        bPlayerLost = true;
        ShowLoseScreen();
        return;
    }

    if (bWin)
    {
        bObjectiveWon = true;
        ShowWinScreen();
    }
}

void ARavenGameMode::ShowWinScreen()
{
    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("RavenGameMode: World was null. Win screen was not shown."));
        return;
    }

    APlayerController* PlayerController = World->GetFirstPlayerController();
    if (PlayerController == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("RavenGameMode: Player controller was null. Win screen was not shown."));
        return;
    }

    if (ABasePlayer* Player = Cast<ABasePlayer>(PlayerController->GetPawn()))
    {
        Player->DisableInput(PlayerController);
    }

    PlayerController->bShowMouseCursor = true;

    if (EndResultsClass == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("RavenGameMode: EndResultsClass is none. Set it on BP_RavenGameMode."));
        return;
    }

    UWBP_EndResults* EndResults = CreateWidget<UWBP_EndResults>(PlayerController, EndResultsClass);
    if (EndResults == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("RavenGameMode: CreateWidget failed for EndResultsClass."));
        return;
    }

    EndResults->AddToViewport();
    EndResults->WinConditionMet();

    FInputModeUIOnly InputMode;
    InputMode.SetWidgetToFocus(EndResults->TakeWidget());
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    PlayerController->SetInputMode(InputMode);
    PlayerController->FlushPressedKeys();
}


void ARavenGameMode::HandlePlayerDeath(float Ratio)
{
    if (bPlayerLost || bObjectiveWon)
    {
        return;
    }

    bPlayerLost = true;
    UE_LOG(LogTemp, Log, TEXT("RavenGameMode: Player died. Health ratio %f."), Ratio);
    ShowLoseScreen();
}

void ARavenGameMode::ShowLoseScreen()
{
    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("RavenGameMode: World was null. Lose screen was not shown."));
        return;
    }

    APlayerController* PlayerController = World->GetFirstPlayerController();
    if (PlayerController == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("RavenGameMode: Player controller was null. Lose screen was not shown."));
        return;
    }

    if (ABasePlayer* Player = Cast<ABasePlayer>(PlayerController->GetPawn()))
    {
        Player->DisableInput(PlayerController);
    }

    PlayerController->bShowMouseCursor = true;

    if (EndResultsClass == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("RavenGameMode: EndResultsClass is none. Set it on BP_RavenGameMode."));
        return;
    }

    UWBP_EndResults* EndResults = CreateWidget<UWBP_EndResults>(PlayerController, EndResultsClass);
    if (EndResults == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("RavenGameMode: CreateWidget failed for EndResultsClass."));
        return;
    }

    // Leave the switcher on the lose page. WinConditionMet() is the win page.
    EndResults->AddToViewport();

    FInputModeUIOnly InputMode;
    InputMode.SetWidgetToFocus(EndResults->TakeWidget());
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    PlayerController->SetInputMode(InputMode);
    PlayerController->FlushPressedKeys();
}

void ARavenGameMode::StartDefend()
{
    UWorld* World = GetWorld();
    CurrentWaveIndex = -1;
    WaveSpawnsRemaining = 0;
    bWaveActive = false;
    bWavesFinished = false;
    WaveEnemies.Reset();

    for (TActorIterator<AShelter> It(World); It; ++It)
    {
        Shelter = *It;
        break;
    }
    if (Shelter == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("RavenGameMode: End rule is Establish Shelter but no shelter is in the level."));
        return;
    }
    Shelter->OnShelterDestroyed.AddDynamic(this, &ARavenGameMode::HandleShelterDestroyed);

    // Keep the shelter % on the HUD current
    if (UHealthComponent* ShelterHealth = Shelter->FindComponentByClass<UHealthComponent>())
    {
        ShelterHealth->OnHurt.AddDynamic(this, &ARavenGameMode::HandleShelterHealthChanged);
        ShelterHealth->OnHeal.AddDynamic(this, &ARavenGameMode::HandleShelterHealthChanged);
    }

    SpawnPoints.Reset();
    UGameplayStatics::GetAllActorsWithTag(this, EnemySpawnTag, SpawnPoints);
    if (SpawnPoints.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("RavenGameMode: No actors tagged %s. No waves will spawn."), *EnemySpawnTag.ToString());
        return;
    }

    if (DefendWaves.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("RavenGameMode: DefendWaves is empty. Add waves on the game mode Blueprint."));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("RavenGameMode: Establish Shelter with %d waves and %d spawn points."), DefendWaves.Num(), SpawnPoints.Num());

    if (Shelter->GetBuiltStages() > 0)
    {
        QueueWave(0);
    }
    else
    {
        // Waves start when the player starts building
        Shelter->OnStageBuilt.AddDynamic(this, &ARavenGameMode::HandleShelterStageBuilt);
        UE_LOG(LogTemp, Log, TEXT("RavenGameMode: Waiting for the player to start the shelter."));
    }
}

void ARavenGameMode::QueueWave(int32 WaveIndex)
{
    if (!DefendWaves.IsValidIndex(WaveIndex))
    {
        return;
    }

    CurrentWaveIndex = WaveIndex;
    const float Delay = DefendWaves[WaveIndex].Delay;
    UE_LOG(LogTemp, Log, TEXT("RavenGameMode: Wave %d of %d in %.0f seconds."), WaveIndex + 1, DefendWaves.Num(), Delay);

    if (Delay <= 0.0f)
    {
        BeginWave();
    }
    else
    {
        GetWorldTimerManager().SetTimer(WaveTimerHandle, this, &ARavenGameMode::BeginWave, Delay, false);
        GetWorldTimerManager().SetTimer(CountdownTimerHandle, this, &ARavenGameMode::BroadcastObjective, 1.0f, true);
    }

    BroadcastObjective();
}

void ARavenGameMode::BeginWave()
{
    if (bObjectiveWon || bPlayerLost || !DefendWaves.IsValidIndex(CurrentWaveIndex))
    {
        return;
    }
    GetWorldTimerManager().ClearTimer(CountdownTimerHandle);

    const FDefendWave& Wave = DefendWaves[CurrentWaveIndex];
    WaveSpawnsRemaining = Wave.Count;
    WaveEnemies.Reset();
    bWaveActive = true;
    UE_LOG(LogTemp, Log, TEXT("RavenGameMode: Wave %d started with %d enemies."), CurrentWaveIndex + 1, Wave.Count);

    // First enemy right away, the rest on the timer
    SpawnWaveEnemy();
    if (WaveSpawnsRemaining > 0)
    {
        GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &ARavenGameMode::SpawnWaveEnemy, Wave.SpawnInterval, true);
    }

    BroadcastObjective();
}

void ARavenGameMode::SpawnWaveEnemy()
{
    if (bObjectiveWon || bPlayerLost || WaveSpawnsRemaining <= 0 || !DefendWaves.IsValidIndex(CurrentWaveIndex))
    {
        GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
        CheckWaveCleared();
        return;
    }

    // Counted down even if the spawn fails, so a bad spawn can't stall the mission
    WaveSpawnsRemaining--;
    if (WaveSpawnsRemaining <= 0)
    {
        GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
    }

    const FDefendWave& Wave = DefendWaves[CurrentWaveIndex];
    UWorld* World = GetWorld();
    if (Wave.EnemyClass == nullptr || SpawnPoints.Num() == 0 || World == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("RavenGameMode: Wave %d has no Enemy Class or no spawn points."), CurrentWaveIndex + 1);
        CheckWaveCleared();
        return;
    }

    // Random marker, then a random reachable point near it
    AActor* Marker = SpawnPoints[FMath::RandRange(0, SpawnPoints.Num() - 1)];
    FVector Location = Marker ? Marker->GetActorLocation() : FVector::ZeroVector;
    if (UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(World))
    {
        FNavLocation Point;
        if (NavSys->GetRandomReachablePointInRadius(Location, SpawnScatterRadius, Point))
        {
            Location = Point.Location;
        }
    }

    // Lift by the capsule so the enemy stands on the floor
    const ACharacter* EnemyDefaults = Wave.EnemyClass->GetDefaultObject<ACharacter>();
    if (EnemyDefaults && EnemyDefaults->GetCapsuleComponent())
    {
        Location.Z += EnemyDefaults->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    }

    const FRotator Facing(0.0f, Shelter ? (Shelter->GetActorLocation() - Location).Rotation().Yaw : 0.0f, 0.0f);

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    ABaseEnemy* Enemy = World->SpawnActor<ABaseEnemy>(Wave.EnemyClass, Location, Facing, Params);
    if (Enemy == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("RavenGameMode: Wave enemy failed to spawn."));
        CheckWaveCleared();
        return;
    }

    if (Enemy->GetController() == nullptr)
    {
        Enemy->SpawnDefaultController();
    }

    if (ABaseEnemyController* EnemyAI = Cast<ABaseEnemyController>(Enemy->GetController()))
    {
        // Go after the shelter, and come back to it after fighting the player
        EnemyAI->SetForcedTarget(Shelter, true);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("RavenGameMode: Wave enemy has no BaseEnemyController. It will only fight the player."));
    }

    WaveEnemies.Add(Enemy);
    RegisterTarget(Enemy);

    if (UHealthComponent* Health = Enemy->FindComponentByClass<UHealthComponent>())
    {
        Health->OnDeath.AddDynamic(this, &ARavenGameMode::HandleWaveEnemyDeath);
    }
}

void ARavenGameMode::HandleWaveEnemyDeath(float Ratio)
{
    CheckWaveCleared();
}

void ARavenGameMode::CheckWaveCleared()
{
    if (bObjectiveWon || bPlayerLost || !bWaveActive || WaveSpawnsRemaining > 0 || !DefendWaves.IsValidIndex(CurrentWaveIndex))
    {
        return;
    }

    // Anyone in this wave still standing?
    for (const TWeakObjectPtr<ABaseEnemy>& Entry : WaveEnemies)
    {
        const ABaseEnemy* Enemy = Entry.Get();
        if (Enemy == nullptr || Enemy->IsDead())
        {
            continue;
        }
        const UHealthComponent* Health = Enemy->FindComponentByClass<UHealthComponent>();
        if (Health && Health->GetHealthRatio() > 0.0f)
        {
            return;
        }
    }

    bWaveActive = false;
    const FDefendWave& Wave = DefendWaves[CurrentWaveIndex];
    UE_LOG(LogTemp, Log, TEXT("RavenGameMode: Wave %d of %d cleared."), CurrentWaveIndex + 1, DefendWaves.Num());

    if (Shelter)
    {
        for (int32 i = 0; i < Wave.StagesBuiltWhenCleared; ++i)
        {
            Shelter->BuildNextStage();
        }
    }

    if (DefendWaves.IsValidIndex(CurrentWaveIndex + 1))
    {
        QueueWave(CurrentWaveIndex + 1);
    }
    else
    {
        bWavesFinished = true;
        UE_LOG(LogTemp, Log, TEXT("RavenGameMode: Every wave cleared. Shelter established."));
        CheckObjective();
    }
}

void ARavenGameMode::HandleShelterDestroyed(AShelter* DestroyedShelter)
{
    if (bObjectiveWon || bPlayerLost)
    {
        return;
    }

    bPlayerLost = true;
    GetWorldTimerManager().ClearTimer(WaveTimerHandle);
    GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
    GetWorldTimerManager().ClearTimer(CountdownTimerHandle);

    UE_LOG(LogTemp, Log, TEXT("RavenGameMode: The shelter was destroyed. Mission failed."));
    ShowLoseScreen();
}

void ARavenGameMode::HandleShelterStageBuilt(AShelter* BuiltShelter, int32 StageIndex)
{
    // Only the first build starts the waves. Later stages come from cleared waves.
    if (CurrentWaveIndex < 0 && !bObjectiveWon && !bPlayerLost)
    {
        QueueWave(0);
    }
}

void ARavenGameMode::HandleShelterHealthChanged(float Ratio)
{
    BroadcastObjective();
}

FText ARavenGameMode::GetDefendObjectiveText() const
{
    if (Shelter == nullptr)
    {
        return FText::GetEmpty();
    }

    // Nothing built and no wave queued: tell the player how to start
    if (Shelter->GetBuiltStages() == 0 && CurrentWaveIndex < 0)
    {
        return ObjectiveShelterStartFormat;
    }

    FFormatNamedArguments Args;
    Args.Add(TEXT("Wave"), CurrentWaveIndex + 1);
    Args.Add(TEXT("Waves"), DefendWaves.Num());
    Args.Add(TEXT("Seconds"), GetSecondsToNextWave());
    Args.Add(TEXT("Remaining"), GetDefendEnemiesRemaining());

    const FText WaveLine = FText::Format(bWaveActive ? ObjectiveWaveActiveFormat : ObjectiveWaveIncomingFormat, Args);
    const FText ShelterLine = FText::Format(ObjectiveShelterStatusFormat, Args);
    return FText::Format(NSLOCTEXT("Objective", "DefendLines", "{0}\n{1}"), WaveLine, ShelterLine);
}

int32 ARavenGameMode::GetDefendEnemiesRemaining() const
{
    int32 Remaining = WaveSpawnsRemaining;
    for (const TWeakObjectPtr<ABaseEnemy>& Entry : WaveEnemies)
    {
        const ABaseEnemy* Enemy = Entry.Get();
        if (Enemy == nullptr || Enemy->IsDead())
        {
            continue;
        }
        const UHealthComponent* Health = Enemy->FindComponentByClass<UHealthComponent>();
        if (Health && Health->GetHealthRatio() > 0.0f)
        {
            Remaining++;
        }
    }
    return Remaining;
}

int32 ARavenGameMode::GetSecondsToNextWave() const
{
    const float SecondsLeft = GetWorldTimerManager().GetTimerRemaining(WaveTimerHandle);
    return FMath::Max(FMath::CeilToInt(SecondsLeft), 0);
}

float ARavenGameMode::GetShelterHealthRatio() const
{
    return Shelter ? Shelter->GetHealthRatio() : 0.0f;
}
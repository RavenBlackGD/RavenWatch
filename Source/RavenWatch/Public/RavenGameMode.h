// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RavenGameMode.generated.h"

class UWBP_EndResults;
class USoundBase;
class UAudioComponent;
class ASurvivor;
class ABaseEnemy;
class AShelter;

// What has to be done before the level is won
UENUM(BlueprintType)
enum class EMissionEndRule : uint8
{
    // Kill every enemy and spawner. The original rule.
    ClearEnemies        UMETA(DisplayName = "Clear Enemies"),

    // Ends as soon as every survivor has escaped or died
    RescueSurvivors     UMETA(DisplayName = "Rescue Survivors"),

    // Every survivor rescued and every enemy and spawner dead
    RescueAndClear      UMETA(DisplayName = "Rescue Survivors And Clear Enemies"),

    // Survive every wave in DefendWaves while the shelter is built. Lost if the shelter is destroyed.
    DefendShelter       UMETA(DisplayName = "Establish Shelter")
};

// One wave of the Establish Shelter mission
USTRUCT(BlueprintType)
struct FDefendWave
{
    GENERATED_BODY()

    // Enemy Blueprint for this wave
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Wave)
    TSubclassOf<ABaseEnemy> EnemyClass;

    // Enemies in this wave
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Wave, meta = (ClampMin = "0"))
    int32 Count = 3;

    // Seconds before the wave starts: after the level starts for the first wave, after the last wave is cleared for the rest
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Wave, meta = (ClampMin = "0.0"))
    float Delay = 8.0f;

    // Seconds between each enemy appearing
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Wave, meta = (ClampMin = "0.1"))
    float SpawnInterval = 1.0f;

    // Shelter stages built when this wave is cleared. 0 means a defend-only wave.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Wave, meta = (ClampMin = "0"))
    int32 StagesBuiltWhenCleared = 1;
};

// Sends the current objective line to the HUD
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FObjectiveChanged, const FText&, ObjectiveText);

// A short message for the HUD to flash, like a survivor dying
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMissionAlert, const FText&, AlertText);


UCLASS()
class RAVENWATCH_API ARavenGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:

    ARavenGameMode();

    // Fires whenever an enemy or survivor count changes
    UPROPERTY(BlueprintAssignable, Category = Objective)
    FObjectiveChanged OnObjectiveChanged;

    // Fires when something the player should notice happens
    UPROPERTY(BlueprintAssignable, Category = Objective)
    FMissionAlert OnMissionAlert;

    // The objective line for the current End Rule and counts
    UFUNCTION(BlueprintPure, Category = Objective)
    FText GetObjectiveText() const;

    UFUNCTION(BlueprintPure, Category = Defend)
    bool IsDefendMission() const { return EndRule == EMissionEndRule::DefendShelter && Shelter != nullptr; }

    // 1-based wave number. 0 before the first wave is queued.
    UFUNCTION(BlueprintPure, Category = Defend)
    int32 GetDefendWaveNumber() const { return CurrentWaveIndex + 1; }

    UFUNCTION(BlueprintPure, Category = Defend)
    int32 GetDefendWaveCount() const { return DefendWaves.Num(); }

    // True while a wave is spawning or being fought, false during the countdown
    UFUNCTION(BlueprintPure, Category = Defend)
    bool IsDefendWaveActive() const { return bWaveActive; }

    // This wave's living enemies plus those still to spawn
    UFUNCTION(BlueprintPure, Category = Defend)
    int32 GetDefendEnemiesRemaining() const;

    // Seconds until the next wave starts. 0 during a wave.
    UFUNCTION(BlueprintPure, Category = Defend)
    int32 GetSecondsToNextWave() const;

    // Shelter health, 0 to 1
    UFUNCTION(BlueprintPure, Category = Defend)
    float GetShelterHealthRatio() const;

    // Counts one living enemy or spawner and listens for its death
    void RegisterTarget(AActor* Target);

    // Drops this actor from the living count. Safe to call again for the same actor.
    void TargetDestroyed(AActor* Target);

    // Pauses or resumes the level music. Called by the pause menu.
    void SetLevelMusicPaused(bool bPaused);

    // Counts one survivor and listens for it escaping or dying
    void RegisterSurvivor(ASurvivor* Survivor);

    // True when enemies on this mission go after survivors
    bool DoEnemiesHuntSurvivors() const { return bEnemiesHuntSurvivors; }

    // Survivors still in the level and alive, trapped or running
    void GetLivingSurvivors(TArray<ASurvivor*>& OutSurvivors) const;

    // Counts the level's enemies, spawners, survivors and player, then starts the objective.
    // Runs from BeginPlay, or from the level generator once a generated level is ready.
    UFUNCTION(BlueprintCallable, Category = Objective)
    void StartMission();

protected:

    virtual void BeginPlay() override;

    // One living target died. Shows the win screen when none remain.
    UFUNCTION()
    void RemoveTarget(float Ratio);

    // Player health hit zero. Shows the lose page.
    UFUNCTION()
    void HandlePlayerDeath(float Ratio);

    // Same results widget the win pickup uses
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Objective)
    TSubclassOf<UWBP_EndResults> EndResultsClass;

    // Flashed on the HUD when a survivor dies. {Left} is survivors still alive and not yet saved.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Objective|Text")
    FText SurvivorLostAlertFormat;

    // Living enemies plus living spawners
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Objective)
    int32 LivingTargets;

    // Objective wording. {Remaining} is living enemies plus spawners.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Objective|Text")
    FText ObjectiveEnemiesFormat;

    // {Saved} and {Total} are survivor counts
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Objective|Text")
    FText ObjectiveRescueFormat;

    // Added after the rescue line once anyone dies. {Lost} is the count.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Objective|Text")
    FText ObjectiveLostFormat;

    // Establish Shelter, before the player starts building
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Objective|Text")
    FText ObjectiveShelterStartFormat;

    // Establish Shelter, waiting for a wave. {Wave} {Waves} {Seconds}
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Objective|Text")
    FText ObjectiveWaveIncomingFormat;

    // Establish Shelter, during a wave. {Wave} {Waves} {Remaining}
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Objective|Text")
    FText ObjectiveWaveActiveFormat;

    // Establish Shelter, second line. {Health} is a percent. {Built} {Stages}
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Objective|Text")
    FText ObjectiveShelterStatusFormat;

    // What wins this level. Make a child Blueprint per mission type and pick it in World Settings.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Objective)
    EMissionEndRule EndRule;

    // Fewest survivors that must escape. Fewer than this when all are finished loses the level.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Objective, meta = (ClampMin = "0"))
    int32 MinSurvivorsSaved;

    // Enemies shoot any survivor they can see, trapped or running, and survivors can be hurt from the start
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Objective)
    bool bEnemiesHuntSurvivors;

    // Survivors found at start plus any registered later
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Objective)
    int32 TotalSurvivors;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Objective)
    int32 SurvivorsSaved;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Objective)
    int32 SurvivorsLost;

    // A survivor escaped (true) or died (false)
    UFUNCTION()
    void HandleSurvivorFinished(ASurvivor* Survivor, bool bEscaped);

    // Spawn a few enemies to hunt each survivor after it is freed. Off means survivors run out safely.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Ambush)
    bool bAmbushSurvivors;

    // Enemy that spawns for an ambush
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Ambush, meta = (EditCondition = "bAmbushSurvivors"))
    TSubclassOf<ABaseEnemy> AmbushEnemyClass;

    // Chance each freed survivor gets ambushed. 1 is always.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Ambush, meta = (EditCondition = "bAmbushSurvivors", ClampMin = "0.0", ClampMax = "1.0"))
    float AmbushChance;

    // Fewest enemies per ambush
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Ambush, meta = (EditCondition = "bAmbushSurvivors", ClampMin = "0"))
    int32 AmbushCountMin;

    // Most enemies per ambush
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Ambush, meta = (EditCondition = "bAmbushSurvivors", ClampMin = "0"))
    int32 AmbushCountMax;

    // Seconds after the survivor starts running
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Ambush, meta = (EditCondition = "bAmbushSurvivors", ClampMin = "0.0"))
    float AmbushDelay;

    // How far from the survivor enemies can appear
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Ambush, meta = (EditCondition = "bAmbushSurvivors", ClampMin = "100.0"))
    float AmbushSpawnDistance;

    // A survivor stood up and started running
    UFUNCTION()
    void HandleSurvivorFreed(ASurvivor* Survivor);

    // Establish Shelter waves, fought in order
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Defend)
    TArray<FDefendWave> DefendWaves;

    // Actor Tag on the level markers where wave enemies appear
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Defend)
    FName EnemySpawnTag;

    // Enemies appear at a random reachable point this far from a marker, so a wave doesn't stack up
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Defend, meta = (ClampMin = "0.0"))
    float SpawnScatterRadius;

    // The shelter was destroyed. Shows the lose screen.
    UFUNCTION()
    void HandleShelterDestroyed(AShelter* DestroyedShelter);


    // The first stage was built by the player. Starts the waves.
    UFUNCTION()
    void HandleShelterStageBuilt(AShelter* BuiltShelter, int32 StageIndex);

    // Shelter health went up or down. Refreshes the objective text.
    UFUNCTION()
    void HandleShelterHealthChanged(float Ratio);

    // A wave enemy died. Checks whether the wave is cleared.
    UFUNCTION()
    void HandleWaveEnemyDeath(float Ratio);

    // Music that plays for the whole level. Leave empty to play none.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Audio)
    USoundBase* LevelMusic;

private:

    // True after the win screen has been shown
    bool bObjectiveWon;

    // True after the lose screen has been shown
    bool bPlayerLost;

    // True when this level started with at least one target
    bool bTrackingObjective;

    // True once StartMission has run, so it only runs once
    bool bMissionStarted;

    // Actors already added to LivingTargets
    TSet<TWeakObjectPtr<AActor>> CountedTargets;

    // Survivors already added to TotalSurvivors
    TSet<TWeakObjectPtr<AActor>> CountedSurvivors;

    // Applies EndRule and shows the win or lose screen when it is met
    void CheckObjective();

    // Spawns the ambush on a survivor that is still running
    void SpawnAmbush(TWeakObjectPtr<ASurvivor> SurvivorPtr);

    // Sends GetObjectiveText() to OnObjectiveChanged
    void BroadcastObjective();

    // Playing instance of LevelMusic
    UPROPERTY()
    UAudioComponent* LevelMusicComponent;

    // Counts whoever in the set is still alive, then shows the win screen at zero
    void RecountTargets();

    // Creates the results widget and stops the player
    void ShowWinScreen();

    // Same widget as the lose box, left on the lose page
    void ShowLoseScreen();

    // The shelter found at start, Establish Shelter only
    UPROPERTY()
    AShelter* Shelter;

    // Actors tagged EnemySpawnTag
    UPROPERTY()
    TArray<AActor*> SpawnPoints;

    // Wave being fought or waited on. -1 before the first.
    int32 CurrentWaveIndex;

    // Enemies of the current wave still to appear
    int32 WaveSpawnsRemaining;

    // True from a wave's first spawn until it is cleared
    bool bWaveActive;

    // True once the last wave is cleared
    bool bWavesFinished;

    // Enemies spawned by the current wave
    TArray<TWeakObjectPtr<ABaseEnemy>> WaveEnemies;

    // Waits out a wave's Delay
    FTimerHandle WaveTimerHandle;

    // Spawns a wave's enemies one at a time
    FTimerHandle SpawnTimerHandle;

    // Refreshes the "next wave in" countdown once a second
    FTimerHandle CountdownTimerHandle;

    // The two Establish Shelter objective lines
    FText GetDefendObjectiveText() const;

    // Finds the shelter and spawn markers, then queues the first wave
    void StartDefend();

    // Starts the delay before a wave
    void QueueWave(int32 WaveIndex);

    // Starts spawning the current wave
    void BeginWave();

    // Spawns one enemy of the current wave
    void SpawnWaveEnemy();

    // Builds stages and queues the next wave once every enemy of this one is dead
    void CheckWaveCleared();
};
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LevelChunkCatalog.h"
#include "ProcLevelGenerator.generated.h"

class ULevelStreamingDynamic;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLevelReady);

// Place one in the shell map, on the floor where the start chunk should sit, with the PlayerStart on top
UCLASS()
class RAVENWATCH_API AProcLevelGenerator : public AActor
{
    GENERATED_BODY()

public:
    AProcLevelGenerator();

    // True once every chunk is in and the navmesh is built
    UFUNCTION(BlueprintPure, Category = Generation)
    bool IsLevelReady() const { return bLevelReady; }

    // The seed this run used. Put it in Seed to get the same level again.
    UFUNCTION(BlueprintPure, Category = Generation)
    int32 GetUsedSeed() const { return UsedSeed; }

    // Fires once when the level is ready
    UPROPERTY(BlueprintAssignable, Category = Generation)
    FLevelReady OnLevelReady;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Generation)
    ULevelChunkCatalog* Catalog;

    // 0 picks a new seed every run
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Generation)
    int32 Seed;

    // Cells from start to exit, both included
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Generation, meta = (ClampMin = "2"))
    int32 MainPathLength;

    // Side streets grown off the main path
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Generation, meta = (ClampMin = "0"))
    int32 BranchCount;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Generation, meta = (ClampMin = "1"))
    int32 BranchLengthMin;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Generation, meta = (ClampMin = "1"))
    int32 BranchLengthMax;

    // Chance to join two neighbouring cells that aren't joined yet, making loops
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Generation, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float LoopChance;

    // Fresh layouts to try if one can't be filled with chunks
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Generation, meta = (ClampMin = "1"))
    int32 MaxAttempts;

    // Seconds between navmesh checks after the chunks load
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Generation, meta = (ClampMin = "0.05"))
    float NavCheckInterval;

    // Start the mission anyway if the navmesh isn't done after this many seconds
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Generation, meta = (ClampMin = "1.0"))
    float MaxNavWait;

private:
    struct FCell
    {
        uint8 OpenMask = 0;
        EChunkRole Role = EChunkRole::Normal;
    };

    struct FPlacement
    {
        FIntPoint Cell = FIntPoint::ZeroValue;
        int32 Chunk = INDEX_NONE;
        int32 Turns = 0;
    };

    void Generate();
    bool BuildLayout(FRandomStream& Stream);
    bool GrowPath(FRandomStream& Stream, FIntPoint From, int32 Steps, TArray<FIntPoint>& OutPath);
    void Connect(const FIntPoint& A, const FIntPoint& B);
    bool PickChunks(FRandomStream& Stream, TArray<FPlacement>& OutPlacements) const;
    bool PickChunk(const FCell& Cell, FRandomStream& Stream, int32& OutChunk, int32& OutTurns) const;
    void LoadChunks(const TArray<FPlacement>& Placements);
    void CheckNavigation();

    TMap<FIntPoint, FCell> Cells;

    UPROPERTY()
    TArray<TObjectPtr<ULevelStreamingDynamic>> LoadedChunks;

    FTimerHandle NavTimer;
    float NavWaitStart;
    int32 UsedSeed;
    bool bLevelReady;
};
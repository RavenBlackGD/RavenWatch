// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LevelChunkCatalog.generated.h"

UENUM(BlueprintType)
enum class EChunkRole : uint8
{
    Normal  UMETA(DisplayName = "Normal"),
    Start   UMETA(DisplayName = "Start"),
    Exit    UMETA(DisplayName = "Exit")
};

// One hand-built piece of street. Its map's origin is the cell centre and the floor top sits at Z 0.
USTRUCT(BlueprintType)
struct FLevelChunk
{
    GENERATED_BODY()

    // The chunk's map
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Chunk)
    TSoftObjectPtr<UWorld> Level;

    // Start and Exit chunks are only used for the first and last cell of the main path
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Chunk)
    EChunkRole Role = EChunkRole::Normal;

    // Open sides as built, before any turning. North is +X, East is +Y.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Chunk)
    bool bOpenNorth = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Chunk)
    bool bOpenEast = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Chunk)
    bool bOpenSouth = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Chunk)
    bool bOpenWest = false;

    // Higher is picked more often among the chunks that fit a cell
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Chunk, meta = (ClampMin = "0.0"))
    float Weight = 1.0f;

    // Off keeps this chunk in its built facing only
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Chunk)
    bool bCanRotate = true;

    // Bit 0 North, 1 East, 2 South, 3 West
    uint8 GetOpenMask() const
    {
        return (bOpenNorth ? 1 : 0) | (bOpenEast ? 2 : 0) | (bOpenSouth ? 4 : 0) | (bOpenWest ? 8 : 0);
    }
};

UCLASS(BlueprintType)
class RAVENWATCH_API ULevelChunkCatalog : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    // Width of one square cell in cm. Every chunk is built to exactly this size.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Chunks, meta = (ClampMin = "100.0"))
    float CellSize = 4000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Chunks)
    TArray<FLevelChunk> Chunks;
};
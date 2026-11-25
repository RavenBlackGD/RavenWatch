// Fill out your copyright notice in the Description page of Project Settings.


#include "ProcLevelGenerator.h"
#include "RavenGameMode.h"
#include "Components/SceneComponent.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "TimerManager.h"

namespace
{
    // North, East, South, West. Bit i of an open mask means side i is open.
    const FIntPoint Directions[4] = { FIntPoint(1, 0), FIntPoint(0, 1), FIntPoint(-1, 0), FIntPoint(0, -1) };

    // Turning a chunk 90 degrees moves each open side one step clockwise
    uint8 RotateMask(uint8 Mask, int32 Turns)
    {
        Turns %= 4;
        return static_cast<uint8>(((Mask << Turns) | (Mask >> (4 - Turns))) & 0xF);
    }

    // Fixed order so the same seed always gives the same level
    void SortCells(TArray<FIntPoint>& Cells)
    {
        Cells.Sort([](const FIntPoint& A, const FIntPoint& B)
            {
                return A.X < B.X || (A.X == B.X && A.Y < B.Y);
            });
    }
}

AProcLevelGenerator::AProcLevelGenerator()
{
    PrimaryActorTick.bCanEverTick = false;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

    Catalog = nullptr;
    Seed = 0;
    MainPathLength = 6;
    BranchCount = 2;
    BranchLengthMin = 1;
    BranchLengthMax = 2;
    LoopChance = 0.0f;
    MaxAttempts = 20;
    NavCheckInterval = 0.25f;
    MaxNavWait = 15.0f;
    NavWaitStart = 0.0f;
    UsedSeed = 0;
    bLevelReady = false;
}

void AProcLevelGenerator::BeginPlay()
{
    Super::BeginPlay();

    // Wait one frame so the chunks' actors start play like any other level's
    GetWorldTimerManager().SetTimerForNextTick(this, &AProcLevelGenerator::Generate);
}

void AProcLevelGenerator::Generate()
{
    if (Catalog == nullptr || Catalog->Chunks.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("ProcLevelGenerator: No chunk catalog set. Nothing will load."));
        return;
    }

    UsedSeed = Seed != 0 ? Seed : FMath::RandRange(1, MAX_int32 - 1);
    FRandomStream Stream(UsedSeed);

    TArray<FPlacement> Placements;
    bool bBuilt = false;
    for (int32 Attempt = 0; Attempt < MaxAttempts && !bBuilt; ++Attempt)
    {
        bBuilt = BuildLayout(Stream) && PickChunks(Stream, Placements);
    }

    if (!bBuilt)
    {
        UE_LOG(LogTemp, Error, TEXT("ProcLevelGenerator: Seed %d gave no layout the catalog can fill in %d tries."), UsedSeed, MaxAttempts);
        return;
    }

    LoadChunks(Placements);
    UE_LOG(LogTemp, Log, TEXT("ProcLevelGenerator: Seed %d, %d chunks loaded."), UsedSeed, LoadedChunks.Num());

    NavWaitStart = GetWorld()->GetTimeSeconds();
    GetWorldTimerManager().SetTimer(NavTimer, this, &AProcLevelGenerator::CheckNavigation, NavCheckInterval, true);
}

bool AProcLevelGenerator::BuildLayout(FRandomStream& Stream)
{
    Cells.Reset();

    const FIntPoint StartCell(0, 0);
    Cells.Add(StartCell);

    // Main path from start to exit
    TArray<FIntPoint> MainPath;
    if (!GrowPath(Stream, StartCell, MainPathLength - 1, MainPath))
    {
        return false;
    }
    Cells.FindChecked(StartCell).Role = EChunkRole::Start;
    Cells.FindChecked(MainPath.Last()).Role = EChunkRole::Exit;

    // Side streets off the middle of the main path, never the start or exit, so both stay dead ends
    const int32 LastMiddle = MainPath.Num() - 2;
    for (int32 Branch = 0; Branch < BranchCount && LastMiddle >= 0; ++Branch)
    {
        const FIntPoint Root = MainPath[Stream.RandRange(0, LastMiddle)];
        const int32 Length = Stream.RandRange(BranchLengthMin, FMath::Max(BranchLengthMin, BranchLengthMax));

        // A boxed-in branch just stops short
        TArray<FIntPoint> BranchCells;
        GrowPath(Stream, Root, Length, BranchCells);
    }

    // Join some neighbouring cells that aren't joined yet, so there are loops
    if (LoopChance > 0.0f)
    {
        TArray<FIntPoint> Keys;
        Cells.GetKeys(Keys);
        SortCells(Keys);

        for (const FIntPoint& Cell : Keys)
        {
            // North and East only, so each pair is checked once
            for (int32 Dir = 0; Dir < 2; ++Dir)
            {
                const FIntPoint Other = Cell + Directions[Dir];
                const FCell* Mine = Cells.Find(Cell);
                const FCell* Theirs = Cells.Find(Other);

                if (Theirs == nullptr || (Mine->OpenMask & (1 << Dir)) != 0)
                {
                    continue;
                }
                if (Mine->Role != EChunkRole::Normal || Theirs->Role != EChunkRole::Normal)
                {
                    continue;
                }
                if (Stream.FRand() < LoopChance)
                {
                    Connect(Cell, Other);
                }
            }
        }
    }

    return true;
}

bool AProcLevelGenerator::GrowPath(FRandomStream& Stream, FIntPoint From, int32 Steps, TArray<FIntPoint>& OutPath)
{
    OutPath.Reset();
    FIntPoint Current = From;

    for (int32 Step = 0; Step < Steps; ++Step)
    {
        // Neighbours nothing has used yet
        TArray<FIntPoint, TInlineAllocator<4>> Free;
        for (const FIntPoint& Dir : Directions)
        {
            if (!Cells.Contains(Current + Dir))
            {
                Free.Add(Current + Dir);
            }
        }

        if (Free.Num() == 0)
        {
            return false;
        }

        const FIntPoint Next = Free[Stream.RandRange(0, Free.Num() - 1)];
        Cells.Add(Next);
        Connect(Current, Next);
        OutPath.Add(Next);
        Current = Next;
    }

    return true;
}

void AProcLevelGenerator::Connect(const FIntPoint& A, const FIntPoint& B)
{
    for (int32 Dir = 0; Dir < 4; ++Dir)
    {
        if (B - A == Directions[Dir])
        {
            Cells.FindChecked(A).OpenMask |= 1 << Dir;
            Cells.FindChecked(B).OpenMask |= 1 << ((Dir + 2) % 4);
            return;
        }
    }
}

bool AProcLevelGenerator::PickChunks(FRandomStream& Stream, TArray<FPlacement>& OutPlacements) const
{
    OutPlacements.Reset();

    TArray<FIntPoint> Keys;
    Cells.GetKeys(Keys);
    SortCells(Keys);

    for (const FIntPoint& Key : Keys)
    {
        const FCell& Cell = Cells.FindChecked(Key);

        FPlacement Placement;
        Placement.Cell = Key;
        if (!PickChunk(Cell, Stream, Placement.Chunk, Placement.Turns))
        {
            UE_LOG(LogTemp, Warning, TEXT("ProcLevelGenerator: No %s chunk fits open sides N%d E%d S%d W%d. Trying another layout."),
                *UEnum::GetValueAsString(Cell.Role),
                (Cell.OpenMask & 1) ? 1 : 0, (Cell.OpenMask & 2) ? 1 : 0, (Cell.OpenMask & 4) ? 1 : 0, (Cell.OpenMask & 8) ? 1 : 0);
            return false;
        }
        OutPlacements.Add(Placement);
    }

    return true;
}

bool AProcLevelGenerator::PickChunk(const FCell& Cell, FRandomStream& Stream, int32& OutChunk, int32& OutTurns) const
{
    struct FOption
    {
        int32 Chunk;
        int32 Turns;
        float Weight;
    };

    TArray<FOption> Options;
    float TotalWeight = 0.0f;

    for (int32 Index = 0; Index < Catalog->Chunks.Num(); ++Index)
    {
        const FLevelChunk& Chunk = Catalog->Chunks[Index];
        if (Chunk.Role != Cell.Role || Chunk.Weight <= 0.0f || Chunk.Level.IsNull())
        {
            continue;
        }

        // Every turn that lines the chunk's open sides up with the cell's
        TArray<int32, TInlineAllocator<4>> Fits;
        const int32 TurnsAllowed = Chunk.bCanRotate ? 4 : 1;
        for (int32 Turns = 0; Turns < TurnsAllowed; ++Turns)
        {
            if (RotateMask(Chunk.GetOpenMask(), Turns) == Cell.OpenMask)
            {
                Fits.Add(Turns);
            }
        }

        // Split the weight so a symmetric chunk isn't picked more just because more turns fit
        for (int32 Turns : Fits)
        {
            const float Weight = Chunk.Weight / Fits.Num();
            Options.Add({ Index, Turns, Weight });
            TotalWeight += Weight;
        }
    }

    if (Options.Num() == 0)
    {
        return false;
    }

    float Roll = Stream.FRandRange(0.0f, TotalWeight);
    for (const FOption& Option : Options)
    {
        Roll -= Option.Weight;
        if (Roll <= 0.0f)
        {
            OutChunk = Option.Chunk;
            OutTurns = Option.Turns;
            return true;
        }
    }

    OutChunk = Options.Last().Chunk;
    OutTurns = Options.Last().Turns;
    return true;
}

void AProcLevelGenerator::LoadChunks(const TArray<FPlacement>& Placements)
{
    const FVector Origin = GetActorLocation();
    const float CellSize = Catalog->CellSize;

    for (const FPlacement& Placement : Placements)
    {
        const FLevelChunk& Chunk = Catalog->Chunks[Placement.Chunk];
        const FVector Location = Origin + FVector(Placement.Cell.X * CellSize, Placement.Cell.Y * CellSize, 0.0f);
        const FRotator Rotation(0.0f, Placement.Turns * 90.0f, 0.0f);

        bool bLoaded = false;
        ULevelStreamingDynamic* Streaming = ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr(this, Chunk.Level, Location, Rotation, bLoaded);
        if (bLoaded && Streaming)
        {
            LoadedChunks.Add(Streaming);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("ProcLevelGenerator: Could not load %s."), *Chunk.Level.ToString());
        }
    }

    // Load them all now so the player has ground under them straight away
    UGameplayStatics::FlushLevelStreaming(this);
}

void AProcLevelGenerator::CheckNavigation()
{
    const float Waited = GetWorld()->GetTimeSeconds() - NavWaitStart;

    if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
    {
        const bool bBuilding = NavSys->IsNavigationBuildInProgress();
        const bool bDirty = NavSys->HasDirtyAreasQueued();

        if (bBuilding || bDirty)
        {
            UE_LOG(LogTemp, Log, TEXT("ProcLevelGenerator: Nav wait %.1fs. Building %d, Dirty %d, Tasks remaining %d, running %d."),
                Waited, bBuilding ? 1 : 0, bDirty ? 1 : 0,
                NavSys->GetNumRemainingBuildTasks(), NavSys->GetNumRunningBuildTasks());

            if (Waited < MaxNavWait)
            {
                return;
            }
            UE_LOG(LogTemp, Warning, TEXT("ProcLevelGenerator: Navmesh still not done after %.1fs. Starting anyway."), Waited);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("ProcLevelGenerator: No navigation system. Starting without a navmesh."));
    }

    GetWorldTimerManager().ClearTimer(NavTimer);
    bLevelReady = true;
    UE_LOG(LogTemp, Log, TEXT("ProcLevelGenerator: Navmesh built. Level ready."));

    if (ARavenGameMode* GameMode = GetWorld()->GetAuthGameMode<ARavenGameMode>())
    {
        GameMode->StartMission();
    }

    OnLevelReady.Broadcast();
}
// Fill out your copyright notice in the Description page of Project Settings.

#include "Shelter.h"
#include "HealthComponent.h"
#include "Components/SceneComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

// Sets default values
AShelter::AShelter()
{
    // Health growth runs on a timer, not Tick
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));

    // Invisible box over the build site. Only the player's interact trace ,Visibility, sees it.
    InteractVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractVolume"));
    InteractVolume->SetupAttachment(SceneRoot);
    InteractVolume->SetBoxExtent(FVector(500.0f, 650.0f, 150.0f));
    InteractVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    InteractVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
    InteractVolume->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    InteractVolume->SetGenerateOverlapEvents(false);

    StartHoldTime = 2.0f;
    StartPrompt = NSLOCTEXT("Shelter", "StartPrompt", "Establish Shelter");
    bPreviewShown = false;
    PromptText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("PromptText"));
    PromptText->SetupAttachment(SceneRoot);
    PromptText->SetHorizontalAlignment(EHTA_Center);
    PromptText->SetVerticalAlignment(EVRTA_TextBottom);
    PromptText->SetWorldSize(60.0f);
    PromptText->SetTextRenderColor(FColor::White);
    PromptText->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PromptText->SetCastShadow(false);

    // Mesh is chosen on BP_Shelter
    PromptArrow = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PromptArrow"));
    PromptArrow->SetupAttachment(SceneRoot);
    PromptArrow->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PromptArrow->SetCastShadow(false);

    PromptMessage = NSLOCTEXT("Shelter", "PromptMessage", "Hold - E - to begin\nShelter Construction");
    PromptHeight = 450.0f;
    PromptArrowGap = 90.0f;
    PromptBobHeight = 20.0f;
    PromptBobSpeed = 1.0f;
    PromptTime = 0.0f;
    StartingStages = 1;
    HealthGrowthPerSecond = 20.0f;
    HealthGrowthInterval = 0.5f;
    bPlayerCanDamage = false;
    BuiltStages = 0;
    CurrentMaxHealth = 1.0f;
    bDestroyed = false;

    SetCanBeDamaged(true);
}

// Called when the game starts or when spawned
void AShelter::BeginPlay()
{
    Super::BeginPlay();

    if (Stages.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("Shelter: %s has no stages. Add them in the Details panel."), *GetName());
    }

    // Find each stage's pieces by tag, and make them pass bullet damage to the shelter
    StagePieces.SetNum(Stages.Num());
    for (int32 i = 0; i < Stages.Num(); ++i)
    {
        TArray<AActor*> Found;
        UGameplayStatics::GetAllActorsWithTag(this, Stages[i].PieceTag, Found);

        if (Found.Num() == 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("Shelter: no actors tagged %s for stage %d."), *Stages[i].PieceTag.ToString(), i + 1);
        }

        for (AActor* Piece : Found)
        {
            // Static meshes ignore bullets by default. Turn on what the projectile needs.
            Piece->SetCanBeDamaged(true);
            TInlineComponentArray<UPrimitiveComponent*> Prims(Piece);
            for (UPrimitiveComponent* Prim : Prims)
            {
                Prim->SetGenerateOverlapEvents(true);
            }

            Piece->OnTakeAnyDamage.AddUniqueDynamic(this, &AShelter::HandlePieceDamaged);
            StagePieces[i].Add(Piece);
        }
    }

    // Show the starting stages, hide the rest
    BuiltStages = FMath::Clamp(StartingStages, 0, Stages.Num());
    for (int32 i = 0; i < Stages.Num(); ++i)
    {
        SetStageVisible(i, i < BuiltStages);
    }

    // Start at full health for what is already standing
    CurrentMaxHealth = BuiltStages > 0 ? Stages[BuiltStages - 1].MaxHealth : 1.0f;
    if (HealthComponent)
    {
        HealthComponent->InitializeHealth(CurrentMaxHealth);
        HealthComponent->OnDeath.AddDynamic(this, &AShelter::HandleDeath);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Shelter: %s has no health component."), *GetName());
    }

    UE_LOG(LogTemp, Log, TEXT("Shelter: %d of %d stages built at start. Health %.0f."), BuiltStages, Stages.Num(), CurrentMaxHealth);

    // The interface data is not editable in the Details panel, so fill it from our settings
    InteractionData.InteractionName = StartPrompt;
    InteractionData.InteractionDuration = StartHoldTime;

    // Already started (Starting Stages above 0): nothing to interact with
    if (BuiltStages > 0 && InteractVolume)
    {
        InteractVolume->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
    }

    // Floating prompt over the site, only while building hasn't started
    if (PromptText)
    {
        PromptText->SetText(PromptMessage);
        PromptText->SetRelativeLocation(FVector(0.0f, 0.0f, PromptHeight));
    }
    if (PromptArrow)
    {
        PromptArrow->SetRelativeLocation(FVector(0.0f, 0.0f, PromptHeight - PromptArrowGap));
    }
    SetStartPromptVisible(BuiltStages == 0);
}

bool AShelter::BuildNextStage()
{
    if (bDestroyed || IsComplete())
    {
        return false;
    }

    const int32 StageIndex = BuiltStages;
    SetStageVisible(StageIndex, true);
    ++BuiltStages;

    // Raise the ceiling now; current health climbs to it over time
    CurrentMaxHealth = Stages[StageIndex].MaxHealth;
    if (HealthComponent)
    {
        if (StageIndex == 0)
        {
            // First stage of an empty site comes up at full health
            HealthComponent->InitializeHealth(CurrentMaxHealth);
        }
        else
        {
            HealthComponent->SetMaxHealth(CurrentMaxHealth);
        }
    }

    if (HealthGrowthPerSecond > 0.0f)
    {
        GetWorldTimerManager().SetTimer(GrowthTimerHandle, this, &AShelter::GrowHealth, HealthGrowthInterval, true);
    }

    UE_LOG(LogTemp, Log, TEXT("Shelter: stage %d of %d built. Max health %.0f."), BuiltStages, Stages.Num(), CurrentMaxHealth);
    OnStageBuilt.Broadcast(this, StageIndex);
    return true;
}

float AShelter::GetHealthRatio() const
{
    return HealthComponent ? HealthComponent->GetHealthRatio() : 0.0f;
}

bool AShelter::IsShelterPart(const AActor* Actor) const
{
    if (Actor == nullptr)
    {
        return false;
    }
    if (Actor == this)
    {
        return true;
    }
    for (int32 i = 0; i < BuiltStages && i < StagePieces.Num(); ++i)
    {
        for (const TWeakObjectPtr<AActor>& Piece : StagePieces[i])
        {
            if (Piece.Get() == Actor)
            {
                return true;
            }
        }
    }
    return false;
}

FVector AShelter::GetAimPoint(const FVector& From) const
{
    FVector Best = GetActorLocation();
    float BestDistSq = TNumericLimits<float>::Max();

    for (int32 i = 0; i < BuiltStages && i < StagePieces.Num(); ++i)
    {
        for (const TWeakObjectPtr<AActor>& Piece : StagePieces[i])
        {
            if (const AActor* Actor = Piece.Get())
            {
                FVector Origin, Extent;
                Actor->GetActorBounds(true, Origin, Extent);

                const float DistSq = FVector::DistSquared(From, Origin);
                if (DistSq < BestDistSq)
                {
                    BestDistSq = DistSq;
                    Best = Origin;
                }
            }
        }
    }
    return Best;
}

void AShelter::SetStageVisible(int32 StageIndex, bool bVisible)
{
    if (!StagePieces.IsValidIndex(StageIndex))
    {
        return;
    }
    for (const TWeakObjectPtr<AActor>& Piece : StagePieces[StageIndex])
    {
        if (AActor* Actor = Piece.Get())
        {
            Actor->SetActorHiddenInGame(!bVisible);
            Actor->SetActorEnableCollision(bVisible);
        }
    }
}

void AShelter::GrowHealth()
{
    if (bDestroyed || HealthComponent == nullptr || GetHealthRatio() >= 1.0f)
    {
        GetWorldTimerManager().ClearTimer(GrowthTimerHandle);
        return;
    }

    // Negative damage heals. HealthComponent clamps at max and fires OnHeal.
    const float Amount = HealthGrowthPerSecond * HealthGrowthInterval;
    UGameplayStatics::ApplyDamage(this, -Amount, nullptr, this, nullptr);
}

void AShelter::HandlePieceDamaged(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
    if (bDestroyed || Damage <= 0.0f)
    {
        return;
    }

    // The player's own shots do nothing unless allowed
    if (!bPlayerCanDamage && InstigatedBy && InstigatedBy->IsPlayerController())
    {
        return;
    }

    UGameplayStatics::ApplyDamage(this, Damage, InstigatedBy, DamageCauser, nullptr);
}

void AShelter::HandleDeath(float Ratio)
{
    if (bDestroyed)
    {
        return;
    }

    bDestroyed = true;
    GetWorldTimerManager().ClearTimer(GrowthTimerHandle);
    UE_LOG(LogTemp, Log, TEXT("Shelter: %s destroyed."), *GetName());

    OnShelterDestroyed.Broadcast(this);
}

void AShelter::BeginFocus()
{
    // Only an unstarted site shows the outline
    if (BuiltStages == 0 && !bDestroyed)
    {
        SetPreviewOutline(true);
    }
}

void AShelter::EndFocus()
{
    SetPreviewOutline(false);
}

void AShelter::BeginInteract()
{
    UE_LOG(LogTemp, Log, TEXT("Shelter: start held."));
}

void AShelter::EndInteract()
{
    // The player let go of E. BasePlayer already cancels the hold timer.
}

void AShelter::Interact()
{
    // Hold finished
    if (BuiltStages > 0 || bDestroyed)
    {
        return;
    }

    SetPreviewOutline(false);
    SetStartPromptVisible(false);

    // Stop the player's trace from finding the site again, and keep it out of enemies' line of sight
    if (InteractVolume)
    {
        InteractVolume->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
    }

    UE_LOG(LogTemp, Log, TEXT("Shelter: construction started."));
    BuildNextStage();
}

void AShelter::SetPreviewOutline(bool bShow)
{
    if (bPreviewShown == bShow)
    {
        return;
    }
    bPreviewShown = bShow;

    // Every stage not built yet: outline only (no main pass, no shadow), or back to fully hidden
    for (int32 i = BuiltStages; i < StagePieces.Num(); ++i)
    {
        for (const TWeakObjectPtr<AActor>& Piece : StagePieces[i])
        {
            AActor* Actor = Piece.Get();
            if (Actor == nullptr)
            {
                continue;
            }

            Actor->SetActorHiddenInGame(!bShow);

            TInlineComponentArray<UPrimitiveComponent*> Prims(Actor);
            for (UPrimitiveComponent* Prim : Prims)
            {
                Prim->SetRenderInMainPass(!bShow);
                Prim->SetRenderCustomDepth(bShow);
                Prim->SetCastShadow(!bShow);
            }
        }
    }
}

void AShelter::SetStartPromptVisible(bool bVisible)
{
    if (PromptText)
    {
        PromptText->SetVisibility(bVisible);
    }
    if (PromptArrow)
    {
        PromptArrow->SetVisibility(bVisible);
    }

    // Nothing else needs Tick, so it runs only while the prompt is up
    SetActorTickEnabled(bVisible);
}

void AShelter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    PromptTime += DeltaSeconds;

    // Turn the message to face the camera, staying upright
    if (PromptText)
    {
        if (APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
        {
            FVector ToCamera = Camera->GetCameraLocation() - PromptText->GetComponentLocation();
            ToCamera.Z = 0.0f;
            if (!ToCamera.IsNearlyZero())
            {
                PromptText->SetWorldRotation(ToCamera.Rotation());
            }
        }
    }

    // Bob the arrow so it reads as pointing down at the site
    if (PromptArrow)
    {
        const float Bob = FMath::Sin(PromptTime * PromptBobSpeed * 2.0f * PI) * PromptBobHeight;
        PromptArrow->SetRelativeLocation(FVector(0.0f, 0.0f, PromptHeight - PromptArrowGap + Bob));
    }
}
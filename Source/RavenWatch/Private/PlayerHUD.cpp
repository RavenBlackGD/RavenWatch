// PlayerHUD.cpp - C++ parent for the HUD widget

#include "PlayerHUD.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "RavenGameMode.h"
#include "Components/PrimitiveComponent.h"
#include "Components/Widget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Survivor.h"
#include "TimerManager.h"

UPlayerHUD::UPlayerHUD(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SafeColor = FLinearColor(0.1f, 0.7f, 0.7f, 1.0f);
    DangerColor = FLinearColor(0.85f, 0.1f, 0.1f, 1.0f);
    SkyColor = FLinearColor(1.0f, 1.0f, 0.0f, 1.0f);
    TraceDistance = 100000.0f;
    EndPoint = FVector::ZeroVector;
    CrosshairMaterial = nullptr;
    MarkerLayer = nullptr;
}

// Get an editable material for this player's crosshair
void UPlayerHUD::NativeConstruct()
{
    Super::NativeConstruct();

    ClearInteractPrompt();
    ClearInteractProgress();

    // Show the objective now and update it whenever a count changes
    if (ARavenGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ARavenGameMode>() : nullptr)
    {
        GameMode->OnObjectiveChanged.AddUniqueDynamic(this, &UPlayerHUD::SetObjectiveText);
        GameMode->OnMissionAlert.AddUniqueDynamic(this, &UPlayerHUD::ShowAlert);
        SetObjectiveText(GameMode->GetObjectiveText());
    }

    else if (ObjectiveText)
    {
        // Menus and hub levels have no objective
        ObjectiveText->SetVisibility(ESlateVisibility::Collapsed);
    }

        HideAlert();

    if (Crosshair == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("PlayerHUD: Crosshair is missing. Check the widget name."));
        return;
    }

    CrosshairMaterial = Crosshair->GetDynamicMaterial();
    if (CrosshairMaterial == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("PlayerHUD: Crosshair needs M_Crosshair as its brush."));
        return;
    }

    CrosshairMaterial->SetVectorParameterValue(FName(TEXT("Color")), SafeColor);
}

// Update the crosshair while the HUD is active
void UPlayerHUD::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    UpdateAimTrace();
    UpdateSurvivorMarkers();
}

// Return the latest aim point
FVector UPlayerHUD::GetDestination() const
{
    return EndPoint;
}

// Trace from the crosshair and color it for the target
void UPlayerHUD::UpdateAimTrace()
{
    APlayerController* PC = GetOwningPlayer();
    UWorld* World = GetWorld();
    if (PC == nullptr || World == nullptr || Crosshair == nullptr)
    {
        return;
    }

    const FGeometry Geometry = Crosshair->GetCachedGeometry();
    if (Geometry.GetLocalSize().IsNearlyZero())
    {
        return;
    }

    FVector2D PixelPosition;
    FVector2D ViewportPosition;
    USlateBlueprintLibrary::LocalToViewport(
        this, Geometry, Geometry.GetLocalSize() * 0.5f,
        PixelPosition, ViewportPosition);

    FVector Start;
    FVector Direction;
    if (!PC->DeprojectScreenPositionToWorld(
        PixelPosition.X, PixelPosition.Y, Start, Direction))
    {
        return;
    }

    EndPoint = Start + Direction * TraceDistance;

    // Check walls as well as characters so we cannot aim through cover
    FCollisionObjectQueryParams ObjectTypes;
    ObjectTypes.AddObjectTypesToQuery(ECC_WorldStatic);
    ObjectTypes.AddObjectTypesToQuery(ECC_WorldDynamic);
    ObjectTypes.AddObjectTypesToQuery(ECC_Pawn);

    FCollisionQueryParams Params;
    if (APawn* Player = GetOwningPlayerPawn())
    {
        Params.AddIgnoredActor(Player);

        // Ignore the player's attached equipment too
        TArray<AActor*> AttachedActors;
        Player->GetAttachedActors(AttachedActors);
        Params.AddIgnoredActors(AttachedActors);
    }

    TArray<FHitResult> Hits;
    World->LineTraceMultiByObjectType(Hits, Start, EndPoint, ObjectTypes, Params);

    const FHitResult* Hit = Hits.FindByPredicate([](const FHitResult& Candidate)
        {
            const UPrimitiveComponent* Component = Candidate.GetComponent();
            return Component && Component->GetCollisionEnabled() != ECollisionEnabled::QueryOnly;
        });

    FLinearColor Color = SkyColor;
    if (Hit)
    {
        EndPoint = Hit->ImpactPoint;
        const ECollisionChannel HitObjectType = Hit->Component.IsValid()
            ? Hit->Component->GetCollisionObjectType()
            : ECC_WorldStatic;
        Color = (HitObjectType == ECC_Pawn) ? DangerColor : SafeColor;
    }

    if (CrosshairMaterial)
    {
        CrosshairMaterial->SetVectorParameterValue(FName(TEXT("Color")), Color);
    }
}

void UPlayerHUD::SetHealth(float Ratio)
{
    if (HealthBar)
    {
        HealthBar->SetPercent(Ratio);
    }
    else
    {
        // Log after error checking
        UE_LOG(LogTemp, Warning, TEXT("PlayerHUD: HealthBar was null. Check the widget name."));
    }
}

void UPlayerHUD::SetMana(float Ratio)
{
    if (ManaBar)
    {
        ManaBar->SetPercent(Ratio);
    }
}

void UPlayerHUD::SetAmmo(int32 Current, int32 Max)
{
    if (AmmoText)
    {
        AmmoText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), Current, Max)));
    }
}

void UPlayerHUD::SetInteractPrompt(const FText& Text)
{
    if (InteractPrompt)
    {
        InteractPrompt->SetText(Text);
        InteractPrompt->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
}

void UPlayerHUD::ClearInteractPrompt()
{
    if (InteractPrompt)
    {
        InteractPrompt->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UPlayerHUD::SetInteractProgress(float Ratio)
{
    if (InteractProgress)
    {
        InteractProgress->SetPercent(FMath::Clamp(Ratio, 0.0f, 1.0f));
        InteractProgress->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
}

void UPlayerHUD::ClearInteractProgress()
{
    if (InteractProgress)
    {
        InteractProgress->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UPlayerHUD::NativeDestruct()
{
    if (ARavenGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ARavenGameMode>() : nullptr)
    {
        GameMode->OnObjectiveChanged.RemoveDynamic(this, &UPlayerHUD::SetObjectiveText);
        GameMode->OnMissionAlert.RemoveDynamic(this, &UPlayerHUD::ShowAlert);
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(AlertTimer);
    }

    Super::NativeDestruct();
}

void UPlayerHUD::SetObjectiveText(const FText& Text)
{
    if (ObjectiveText)
    {
        ObjectiveText->SetText(Text);
        ObjectiveText->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
    // The game mode fires this on every kill, spawn, shelter hit and countdown tick
    RefreshDefendPanel();
}

void UPlayerHUD::ShowAlert(const FText& Text)
{
    if (AlertText == nullptr || GetWorld() == nullptr)
    {
        return;
    }

    AlertText->SetText(Text);
    AlertText->SetVisibility(ESlateVisibility::HitTestInvisible);

    // A new alert restarts the timer
    GetWorld()->GetTimerManager().SetTimer(AlertTimer, this, &UPlayerHUD::HideAlert, AlertDuration, false);
}

void UPlayerHUD::HideAlert()
{
    if (AlertText)
    {
        AlertText->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UPlayerHUD::RefreshDefendPanel()
{
    const ARavenGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ARavenGameMode>() : nullptr;

    // Only on the defend mission, and only once building has started
    const bool bShow = GameMode && GameMode->IsDefendMission() && GameMode->GetDefendWaveNumber() > 0;
    if (DefendPanel)
    {
        DefendPanel->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    }
    if (!bShow)
    {
        return;
    }

    FFormatNamedArguments Args;
    Args.Add(TEXT("Wave"), GameMode->GetDefendWaveNumber());
    Args.Add(TEXT("Waves"), GameMode->GetDefendWaveCount());
    Args.Add(TEXT("Seconds"), GameMode->GetSecondsToNextWave());
    Args.Add(TEXT("Remaining"), GameMode->GetDefendEnemiesRemaining());
    const float HealthRatio = GameMode->GetShelterHealthRatio();
    Args.Add(TEXT("Health"), FMath::RoundToInt(HealthRatio * 100.0f));

    if (WaveText)
    {
        WaveText->SetText(FText::Format(GameMode->IsDefendWaveActive() ? WaveActiveFormat : WaveIncomingFormat, Args));
    }
    if (EnemyCountText)
    {
        EnemyCountText->SetText(FText::Format(EnemyCountFormat, Args));
    }
    if (ShelterHealthBar)
    {
        ShelterHealthBar->SetPercent(HealthRatio);
    }
    if (ShelterHealthText)
    {
        ShelterHealthText->SetText(FText::Format(ShelterHealthFormat, Args));
    }
}

void UPlayerHUD::UpdateSurvivorMarkers()
{
    if (MarkerLayer == nullptr || SurvivorMarkerClass == nullptr)
    {
        return;
    }

    APlayerController* PC = GetOwningPlayer();
    const ARavenGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ARavenGameMode>() : nullptr;

    // Only survivors still waiting to be freed get a marker
    TArray<ASurvivor*> Trapped;
    if (PC && GameMode)
    {
        GameMode->GetLivingSurvivors(Trapped);
        Trapped.RemoveAll([](const ASurvivor* Survivor)
            {
                return Survivor->GetSurvivorState() != ESurvivorState::Trapped;
            });
    }

    const APawn* PlayerPawn = GetOwningPlayerPawn();
    const FVector PlayerLocation = PlayerPawn ? PlayerPawn->GetActorLocation() : FVector::ZeroVector;

    // Nearest first, then drop the far ones and anything past the cap
    Trapped.Sort([&PlayerLocation](const ASurvivor& A, const ASurvivor& B)
        {
            if (A.IsThreatened() != B.IsThreatened())
            {
                return A.IsThreatened();
            }
            return FVector::DistSquared(A.GetActorLocation(), PlayerLocation) < FVector::DistSquared(B.GetActorLocation(), PlayerLocation);
        });

    if (MarkerMaxDistance > 0.0f)
    {
        Trapped.RemoveAll([this, &PlayerLocation](const ASurvivor* Survivor)
            {
                return FVector::Dist(Survivor->GetActorLocation(), PlayerLocation) > MarkerMaxDistance;
            });
    }
    if (MaxSurvivorMarkers > 0 && Trapped.Num() > MaxSurvivorMarkers)
    {
        Trapped.SetNum(MaxSurvivorMarkers);
    }

    // Make more markers if needed. They are kept and reused.
    while (PC && MarkerPool.Num() < Trapped.Num())
    {
        UUserWidget* Marker = CreateWidget<UUserWidget>(PC, SurvivorMarkerClass);
        if (Marker == nullptr)
        {
            break;
        }
        if (UCanvasPanelSlot* NewSlot = MarkerLayer->AddChildToCanvas(Marker))
        {
            NewSlot->SetAutoSize(true);
            NewSlot->SetAlignment(FVector2D(0.5f, 0.5f));
        }
        MarkerPool.Add(Marker);
    }

    const FVector2D LayerSize = MarkerLayer->GetCachedGeometry().GetLocalSize();
    int32 ViewX = 0;
    int32 ViewY = 0;
    if (PC)
    {
        PC->GetViewportSize(ViewX, ViewY);
    }
    const bool bCanPlace = PC && ViewX > 0 && ViewY > 0 && !LayerSize.IsNearlyZero();

    FVector CameraLocation = FVector::ZeroVector;
    FRotator CameraRotation = FRotator::ZeroRotator;
    if (PC)
    {
        PC->GetPlayerViewPoint(CameraLocation, CameraRotation);
    }

    const FVector2D Center = LayerSize * 0.5f;
    const FVector2D Half = Center - FVector2D(MarkerEdgeMargin, MarkerEdgeMargin);

    for (int32 Index = 0; Index < MarkerPool.Num(); ++Index)
    {
        UUserWidget* Marker = MarkerPool[Index];
        if (Marker == nullptr)
        {
            continue;
        }

        if (!bCanPlace || Index >= Trapped.Num())
        {
            Marker->SetVisibility(ESlateVisibility::Collapsed);
            continue;
        }

        const FVector Target = Trapped[Index]->GetActorLocation() + FVector(0.0f, 0.0f, MarkerHeight);

        // Where the target is relative to the camera
        const FVector Local = CameraRotation.UnrotateVector(Target - CameraLocation);

        FVector2D ScreenPixels;
        const bool bProjected = Local.X > 0.0f && PC->ProjectWorldLocationToScreen(Target, ScreenPixels, true);

        // Viewport pixels to this canvas's units
        FVector2D Position = bProjected
            ? FVector2D(ScreenPixels.X * LayerSize.X / ViewX, ScreenPixels.Y * LayerSize.Y / ViewY)
            : Center;

        const bool bOffScreen = !bProjected
            || FMath::Abs(Position.X - Center.X) > Half.X
            || FMath::Abs(Position.Y - Center.Y) > Half.Y;

        float ArrowAngle = 0.0f;
        if (bOffScreen)
        {
            // Point from the screen centre toward the survivor, then push out to the edge
            FVector2D Direction(Local.Y, -Local.Z);
            if (Direction.IsNearlyZero())
            {
                Direction = FVector2D(0.0f, 1.0f);
            }
            const float ScaleX = Half.X / FMath::Max(FMath::Abs(Direction.X), KINDA_SMALL_NUMBER);
            const float ScaleY = Half.Y / FMath::Max(FMath::Abs(Direction.Y), KINDA_SMALL_NUMBER);
            Position = Center + Direction * FMath::Min(ScaleX, ScaleY);

            // The arrow is drawn pointing up, so add 90 degrees
            ArrowAngle = FMath::RadiansToDegrees(FMath::Atan2(Direction.Y, Direction.X)) + 90.0f;
        }

        if (UCanvasPanelSlot* MarkerSlot = Cast<UCanvasPanelSlot>(Marker->Slot))
        {
            MarkerSlot->SetPosition(Position);
        }

        if (UTextBlock* Distance = Cast<UTextBlock>(Marker->GetWidgetFromName(MarkerDistanceTextName)))
        {
            FFormatNamedArguments Args;
            Args.Add(TEXT("Meters"), FMath::RoundToInt(FVector::Dist(Trapped[Index]->GetActorLocation(), PlayerLocation) / 100.0f));
            Distance->SetText(FText::Format(MarkerDistanceFormat, Args));
        }

        const FLinearColor MarkerColor = Trapped[Index]->IsThreatened() ? MarkerThreatColor : MarkerSafeColor;

        if (UImage* Icon = Cast<UImage>(Marker->GetWidgetFromName(MarkerIconName)))
        {
            Icon->SetColorAndOpacity(MarkerColor);
        }

        if (UWidget* Arrow = Marker->GetWidgetFromName(MarkerArrowName))
        {
            Arrow->SetVisibility(bOffScreen ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
            Arrow->SetRenderTransformAngle(ArrowAngle);

            if (UTextBlock* ArrowText = Cast<UTextBlock>(Arrow))
            {
                ArrowText->SetColorAndOpacity(FSlateColor(MarkerColor));
            }
        }

        Marker->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
}
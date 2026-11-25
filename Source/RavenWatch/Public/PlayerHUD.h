// PlayerHUD.h - C++ parent for the HUD widget

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerHUD.generated.h"

class UCanvasPanel;

UCLASS()
class RAVENWATCH_API UPlayerHUD : public UUserWidget
{
    GENERATED_BODY()

public:

    UPlayerHUD(const FObjectInitializer& ObjectInitializer);

    // Point the rifle toward the crosshair
    UFUNCTION(BlueprintPure, Category = HUD)
    FVector GetDestination() const;

    // Sets the bar fill (0 to 1)
    UFUNCTION(BlueprintCallable, Category = HUD)
    void SetHealth(float Ratio);

    // Sets the mana bar fill (0 to 1)
    UFUNCTION(BlueprintCallable, Category = HUD)
    void SetMana(float Ratio);

    // Shows current / max ammo
    UFUNCTION(BlueprintCallable, Category = HUD)
    void SetAmmo(int32 Current, int32 Max);

    // Shows the interaction prompt with this text
    UFUNCTION(BlueprintCallable, Category = HUD)
    void SetInteractPrompt(const FText& Text);

    // Hides the interaction prompt
    UFUNCTION(BlueprintCallable, Category = HUD)
    void ClearInteractPrompt();

    // Shows the hold bar filled to this amount (0 to 1)
    UFUNCTION(BlueprintCallable, Category = HUD)
    void SetInteractProgress(float Ratio);

    // Hides the hold bar
    UFUNCTION(BlueprintCallable, Category = HUD)
    void ClearInteractProgress();

    // Shows the objective line. Bound to the game mode's OnObjectiveChanged.
    UFUNCTION(BlueprintCallable, Category = HUD)
    void SetObjectiveText(const FText& Text);

    // Reads the game mode and updates the Establish Shelter panel
    UFUNCTION(BlueprintCallable, Category = HUD)
    void RefreshDefendPanel();

    // Flashes AlertText for Alert Duration seconds. Bound to the game mode's OnMissionAlert.
    UFUNCTION(BlueprintCallable, Category = HUD)
    void ShowAlert(const FText& Text);

protected:

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    // Must be named Crosshair in the widget
    UPROPERTY(meta = (BindWidget))
    class UImage* Crosshair;

    // Editable crosshair colors
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = HUD)
    FLinearColor SafeColor;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = HUD)
    FLinearColor DangerColor;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = HUD)
    FLinearColor SkyColor;

    // Distance check for aim target
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = HUD)
    float TraceDistance;

    // Last aim point
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = HUD)
    FVector EndPoint;

    UPROPERTY()
    class UMaterialInstanceDynamic* CrosshairMaterial;

    // Check the aim target and update the color
    void UpdateAimTrace();

    // Places a marker over every trapped survivor, or on the screen edge pointing at them
    void UpdateSurvivorMarkers();

    // Full-screen Canvas Panel named MarkerLayer in the widget. No markers without it.
    UPROPERTY(meta = (BindWidgetOptional))
    UCanvasPanel* MarkerLayer;

    // Widget Blueprint used for one marker (WBP_SurvivorMarker)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Survivor Markers")
    TSubclassOf<UUserWidget> SurvivorMarkerClass;

    // Name of the distance Text block inside the marker widget
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Survivor Markers")
    FName MarkerDistanceTextName = TEXT("DistanceText");

    // Name of the arrow inside the marker widget. Shown only when the survivor is off-screen.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Survivor Markers")
    FName MarkerArrowName = TEXT("Arrow");

    // {Meters} is the distance from the player
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Survivor Markers")
    FText MarkerDistanceFormat = NSLOCTEXT("HUD", "MarkerDistance", "{Meters} m");

    // Height above the survivor's middle to put the marker, in cm
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Survivor Markers")
    float MarkerHeight = 120.0f;

    // Gap between an edge marker and the screen edge
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Survivor Markers", meta = (ClampMin = "0.0"))
    float MarkerEdgeMargin = 60.0f;

    // Show only this many of the nearest survivors. 0 shows all.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Survivor Markers", meta = (ClampMin = "0"))
    int32 MaxSurvivorMarkers = 0;

    // Hide markers for survivors farther than this, in cm. 0 shows them at any distance.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Survivor Markers", meta = (ClampMin = "0.0"))
    float MarkerMaxDistance = 0.0f;

    // Name of the icon Image inside the marker widget
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Survivor Markers")
    FName MarkerIconName = TEXT("IconImage");

    // Icon and arrow color normally
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Survivor Markers")
    FLinearColor MarkerSafeColor = FLinearColor(0.2f, 1.0f, 0.3f, 1.0f);

    // Icon and arrow color while an enemy is after that survivor
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Survivor Markers")
    FLinearColor MarkerThreatColor = FLinearColor(1.0f, 0.15f, 0.1f, 1.0f);

    // Markers made so far, reused every frame
    UPROPERTY()
    TArray<UUserWidget*> MarkerPool;

    // Must be named exactly HealthBar in the Widget Blueprint
    UPROPERTY(meta = (BindWidget))
    class UProgressBar* HealthBar;

    // Name a Progress Bar ManaBar in the widget to use it.
    UPROPERTY(meta = (BindWidgetOptional))
    class UProgressBar* ManaBar;

    // Set to AmmoText in BP
    UPROPERTY(meta = (BindWidget))
    class UTextBlock* AmmoText;

    // Name a Text block InteractPrompt in the widget to use it.
    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* InteractPrompt;

    // Name a Progress Bar InteractProgress in the widget to use it.
    UPROPERTY(meta = (BindWidgetOptional))
    class UProgressBar* InteractProgress;

    // Name a Text block ObjectiveText in the widget to use it.
    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* ObjectiveText;

    UPROPERTY(meta = (BindWidgetOptional))
    class UWidget* DefendPanel;

    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* WaveText;

    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* EnemyCountText;

    UPROPERTY(meta = (BindWidgetOptional))
    class UProgressBar* ShelterHealthBar;

    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* ShelterHealthText;

    // Name a Text block AlertText in the widget to use it
    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* AlertText;

    // Seconds an alert stays on screen
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Alerts", meta = (ClampMin = "0.1"))
    float AlertDuration = 3.0f;

    FTimerHandle AlertTimer;

    void HideAlert();

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Defend")
    FText WaveActiveFormat = NSLOCTEXT("HUD", "WaveActive", "Wave {Wave} / {Waves}");

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Defend")
    FText WaveIncomingFormat = NSLOCTEXT("HUD", "WaveIncoming", "Wave {Wave} / {Waves} in {Seconds}s");

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Defend")
    FText EnemyCountFormat = NSLOCTEXT("HUD", "EnemyCount", "Enemies: {Remaining}");

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Defend")
    FText ShelterHealthFormat = NSLOCTEXT("HUD", "ShelterHealth", "Shelter {Health}%");

};
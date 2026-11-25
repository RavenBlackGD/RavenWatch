// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/WidgetSwitcher.h"
#include "WBP_Settings.generated.h"

class UWBP_ButtonWithText;
class USlider;
class UComboBoxString;
class UCheckBox;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSettingsClosedSignature);

USTRUCT()
struct FPendingGraphicsSettings
{
	GENERATED_BODY()

	int32 ResolutionIndex = 0;
	TEnumAsByte<EWindowMode::Type> WindowMode = EWindowMode::Windowed;
	bool bVSync = false;

	int32 AntiAliasQuality = 1;
	int32 TextureQuality = 2;

	bool bAmbientOcclusion = true;
	bool bDepthOfField = true;
	bool bBloom = true;
	int32 ShadowQuality = 2;
	bool bMotionBlur = true;

	// Audio (0 to 100). Committed to the game instance on Apply.
	float MasterVolume = 100.0f;
	float EffectsVolume = 100.0f;
	float MusicVolume = 100.0f;
	float VoiceVolume = 100.0f;
};

/**
 *
 */
UCLASS()
class RAVENWATCH_API UWBP_Settings : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnSettingsClosedSignature OnSettingsClosedEvent;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidget))
	UWidgetSwitcher* SettingsSwitcher;

	UPROPERTY(meta = (BindWidget))
	UWBP_ButtonWithText* ControlsButton;

	UPROPERTY(meta = (BindWidget))
	UWBP_ButtonWithText* GraphicsButton;

	UPROPERTY(meta = (BindWidget))
	UWBP_ButtonWithText* AudioButton;

	UPROPERTY(meta = (BindWidget))
	UWBP_ButtonWithText* ExitButton;

	// Sliders run 0 to 100. Set Max Value to 100 on each one in the widget Blueprint.
	UPROPERTY(meta = (BindWidget))
	USlider* MasterSlider;

	UPROPERTY(meta = (BindWidget))
	USlider* EffectsSlider;

	UPROPERTY(meta = (BindWidget))
	USlider* MusicSlider;

	UPROPERTY(meta = (BindWidget))
	USlider* VoiceSlider;

	// --- Window Settings ---
	UPROPERTY(meta = (BindWidget))
	UComboBoxString* ResolutionBox;

	UPROPERTY(meta = (BindWidget))
	UComboBoxString* WindowModeBox;

	UPROPERTY(meta = (BindWidget))
	UCheckBox* VsyncCheck;

	UPROPERTY(meta = (BindWidget))
	UWBP_ButtonWithText* ApplyWindowButton;

	// Optional second Apply button on the Audio page. Runs the same apply and save.
	UPROPERTY(meta = (BindWidgetOptional))
	UWBP_ButtonWithText* ApplyAudioButton;

	// --- Basic & Advanced Graphics Settings ---
	UPROPERTY(meta = (BindWidget))
	UComboBoxString* AntiAliasBox;

	UPROPERTY(meta = (BindWidget))
	UComboBoxString* TextureDetailBox;

	//UPROPERTY(meta = (BindWidget))
	//UComboBoxString* WorldDetailBox;

	UPROPERTY(meta = (BindWidget))
	UCheckBox* AmbientOcclusionCheckBox;

	UPROPERTY(meta = (BindWidget))
	UCheckBox* DepthOfFieldCheckBox;

	UPROPERTY(meta = (BindWidget))
	UCheckBox* BloomCheckBox;

	UPROPERTY(meta = (BindWidget))
	UCheckBox* DynamicShadowsCheckBox;

	UPROPERTY(meta = (BindWidget))
	UCheckBox* MotionBlurCheckBox;

	// --- Save Status Switcher (Index 0 = Saved, Index 1 = Unsaved) ---
	UPROPERTY(meta = (BindWidgetOptional))
	UWidgetSwitcher* SaveStatusSwitcher;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* SettingsSavedText;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* SettingsUnsavedText;

	// How long the Saved / Unsaved popups stay visible.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI Feedback")
	float PopupDuration = 1.5f;

	UFUNCTION()
	void OnControlsClicked();

	UFUNCTION()
	void OnGraphicsClicked();

	UFUNCTION()
	void OnAudioClicked();

	UFUNCTION()
	void OnExitButtonClicked();

	UFUNCTION()
	void OnMasterVolumeChanged(float Value);

	UFUNCTION()
	void OnEffectsVolumeChanged(float Value);

	UFUNCTION()
	void OnMusicVolumeChanged(float Value);

	UFUNCTION()
	void OnVoiceVolumeChanged(float Value);

	UFUNCTION()
	void OnApplyWindowClicked();

	UFUNCTION()
	void OnResolutionSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void OnWindowModeSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void OnVSyncCheckChanged(bool bIsChecked);

	UFUNCTION()
	void OnAntiAliasChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void OnTextureDetailChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	//UFUNCTION()
	//void OnWorldDetailChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void OnAmbientOcclusionChanged(bool bIsChecked);

	UFUNCTION()
	void OnDepthOfFieldChanged(bool bIsChecked);

	UFUNCTION()
	void OnBloomChanged(bool bIsChecked);

	UFUNCTION()
	void OnDynamicShadowsChanged(bool bIsChecked);

	UFUNCTION()
	void OnMotionBlurChanged(bool bIsChecked);

private:
	TArray<FIntPoint> SupportedResolutions;
	FPendingGraphicsSettings PendingSettings;
	bool bHasUnsavedChanges = false;

	// Counts down while a Saved / Unsaved popup is visible. 0 means no popup.
	float PopupTimeRemaining = 0.0f;

	// True while the Unsaved popup is showing. When it times out the menu closes.
	bool bUnsavedWarningActive = false;

	void InitializeWindowSettings();
	void InitializeBasicGraphicsSettings();
	void InitializeAdvancedGraphicsSettings();
	void ApplyPendingGraphicsSettings();
	void RestoreSavedAudio();
	void MarkSettingsDirty();
	void ShowSaveStatusPopup(int32 SwitcherIndex);
	void HideSaveStatusPopup();
	void FinishClosingSettings();
	void SetConsoleVariableInt(const TCHAR* CVarName, int32 Value);
	int32 GetConsoleVariableInt(const TCHAR* CVarName, int32 DefaultValue) const;
};
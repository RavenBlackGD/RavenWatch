// Fill out your copyright notice in the Description page of Project Settings.

#include "WBP_Settings.h"
#include "WBP_ButtonWithText.h"
#include "Components/Slider.h"
#include "Components/ComboBoxString.h"
#include "Components/CheckBox.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/GameViewportClient.h"
#include "BasePlayer.h"
#include "CodeGameInstance.h"

void UWBP_Settings::NativeConstruct()
{
	Super::NativeConstruct();

	bHasUnsavedChanges = false;
	bUnsavedWarningActive = false;
	PopupTimeRemaining = 0.0f;

	if (SettingsSavedText)
	{
		SettingsSavedText->SetText(NSLOCTEXT("RavenWatchSettings", "SettingsSavedText", "Settings Saved"));
	}

	if (SettingsUnsavedText)
	{
		SettingsUnsavedText->SetText(NSLOCTEXT("RavenWatchSettings", "SettingsUnsavedText", "Settings Unsaved"));
	}

	if (SaveStatusSwitcher)
	{
		SaveStatusSwitcher->SetActiveWidgetIndex(0);
		SaveStatusSwitcher->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (ControlsButton)
	{
		ControlsButton->WidgetClickResponceEvent.AddUniqueDynamic(this, &UWBP_Settings::OnControlsClicked);
	}
	if (GraphicsButton)
	{
		GraphicsButton->WidgetClickResponceEvent.AddUniqueDynamic(this, &UWBP_Settings::OnGraphicsClicked);
	}
	if (AudioButton)
	{
		AudioButton->WidgetClickResponceEvent.AddUniqueDynamic(this, &UWBP_Settings::OnAudioClicked);
	}
	if (ExitButton)
	{
		ExitButton->WidgetClickResponceEvent.AddUniqueDynamic(this, &UWBP_Settings::OnExitButtonClicked);
	}
	if (ApplyAudioButton)
	{
		ApplyAudioButton->WidgetClickResponceEvent.AddUniqueDynamic(this, &UWBP_Settings::OnApplyWindowClicked);
	}

	if (MasterSlider)
	{
		MasterSlider->OnValueChanged.AddUniqueDynamic(this, &UWBP_Settings::OnMasterVolumeChanged);
	}
	if (EffectsSlider)
	{
		EffectsSlider->OnValueChanged.AddUniqueDynamic(this, &UWBP_Settings::OnEffectsVolumeChanged);
	}
	if (MusicSlider)
	{
		MusicSlider->OnValueChanged.AddUniqueDynamic(this, &UWBP_Settings::OnMusicVolumeChanged);
	}
	if (VoiceSlider)
	{
		VoiceSlider->OnValueChanged.AddUniqueDynamic(this, &UWBP_Settings::OnVoiceVolumeChanged);
	}

	UCodeGameInstance* GI = GetGameInstance<UCodeGameInstance>();

	const float Master = GI ? GI->MasterVolume : 100.0f;
	const float Effects = GI ? GI->EffectsVolume : 100.0f;
	const float Music = GI ? GI->MusicVolume : 100.0f;
	const float Voice = GI ? GI->VoiceVolume : 100.0f;

	PendingSettings.MasterVolume = Master;
	PendingSettings.EffectsVolume = Effects;
	PendingSettings.MusicVolume = Music;
	PendingSettings.VoiceVolume = Voice;

	if (MasterSlider)
	{
		MasterSlider->SetValue(Master);
	}
	if (EffectsSlider)
	{
		EffectsSlider->SetValue(Effects);
	}
	if (MusicSlider)
	{
		MusicSlider->SetValue(Music);
	}
	if (VoiceSlider)
	{
		VoiceSlider->SetValue(Voice);
	}

	if (GI)
	{
		GI->ApplyAudioSettings();
	}

	InitializeWindowSettings();
	InitializeBasicGraphicsSettings();
	InitializeAdvancedGraphicsSettings();

	// Ensure initial population does not mark settings dirty
	bHasUnsavedChanges = false;

	if (UWorld* World = GetWorld())
	{
		if (UGameViewportClient* Viewport = World->GetGameViewport())
		{
			Viewport->bDisableWorldRendering = true;
		}
	}

	// Hide the Player HUD
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (ABasePlayer* Player = Cast<ABasePlayer>(PC->GetPawn()))
		{
			Player->SetHUDVisible(false);
		}
	}
}

void UWBP_Settings::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		if (UGameViewportClient* Viewport = World->GetGameViewport())
		{
			Viewport->bDisableWorldRendering = false;
		}
	}

	if (APlayerController* PC = GetOwningPlayer())
	{
		if (ABasePlayer* Player = Cast<ABasePlayer>(PC->GetPawn()))
		{
			Player->SetHUDVisible(true);
		}
	}

	Super::NativeDestruct();
}

void UWBP_Settings::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (PopupTimeRemaining > 0.0f)
	{
		PopupTimeRemaining -= InDeltaTime;
		if (PopupTimeRemaining <= 0.0f)
		{
			// HideSaveStatusPopup() resets the flag, so capture it first
			const bool bShouldClose = bUnsavedWarningActive;
			HideSaveStatusPopup();

			if (bShouldClose)
			{
				FinishClosingSettings();
			}
		}
	}
}

void UWBP_Settings::OnControlsClicked()
{
	if (SettingsSwitcher)
	{
		SettingsSwitcher->SetActiveWidgetIndex(1);
	}
}

void UWBP_Settings::OnGraphicsClicked()
{
	if (SettingsSwitcher)
	{
		SettingsSwitcher->SetActiveWidgetIndex(2);
	}
}

void UWBP_Settings::OnAudioClicked()
{
	if (SettingsSwitcher)
	{
		SettingsSwitcher->SetActiveWidgetIndex(3);
	}
}

void UWBP_Settings::OnExitButtonClicked()
{
	// First click with unsaved changes shows the popup. The menu closes when it
	// times out, or immediately if Exit is pressed again while it is showing.
	if (bHasUnsavedChanges && !bUnsavedWarningActive)
	{
		ShowSaveStatusPopup(1);
		return;
	}

	FinishClosingSettings();
}

void UWBP_Settings::FinishClosingSettings()
{
	UCodeGameInstance* GI = GetGameInstance<UCodeGameInstance>();

	// Unapplied audio previews should not stay active after the menu closes
	if (bHasUnsavedChanges)
	{
		RestoreSavedAudio();
	}

	if (GI)
	{
		GI->SaveCustomSettings();
	}

	RemoveFromParent();
	OnSettingsClosedEvent.Broadcast();
}

void UWBP_Settings::RestoreSavedAudio()
{
	if (UCodeGameInstance* GI = GetGameInstance<UCodeGameInstance>())
	{
		GI->ApplyAudioSettings();
	}
}

void UWBP_Settings::OnMasterVolumeChanged(float Value)
{
	PendingSettings.MasterVolume = Value;
	if (UCodeGameInstance* GI = GetGameInstance<UCodeGameInstance>())
	{
		GI->ApplyVolumeToClass(GI->MasterSoundClass, Value); // live preview only
	}
	MarkSettingsDirty();
}

void UWBP_Settings::OnEffectsVolumeChanged(float Value)
{
	PendingSettings.EffectsVolume = Value;
	if (UCodeGameInstance* GI = GetGameInstance<UCodeGameInstance>())
	{
		GI->ApplyVolumeToClass(GI->EffectsSoundClass, Value);
	}
	MarkSettingsDirty();
}

void UWBP_Settings::OnMusicVolumeChanged(float Value)
{
	PendingSettings.MusicVolume = Value;
	if (UCodeGameInstance* GI = GetGameInstance<UCodeGameInstance>())
	{
		GI->ApplyVolumeToClass(GI->MusicSoundClass, Value);
	}
	MarkSettingsDirty();
}

void UWBP_Settings::OnVoiceVolumeChanged(float Value)
{
	PendingSettings.VoiceVolume = Value;
	if (UCodeGameInstance* GI = GetGameInstance<UCodeGameInstance>())
	{
		GI->ApplyVolumeToClass(GI->VoiceSoundClass, Value);
	}
	MarkSettingsDirty();
}

void UWBP_Settings::MarkSettingsDirty()
{
	bHasUnsavedChanges = true;
	HideSaveStatusPopup();
}

// WINDOW SETTINGS

void UWBP_Settings::InitializeWindowSettings()
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();

	if (ResolutionBox)
	{
		ResolutionBox->ClearOptions();
		SupportedResolutions.Empty();
		UKismetSystemLibrary::GetSupportedFullscreenResolutions(SupportedResolutions);

		if (SupportedResolutions.Num() == 0)
		{
			SupportedResolutions = {
				FIntPoint(1280, 720),
				FIntPoint(1600, 900),
				FIntPoint(1920, 1080),
				FIntPoint(2560, 1440),
				FIntPoint(3840, 2160)
			};
		}

		const FIntPoint CurrentRes = Settings ? Settings->GetScreenResolution() : FIntPoint(1920, 1080);
		int32 SelectedResIndex = 0;

		for (int32 i = 0; i < SupportedResolutions.Num(); ++i)
		{
			const FIntPoint& Res = SupportedResolutions[i];
			const FString Label = FString::Printf(TEXT("%d x %d"), Res.X, Res.Y);
			ResolutionBox->AddOption(Label);

			if (Res == CurrentRes)
			{
				SelectedResIndex = i;
			}
		}

		PendingSettings.ResolutionIndex = SelectedResIndex;
		ResolutionBox->SetSelectedIndex(SelectedResIndex);
		ResolutionBox->OnSelectionChanged.AddUniqueDynamic(this, &UWBP_Settings::OnResolutionSelectionChanged);
	}

	if (WindowModeBox)
	{
		WindowModeBox->ClearOptions();
		WindowModeBox->AddOption(TEXT("Fullscreen"));
		WindowModeBox->AddOption(TEXT("Borderless"));
		WindowModeBox->AddOption(TEXT("Windowed"));

		int32 ModeIdx = 2;
		EWindowMode::Type CurrentMode = EWindowMode::Windowed;

		if (Settings)
		{
			CurrentMode = Settings->GetFullscreenMode();
			switch (CurrentMode)
			{
			case EWindowMode::Fullscreen:
				ModeIdx = 0;
				break;
			case EWindowMode::WindowedFullscreen:
				ModeIdx = 1;
				break;
			case EWindowMode::Windowed:
			default:
				ModeIdx = 2;
				break;
			}
		}

		PendingSettings.WindowMode = CurrentMode;
		WindowModeBox->SetSelectedIndex(ModeIdx);
		WindowModeBox->OnSelectionChanged.AddUniqueDynamic(this, &UWBP_Settings::OnWindowModeSelectionChanged);
	}

	if (VsyncCheck)
	{
		const bool bVSync = Settings ? Settings->IsVSyncEnabled() : false;
		PendingSettings.bVSync = bVSync;
		VsyncCheck->SetIsChecked(bVSync);
		VsyncCheck->OnCheckStateChanged.AddUniqueDynamic(this, &UWBP_Settings::OnVSyncCheckChanged);
	}

	if (ApplyWindowButton)
	{
		ApplyWindowButton->WidgetClickResponceEvent.AddUniqueDynamic(this, &UWBP_Settings::OnApplyWindowClicked);
	}
}

void UWBP_Settings::OnResolutionSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (SelectionType == ESelectInfo::Direct || !ResolutionBox)
	{
		return;
	}

	const int32 ResIdx = ResolutionBox->GetSelectedIndex();
	if (SupportedResolutions.IsValidIndex(ResIdx))
	{
		PendingSettings.ResolutionIndex = ResIdx;
		MarkSettingsDirty();
	}
}

void UWBP_Settings::OnWindowModeSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (SelectionType == ESelectInfo::Direct || !WindowModeBox)
	{
		return;
	}

	switch (WindowModeBox->GetSelectedIndex())
	{
	case 0:
		PendingSettings.WindowMode = EWindowMode::Fullscreen;
		break;
	case 1:
		PendingSettings.WindowMode = EWindowMode::WindowedFullscreen;
		break;
	case 2:
	default:
		PendingSettings.WindowMode = EWindowMode::Windowed;
		break;
	}

	MarkSettingsDirty();
}

void UWBP_Settings::OnVSyncCheckChanged(bool bIsChecked)
{
	PendingSettings.bVSync = bIsChecked;
	MarkSettingsDirty();
}

void UWBP_Settings::OnApplyWindowClicked()
{
	UE_LOG(LogTemp, Log, TEXT("Settings Apply clicked"));
	ApplyPendingGraphicsSettings();
	bHasUnsavedChanges = false;
	ShowSaveStatusPopup(0);
}

void UWBP_Settings::ApplyPendingGraphicsSettings()
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (Settings)
	{
		if (SupportedResolutions.IsValidIndex(PendingSettings.ResolutionIndex))
		{
			Settings->SetScreenResolution(SupportedResolutions[PendingSettings.ResolutionIndex]);
		}

		Settings->SetFullscreenMode(PendingSettings.WindowMode);
		Settings->SetVSyncEnabled(PendingSettings.bVSync);
		Settings->SetAntiAliasingQuality(PendingSettings.AntiAliasQuality);
		Settings->SetTextureQuality(PendingSettings.TextureQuality);
		Settings->SetShadowQuality(PendingSettings.ShadowQuality);

		Settings->ApplySettings(false);
		Settings->SaveSettings();
	}

	SetConsoleVariableInt(TEXT("r.AmbientOcclusionLevels"), PendingSettings.bAmbientOcclusion ? -1 : 0);
	SetConsoleVariableInt(TEXT("r.DepthOfFieldQuality"), PendingSettings.bDepthOfField ? 2 : 0);
	SetConsoleVariableInt(TEXT("r.BloomQuality"), PendingSettings.bBloom ? 5 : 0);
	SetConsoleVariableInt(TEXT("r.MotionBlurQuality"), PendingSettings.bMotionBlur ? 4 : 0);

	if (UCodeGameInstance* GI = GetGameInstance<UCodeGameInstance>())
	{
		GI->bAmbientOcclusionEnabled = PendingSettings.bAmbientOcclusion;
		GI->bDepthOfFieldEnabled = PendingSettings.bDepthOfField;
		GI->bBloomEnabled = PendingSettings.bBloom;
		GI->bMotionBlurEnabled = PendingSettings.bMotionBlur;

		GI->MasterVolume = PendingSettings.MasterVolume;
		GI->EffectsVolume = PendingSettings.EffectsVolume;
		GI->MusicVolume = PendingSettings.MusicVolume;
		GI->VoiceVolume = PendingSettings.VoiceVolume;

		GI->SaveCustomSettings();
		GI->ApplyAudioSettings();

		UE_LOG(LogTemp, Log, TEXT("Settings Apply: saved volumes Master=%.1f Effects=%.1f Music=%.1f Voice=%.1f"),
			GI->MasterVolume, GI->EffectsVolume, GI->MusicVolume, GI->VoiceVolume);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Settings Apply: game instance is not UCodeGameInstance, audio was not saved. Check Project Settings > Maps & Modes."));
	}
}

void UWBP_Settings::ShowSaveStatusPopup(int32 SwitcherIndex)
{
	if (!SaveStatusSwitcher)
	{
		return;
	}

	SaveStatusSwitcher->SetActiveWidgetIndex(SwitcherIndex);
	SaveStatusSwitcher->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	bUnsavedWarningActive = (SwitcherIndex == 1);
	PopupTimeRemaining = PopupDuration;
}

void UWBP_Settings::HideSaveStatusPopup()
{
	if (SaveStatusSwitcher)
	{
		SaveStatusSwitcher->SetVisibility(ESlateVisibility::Collapsed);
	}

	PopupTimeRemaining = 0.0f;
	bUnsavedWarningActive = false;
}

// BASIC GRAPHICS SETTINGS

void UWBP_Settings::InitializeBasicGraphicsSettings()
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();

	if (AntiAliasBox)
	{
		AntiAliasBox->ClearOptions();
		AntiAliasBox->AddOption(TEXT("Off"));
		AntiAliasBox->AddOption(TEXT("FXAA Medium"));
		AntiAliasBox->AddOption(TEXT("High"));
		AntiAliasBox->AddOption(TEXT("Epic"));

		const int32 AAQuality = Settings ? FMath::Clamp(Settings->GetAntiAliasingQuality(), 0, 3) : 1;
		PendingSettings.AntiAliasQuality = AAQuality;
		AntiAliasBox->SetSelectedIndex(AAQuality);
		AntiAliasBox->OnSelectionChanged.AddUniqueDynamic(this, &UWBP_Settings::OnAntiAliasChanged);
	}
}

void UWBP_Settings::OnAntiAliasChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (SelectionType == ESelectInfo::Direct)
	{
		return;
	}

	int32 Quality = 1;
	if (SelectedItem == TEXT("Off")) { Quality = 0; }
	else if (SelectedItem == TEXT("FXAA Medium")) { Quality = 1; }
	else if (SelectedItem == TEXT("High")) { Quality = 2; }
	else if (SelectedItem == TEXT("Epic")) { Quality = 3; }

	PendingSettings.AntiAliasQuality = Quality;
	MarkSettingsDirty();
}

// ADVANCED GRAPHICS SETTINGS
void UWBP_Settings::InitializeAdvancedGraphicsSettings()
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	const UCodeGameInstance* GI = GetGameInstance<UCodeGameInstance>();

	if (TextureDetailBox)
	{
		TextureDetailBox->ClearOptions();
		TextureDetailBox->AddOption(TEXT("Low"));
		TextureDetailBox->AddOption(TEXT("Medium"));
		TextureDetailBox->AddOption(TEXT("High Quality"));
		TextureDetailBox->AddOption(TEXT("Epic"));

		const int32 TexQuality = Settings ? FMath::Clamp(Settings->GetTextureQuality(), 0, 3) : 2;
		PendingSettings.TextureQuality = TexQuality;
		TextureDetailBox->SetSelectedIndex(TexQuality);
		TextureDetailBox->OnSelectionChanged.AddUniqueDynamic(this, &UWBP_Settings::OnTextureDetailChanged);
	}

	if (AmbientOcclusionCheckBox)
	{
		const bool bChecked = GI ? GI->bAmbientOcclusionEnabled : (GetConsoleVariableInt(TEXT("r.AmbientOcclusionLevels"), 1) > 0);
		PendingSettings.bAmbientOcclusion = bChecked;
		AmbientOcclusionCheckBox->SetIsChecked(bChecked);
		AmbientOcclusionCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &UWBP_Settings::OnAmbientOcclusionChanged);
	}

	if (DepthOfFieldCheckBox)
	{
		const bool bChecked = GI ? GI->bDepthOfFieldEnabled : (GetConsoleVariableInt(TEXT("r.DepthOfFieldQuality"), 2) > 0);
		PendingSettings.bDepthOfField = bChecked;
		DepthOfFieldCheckBox->SetIsChecked(bChecked);
		DepthOfFieldCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &UWBP_Settings::OnDepthOfFieldChanged);
	}

	if (BloomCheckBox)
	{
		const bool bChecked = GI ? GI->bBloomEnabled : (GetConsoleVariableInt(TEXT("r.BloomQuality"), 5) > 0);
		PendingSettings.bBloom = bChecked;
		BloomCheckBox->SetIsChecked(bChecked);
		BloomCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &UWBP_Settings::OnBloomChanged);
	}

	if (DynamicShadowsCheckBox)
	{
		const int32 ShadowQuality = Settings ? Settings->GetShadowQuality() : 2;
		PendingSettings.ShadowQuality = ShadowQuality;
		DynamicShadowsCheckBox->SetIsChecked(ShadowQuality > 0);
		DynamicShadowsCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &UWBP_Settings::OnDynamicShadowsChanged);
	}

	if (MotionBlurCheckBox)
	{
		const bool bChecked = GI ? GI->bMotionBlurEnabled : (GetConsoleVariableInt(TEXT("r.MotionBlurQuality"), 4) > 0);
		PendingSettings.bMotionBlur = bChecked;
		MotionBlurCheckBox->SetIsChecked(bChecked);
		MotionBlurCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &UWBP_Settings::OnMotionBlurChanged);
	}
}

void UWBP_Settings::OnTextureDetailChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (SelectionType == ESelectInfo::Direct)
	{
		return;
	}

	int32 Level = 2;
	if (SelectedItem == TEXT("Low")) { Level = 0; }
	else if (SelectedItem == TEXT("Medium")) { Level = 1; }
	else if (SelectedItem == TEXT("High Quality")) { Level = 2; }
	else if (SelectedItem == TEXT("Epic")) { Level = 3; }

	PendingSettings.TextureQuality = Level;
	MarkSettingsDirty();
}

//void UWBP_Settings::OnWorldDetailChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
//{
//	if (SelectionType == ESelectInfo::Direct)
//	{
//		return;
//	}
//
//	if (UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings())
//	{
//		int32 Level = 2;
//		if (SelectedItem == TEXT("Low")) { Level = 0; }
//		else if (SelectedItem == TEXT("Medium")) { Level = 1; }
//		else if (SelectedItem == TEXT("High Quality")) { Level = 2; }
//		else if (SelectedItem == TEXT("Epic")) { Level = 3; }
//
//		Settings->SetFoliageQuality(Level);
//		Settings->ApplySettings(false);
//		Settings->SaveSettings();
//	}
//}

void UWBP_Settings::OnAmbientOcclusionChanged(bool bIsChecked)
{
	PendingSettings.bAmbientOcclusion = bIsChecked;
	MarkSettingsDirty();
}

void UWBP_Settings::OnDepthOfFieldChanged(bool bIsChecked)
{
	PendingSettings.bDepthOfField = bIsChecked;
	MarkSettingsDirty();
}

void UWBP_Settings::OnBloomChanged(bool bIsChecked)
{
	PendingSettings.bBloom = bIsChecked;
	MarkSettingsDirty();
}

void UWBP_Settings::OnDynamicShadowsChanged(bool bIsChecked)
{
	PendingSettings.ShadowQuality = bIsChecked ? 3 : 0;
	MarkSettingsDirty();
}

void UWBP_Settings::OnMotionBlurChanged(bool bIsChecked)
{
	PendingSettings.bMotionBlur = bIsChecked;
	MarkSettingsDirty();
}

void UWBP_Settings::SetConsoleVariableInt(const TCHAR* CVarName, int32 Value)
{
	if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(CVarName))
	{
		CVar->Set(Value, ECVF_SetByGameSetting);
	}
}

int32 UWBP_Settings::GetConsoleVariableInt(const TCHAR* CVarName, int32 DefaultValue) const
{
	if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(CVarName))
	{
		return CVar->GetInt();
	}
	return DefaultValue;
}
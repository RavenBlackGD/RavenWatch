// Fill out your copyright notice in the Description page of Project Settings.

#include "CodeGameInstance.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "GameFramework/PlayerController.h"
#include "BasePlayer.h"
#include "AttributeSystem.h"
#include "CombatAttributeSet.h"
#include "Equipment/EquipmentComponent.h"
#include "InventoryComponent.h"
#include "PlayerAttributeSet.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "RavenSaveGame.h"
#include "Misc/Paths.h"
#include "Misc/ConfigCacheIni.h"
#include "HAL/IConsoleManager.h"
#include "Sound/SoundMix.h"
#include "Sound/SoundClass.h"
#include "UObject/UObjectGlobals.h"

namespace RavenWatchSettingsConfig
{
	static const TCHAR* Section = TEXT("RavenWatch.Settings");

	static void SetCVarInt(const TCHAR* Name, int32 Value)
	{
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Name))
		{
			CVar->Set(Value, ECVF_SetByGameSetting);
		}
	}
}

const FString UCodeGameInstance::SaveSlotName = TEXT("RavenSaveSlot");
const int32 UCodeGameInstance::SaveUserIndex = 0;

void UCodeGameInstance::Init()
{
	Super::Init();
	LoadAndApplyCustomSettings();

	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this, &UCodeGameInstance::HandlePostLoadMap);
}

void UCodeGameInstance::Shutdown()
{
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
	Super::Shutdown();
}

void UCodeGameInstance::HandlePostLoadMap(UWorld* LoadedWorld)
{
	if (LoadedWorld && LoadedWorld->IsGameWorld() && LoadedWorld->GetGameInstance() == this)
	{
		ApplyAudioSettings();
	}
}

void UCodeGameInstance::ApplyAudioSettings()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (!DefaultSoundMix || !MasterSoundClass || !EffectsSoundClass || !MusicSoundClass || !VoiceSoundClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("UCodeGameInstance: Sound mix or sound classes are not set on the game instance Blueprint, so volume changes have no effect."));
		if (!DefaultSoundMix)
		{
			return;
		}
	}

	// Mixes don't survive level travel, so push once per world.
	if (MixPushedWorld.Get() != World)
	{
		UGameplayStatics::PushSoundMixModifier(World, DefaultSoundMix);
		MixPushedWorld = World;
	}

	ApplyVolumeToClass(MasterSoundClass, MasterVolume);
	ApplyVolumeToClass(EffectsSoundClass, EffectsVolume);
	ApplyVolumeToClass(MusicSoundClass, MusicVolume);
	ApplyVolumeToClass(VoiceSoundClass, VoiceVolume);
}

void UCodeGameInstance::ApplyVolumeToClass(USoundClass* SoundClass, float SliderValue)
{
	if (!DefaultSoundMix || !SoundClass)
	{
		return;
	}

	const float ScaledVolume = FMath::Clamp(SliderValue, 0.0f, 100.0f) / 100.0f;
	UGameplayStatics::SetSoundMixClassOverride(GetWorld(), DefaultSoundMix, SoundClass, ScaledVolume, 1.0f, 0.0f, true);
}

void UCodeGameInstance::SaveCustomSettings()
{
	const TCHAR* Section = RavenWatchSettingsConfig::Section;

	GConfig->SetFloat(Section, TEXT("MasterVolume"), MasterVolume, GGameUserSettingsIni);
	GConfig->SetFloat(Section, TEXT("EffectsVolume"), EffectsVolume, GGameUserSettingsIni);
	GConfig->SetFloat(Section, TEXT("MusicVolume"), MusicVolume, GGameUserSettingsIni);
	GConfig->SetFloat(Section, TEXT("VoiceVolume"), VoiceVolume, GGameUserSettingsIni);

	GConfig->SetBool(Section, TEXT("AmbientOcclusion"), bAmbientOcclusionEnabled, GGameUserSettingsIni);
	GConfig->SetBool(Section, TEXT("DepthOfField"), bDepthOfFieldEnabled, GGameUserSettingsIni);
	GConfig->SetBool(Section, TEXT("Bloom"), bBloomEnabled, GGameUserSettingsIni);
	GConfig->SetBool(Section, TEXT("LightShafts"), bLightShaftsEnabled, GGameUserSettingsIni);
	GConfig->SetBool(Section, TEXT("LensFlares"), bLensFlaresEnabled, GGameUserSettingsIni);
	GConfig->SetBool(Section, TEXT("MotionBlur"), bMotionBlurEnabled, GGameUserSettingsIni);

	GConfig->Flush(false, GGameUserSettingsIni);
}

void UCodeGameInstance::LoadAndApplyCustomSettings()
{
	const TCHAR* Section = RavenWatchSettingsConfig::Section;

	GConfig->GetFloat(Section, TEXT("MasterVolume"), MasterVolume, GGameUserSettingsIni);
	GConfig->GetFloat(Section, TEXT("EffectsVolume"), EffectsVolume, GGameUserSettingsIni);
	GConfig->GetFloat(Section, TEXT("MusicVolume"), MusicVolume, GGameUserSettingsIni);
	GConfig->GetFloat(Section, TEXT("VoiceVolume"), VoiceVolume, GGameUserSettingsIni);

	GConfig->GetBool(Section, TEXT("AmbientOcclusion"), bAmbientOcclusionEnabled, GGameUserSettingsIni);
	GConfig->GetBool(Section, TEXT("DepthOfField"), bDepthOfFieldEnabled, GGameUserSettingsIni);
	GConfig->GetBool(Section, TEXT("Bloom"), bBloomEnabled, GGameUserSettingsIni);
	GConfig->GetBool(Section, TEXT("LightShafts"), bLightShaftsEnabled, GGameUserSettingsIni);
	GConfig->GetBool(Section, TEXT("LensFlares"), bLensFlaresEnabled, GGameUserSettingsIni);
	GConfig->GetBool(Section, TEXT("MotionBlur"), bMotionBlurEnabled, GGameUserSettingsIni);

	RavenWatchSettingsConfig::SetCVarInt(TEXT("r.AmbientOcclusionLevels"), bAmbientOcclusionEnabled ? -1 : 0);
	RavenWatchSettingsConfig::SetCVarInt(TEXT("r.DepthOfFieldQuality"), bDepthOfFieldEnabled ? 2 : 0);
	RavenWatchSettingsConfig::SetCVarInt(TEXT("r.BloomQuality"), bBloomEnabled ? 5 : 0);
	RavenWatchSettingsConfig::SetCVarInt(TEXT("r.LightShaftQuality"), bLightShaftsEnabled ? 1 : 0);
	RavenWatchSettingsConfig::SetCVarInt(TEXT("r.LensFlareQuality"), bLensFlaresEnabled ? 2 : 0);
	RavenWatchSettingsConfig::SetCVarInt(TEXT("r.MotionBlurQuality"), bMotionBlurEnabled ? 4 : 0);
}

void UCodeGameInstance::BeginNewSession()
{
	ResetSession();
	SessionData.bSessionActive = true;
}

void UCodeGameInstance::ResetSession()
{
	SessionData.Reset();
	CurrentLevelIndex = SessionData.CurrentLevelIndex;
}

bool UCodeGameInstance::IsSessionActive() const
{
	return SessionData.bSessionActive;
}

void UCodeGameInstance::TravelToLevel(const FString& LevelName)
{
	UWorld* World = GetWorld();
	if (!World || LevelName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("UCodeGameInstance: cannot travel, no world or empty level name."));
		return;
	}

	if (APlayerController* PC = World->GetFirstPlayerController())
	{
		if (ABasePlayer* Player = Cast<ABasePlayer>(PC->GetPawn()))
		{
			CapturePlayerSession(Player);
		}

		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = false;

		PC->ClientTravel(LevelName, ETravelType::TRAVEL_Absolute);
	}
	else
	{
		World->ServerTravel(LevelName);
	}
}

void UCodeGameInstance::LoadFirstLevel()
{
	BeginNewSession();
	LoadLevelSafe(FirstLevelIndex);
}

void UCodeGameInstance::QuitTheGame()
{
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			PC->ConsoleCommand("quit");
		}
	}
}

void UCodeGameInstance::LoadLevelSafe(int32 Index)
{
	if (!GameLevels.IsValidIndex(Index))
	{
		UE_LOG(LogTemp, Warning, TEXT("UCodeGameInstance: level index %d is not valid - fill in GameLevels on the Game Instance."), Index);
		return;
	}

	CurrentLevelIndex = Index;
	SessionData.CurrentLevelIndex = CurrentLevelIndex;
	SessionData.bSessionActive = true;

	TravelToLevel(GameLevels[Index].ToString());
}

void UCodeGameInstance::QuitToMenu()
{
	ResetSession();
	if (!MainMenuLevel.IsNone())
	{
		TravelToLevel(MainMenuLevel.ToString());
	}
	else
	{
		LoadLevelSafe(0);
	}
}

bool UCodeGameInstance::SaveAndQuit()
{
	if (!SaveSession())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("UCodeGameInstance: Save and Quit cancelled because saving failed."));

		return false;
	}

	QuitToMenu();
	return true;
}

void UCodeGameInstance::ReplayCurrentLevel()
{
	if (UWorld* World = GetWorld())
	{
		FString MapName = World->GetMapName();
		MapName.RemoveFromStart(World->StreamingLevelsPrefix);
		TravelToLevel(MapName);
	}
}

bool UCodeGameInstance::SaveSession()
{
	if (!SessionData.bSessionActive)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("UCodeGameInstance: Cannot save because no session is active."));

		return false;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("UCodeGameInstance: Cannot save because the world is unavailable."));

		return false;
	}

	APlayerController* PC = World->GetFirstPlayerController();
	if (PC == nullptr)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("UCodeGameInstance: Cannot save because no player controller exists."));

		return false;
	}

	ABasePlayer* Player = Cast<ABasePlayer>(PC->GetPawn());
	if (Player == nullptr)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("UCodeGameInstance: Cannot save because the player pawn is unavailable."));

		return false;
	}

	if (!CapturePlayerSession(Player))
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("UCodeGameInstance: Player session capture failed."));

		return false;
	}

	URavenSaveGame* SaveGame =
		Cast<URavenSaveGame>(
			UGameplayStatics::CreateSaveGameObject(
				URavenSaveGame::StaticClass()));

	if (SaveGame == nullptr)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("UCodeGameInstance: Failed to create save object."));

		return false;
	}

	SaveGame->SaveVersion = 1;
	SaveGame->SessionData = SessionData;

	const bool bSaved = UGameplayStatics::SaveGameToSlot(
		SaveGame,
		SaveSlotName,
		SaveUserIndex);

	if (!bSaved)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("UCodeGameInstance: Failed to write save slot."));
	}
	else
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("UCodeGameInstance: Save directory is '%s'."),
			*FPaths::ProjectSavedDir());
	}

	return bSaved;
}

bool UCodeGameInstance::LoadSession()
{
	if (!DoesSaveExist())
	{
		return false;
	}

	USaveGame* LoadedSave =
		UGameplayStatics::LoadGameFromSlot(
			SaveSlotName,
			SaveUserIndex);

	URavenSaveGame* RavenSave =
		Cast<URavenSaveGame>(LoadedSave);

	if (RavenSave == nullptr)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("UCodeGameInstance: Save slot had an invalid save type."));

		return false;
	}

	if (RavenSave->SaveVersion != 1)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("UCodeGameInstance: Unsupported save version %d."),
			RavenSave->SaveVersion);

		return false;
	}

	SessionData = RavenSave->SessionData;
	SessionData.bSessionActive = true;
	CurrentLevelIndex = SessionData.CurrentLevelIndex;

	return true;
}

bool UCodeGameInstance::DoesSaveExist() const
{
	return UGameplayStatics::DoesSaveGameExist(
		SaveSlotName,
		SaveUserIndex);
}

bool UCodeGameInstance::DeleteSave()
{
	if (!DoesSaveExist())
	{
		return true;
	}

	return UGameplayStatics::DeleteGameInSlot(
		SaveSlotName,
		SaveUserIndex);
}

void UCodeGameInstance::ContinueSavedSession()
{
	if (!LoadSession())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("UCodeGameInstance: No valid saved session was found."));

		return;
	}

	if (!GameLevels.IsValidIndex(SessionData.CurrentLevelIndex))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("UCodeGameInstance: Saved level index %d is invalid."),
			SessionData.CurrentLevelIndex);

		ResetSession();
		return;
	}

	TravelToLevel(
		GameLevels[SessionData.CurrentLevelIndex].ToString());
}

bool UCodeGameInstance::CapturePlayerSession(ABasePlayer* Player)
{
	if (!SessionData.bSessionActive || Player == nullptr)
	{
		return false;
	}

	UAttributeSystem* AttributeSystem =
		Player->FindComponentByClass<UAttributeSystem>();

	UEquipmentComponent* EquipmentComponent =
		Player->FindComponentByClass<UEquipmentComponent>();

	UInventoryComponent* InventoryComponent =
		Player->FindComponentByClass<UInventoryComponent>();

	if (AttributeSystem == nullptr || EquipmentComponent == nullptr || InventoryComponent == nullptr)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("UCodeGameInstance: Required player component was missing during session capture."));

		return false;
	}

	SessionData.EquippedItems =
		EquipmentComponent->GetAllEquippedItems();

	SessionData.InventoryItems =
		InventoryComponent->GetItems();

	SessionData.BaseAttributes.Reset();

	for (const TObjectPtr<UAttributeSetBase>& AttributeSetObject :
		AttributeSystem->GetAttributeSets())
	{
		UAttributeSetBase* AttributeSet = AttributeSetObject.Get();

		if (AttributeSet == nullptr)
		{
			continue;
		}

		for (TFieldIterator<FStructProperty> PropertyIterator(
			AttributeSet->GetClass(),
			EFieldIteratorFlags::IncludeSuper);
			PropertyIterator;
			++PropertyIterator)
		{
			FStructProperty* StructProperty = *PropertyIterator;

			if (StructProperty == nullptr ||
				StructProperty->Struct != FAttribute::StaticStruct())
			{
				continue;
			}

			const FAttribute* Attribute =
				StructProperty->ContainerPtrToValuePtr<FAttribute>(AttributeSet);

			if (Attribute == nullptr ||
				!Attribute->IsValid() ||
				!Attribute->IsPersistent())
			{
				continue;
			}

			FSessionAttributeValue& Snapshot =
				SessionData.BaseAttributes.AddDefaulted_GetRef();

			Snapshot.AttributeSetClass = AttributeSet->GetClass();
			Snapshot.AttributeName = StructProperty->GetFName();
			Snapshot.BaseValue = Attribute->GetBaseValue();
		}
	}

	return true;
}

bool UCodeGameInstance::ApplyPlayerSession(ABasePlayer* Player)
{
	if (!SessionData.bSessionActive || Player == nullptr)
	{
		return false;
	}

	UAttributeSystem* AttributeSystem =
		Player->FindComponentByClass<UAttributeSystem>();

	UEquipmentComponent* EquipmentComponent =
		Player->FindComponentByClass<UEquipmentComponent>();

	UInventoryComponent* InventoryComponent =
		Player->FindComponentByClass<UInventoryComponent>();

	if (AttributeSystem == nullptr || EquipmentComponent == nullptr || InventoryComponent == nullptr)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("UCodeGameInstance: Required player component was missing during session restore."));

		return false;
	}

	for (const FSessionAttributeValue& Snapshot :
		SessionData.BaseAttributes)
	{
		if (!Snapshot.AttributeSetClass ||
			Snapshot.AttributeName.IsNone())
		{
			continue;
		}

		if (!AttributeSystem->SetBaseAttributeValue(
			Snapshot.AttributeSetClass,
			Snapshot.AttributeName,
			Snapshot.BaseValue))
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("UCodeGameInstance: Could not restore attribute %s."),
				*Snapshot.AttributeName.ToString());
		}
	}

	if (!EquipmentComponent->SetEquippedItems(SessionData.EquippedItems))
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("UCodeGameInstance: Could not restore equipped items."));

		return false;
	}

	if (!InventoryComponent->SetItems(SessionData.InventoryItems))
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("UCodeGameInstance: Could not restore inventory items."));
		return false;
	}

	return true;
}
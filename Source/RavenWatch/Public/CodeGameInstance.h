// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Equipment/EquipmentTypes.h"
#include "CodeGameInstance.generated.h"

class UAttributeSetBase;
class ABasePlayer;
class USoundMix;
class USoundClass;

USTRUCT(BlueprintType)
struct FSessionAttributeValue
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Session")
	TSubclassOf<UAttributeSetBase> AttributeSetClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Session")
	FName AttributeName = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Session")
	float BaseValue = 0.0f;
};

USTRUCT(BlueprintType)
struct FGameSessionData
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Session")
	int32 CurrentLevelIndex = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Session")
	bool bSessionActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Session")
	TArray<FEquipmentInstance> EquippedItems;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Session")
	TArray<FEquipmentInstance> InventoryItems;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Session")
	TArray<FSessionAttributeValue> BaseAttributes;

	void Reset()
	{
		CurrentLevelIndex = 0;
		bSessionActive = false;
		EquippedItems.Empty();
		InventoryItems.Empty();
		BaseAttributes.Empty();
	}
};

/**
 *
 */
UCLASS()
class RAVENWATCH_API UCodeGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	virtual void Shutdown() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Levels")
	TArray<FName> GameLevels;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Levels")
	FName MainMenuLevel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Levels")
	int32 FirstLevelIndex = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Levels")
	int32 CurrentLevelIndex = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Session")
	FGameSessionData SessionData;

	// Audio settings. 0 to 100 to match the settings sliders.
	// These live outside SessionData so ResetSession() does not touch them.
	UPROPERTY(BlueprintReadWrite, Category = "Audio Settings", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float MasterVolume = 100.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Audio Settings", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float EffectsVolume = 100.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Audio Settings", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float MusicVolume = 100.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Audio Settings", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float VoiceVolume = 100.0f;

	// Audio assets. Set these in the game instance Blueprint defaults.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio Settings")
	TObjectPtr<USoundMix> DefaultSoundMix;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio Settings")
	TObjectPtr<USoundClass> MasterSoundClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio Settings")
	TObjectPtr<USoundClass> EffectsSoundClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio Settings")
	TObjectPtr<USoundClass> MusicSoundClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio Settings")
	TObjectPtr<USoundClass> VoiceSoundClass;

	// Persistent graphics CVar toggles
	UPROPERTY(BlueprintReadWrite, Category = "Graphics Settings")
	bool bAmbientOcclusionEnabled = true;

	UPROPERTY(BlueprintReadWrite, Category = "Graphics Settings")
	bool bDepthOfFieldEnabled = true;

	UPROPERTY(BlueprintReadWrite, Category = "Graphics Settings")
	bool bBloomEnabled = true;

	UPROPERTY(BlueprintReadWrite, Category = "Graphics Settings")
	bool bLightShaftsEnabled = true;

	UPROPERTY(BlueprintReadWrite, Category = "Graphics Settings")
	bool bLensFlaresEnabled = true;

	UPROPERTY(BlueprintReadWrite, Category = "Graphics Settings")
	bool bMotionBlurEnabled = true;

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void SaveCustomSettings();

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void LoadAndApplyCustomSettings();

	// Applies the saved volumes to the sound classes.
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void ApplyAudioSettings();

	// Applies one 0-100 value to one sound class. Used for live slider previews.
	void ApplyVolumeToClass(USoundClass* SoundClass, float SliderValue);

	UFUNCTION(BlueprintCallable, Category = "Session")
	void BeginNewSession();

	UFUNCTION(BlueprintCallable, Category = "Session")
	void ResetSession();

	UFUNCTION(BlueprintPure, Category = "Session")
	bool IsSessionActive() const;

	UFUNCTION(BlueprintCallable, Category = "Game Flow")
	void LoadFirstLevel();

	UFUNCTION(BlueprintCallable, Category = "Game Flow")
	void QuitTheGame();

	UFUNCTION(BlueprintCallable, Category = "Game Flow")
	void LoadLevelSafe(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Game Flow")
	void QuitToMenu();

	UFUNCTION(BlueprintCallable, Category = "Save")
	bool SaveAndQuit();

	UFUNCTION(BlueprintCallable, Category = "Game Flow")
	void ReplayCurrentLevel();

	UFUNCTION(BlueprintCallable, Category = "Save")
	bool SaveSession();

	UFUNCTION(BlueprintCallable, Category = "Save")
	bool LoadSession();

	UFUNCTION(BlueprintPure, Category = "Save")
	bool DoesSaveExist() const;

	UFUNCTION(BlueprintCallable, Category = "Save")
	bool DeleteSave();

	UFUNCTION(BlueprintCallable, Category = "Save")
	void ContinueSavedSession();

	bool ApplyPlayerSession(ABasePlayer* Player);

private:
	bool CapturePlayerSession(ABasePlayer* Player);

	void TravelToLevel(const FString& LevelName);

	void HandlePostLoadMap(UWorld* LoadedWorld);

	FDelegateHandle PostLoadMapHandle;
	TWeakObjectPtr<UWorld> MixPushedWorld;

	static const FString SaveSlotName;
	static const int32 SaveUserIndex;
};
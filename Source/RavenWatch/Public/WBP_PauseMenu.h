// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WBP_PauseMenu.generated.h"

class UWBP_ButtonWithText;
class UWBP_Settings;

/**
 *
 */
UCLASS()
class RAVENWATCH_API UWBP_PauseMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWBP_ButtonWithText> ResumeButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWBP_ButtonWithText> ReplayButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWBP_ButtonWithText> SettingsButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWBP_ButtonWithText> QuitButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWBP_ButtonWithText> ReturnButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWBP_ButtonWithText> SaveButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWBP_ButtonWithText> SaveAndQuitButton;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Audio)
	USoundBase* PauseMusic;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = HUD)
	UWBP_Settings* SettingsWidget;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = HUD)
	TSubclassOf<UWBP_Settings> SettingsClass;

private:
	UFUNCTION()
	void OnResumeClicked();

	UFUNCTION()
	void OnReplayButtonClicked();

	UFUNCTION()
	void OnSettingsButtonClicked();

	UFUNCTION()
	void OnQuitButtonClicked();

	UFUNCTION()
	void OnReturnButtonClicked();

	UFUNCTION()
	void OnSaveButtonClicked();

	UFUNCTION()
	void OnSaveAndQuitButtonClicked();

	UFUNCTION()
	void HandleSettingsClosed();

	UPROPERTY()
	UAudioComponent* PauseMusicComponent;
};
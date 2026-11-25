// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WBP_MainMenu.generated.h"

class UWBP_ButtonWithText;
class UWBP_Settings;

UCLASS()
class RAVENWATCH_API UWBP_MainMenu : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	UWBP_ButtonWithText* PlayGameButton;

	UPROPERTY(meta = (BindWidget))
	UWBP_ButtonWithText* QuitGameButton;

	UPROPERTY(meta = (BindWidget))
	UWBP_ButtonWithText* LoadGameButton;

	UPROPERTY(meta = (BindWidget))
	UWBP_ButtonWithText* DeleteSaveButton;

	UPROPERTY(meta = (BindWidget))
	UWBP_ButtonWithText* SettingsButton;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD")
	TSubclassOf<UWBP_Settings> SettingsClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HUD")
	UWBP_Settings* SettingsWidget;

private:
	UFUNCTION()
	void OnPlayGameClicked();

	UFUNCTION()
	void OnQuitGameClicked();

	UFUNCTION()
	void OnLoadGameClicked();

	UFUNCTION()
	void OnDeleteSaveClicked();

	UFUNCTION()
	void OnSettingsClicked();

	UFUNCTION()
	void HandleSettingsClosed();
};
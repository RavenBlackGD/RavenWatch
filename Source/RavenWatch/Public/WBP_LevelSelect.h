// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WBP_LevelSelect.generated.h"

class UWBP_ButtonWithText;

/**
 * 
 */
UCLASS()
class RAVENWATCH_API UWBP_LevelSelect : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	// Binds to a button named exactly "Level1Button" in the Widget Designer
	UPROPERTY(meta = (BindWidget))
	UWBP_ButtonWithText* Level1Button;

	// Binds to a button named exactly "BackButton" in the Widget Designer
	UPROPERTY(meta = (BindWidget))
	UWBP_ButtonWithText* HubButton;

	UPROPERTY(meta = (BindWidget))
	UWBP_ButtonWithText* LeaveMenuButton;

	UPROPERTY(meta = (BindWidgetOptional))
	UWBP_ButtonWithText* Level2Button;

	UPROPERTY(meta = (BindWidgetOptional))
	UWBP_ButtonWithText* Level3Button;

	// Which entry in the Game Instance's Game Levels each mission button loads
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Levels")
	int32 Level1Index = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Levels")
	int32 Level2Index = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Levels")
	int32 Level3Index = 4;

private:
	UFUNCTION()
	void OnLevel1Clicked();

	UFUNCTION()
	void OnLevel2Clicked();

	UFUNCTION()
	void OnLevel3Clicked();

	// Loads a Game Levels entry through the Game Instance
	void LoadLevelIndex(int32 Index);

	UFUNCTION()
	void OnHubClicked();

	UFUNCTION()
	void OnLeaveMenuClicked();

};
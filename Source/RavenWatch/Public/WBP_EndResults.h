// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WBP_EndResults.generated.h"

class UWidgetSwitcher;
class UVerticalBox;
class UCodeGameInstance;
class UWBP_ButtonWithText;

/**
 * 
 */
UCLASS()
class RAVENWATCH_API UWBP_EndResults : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> ResultsSwitch;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> ButtonArea;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWBP_ButtonWithText> ReplayButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWBP_ButtonWithText> QuitButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWBP_ButtonWithText> ReturnButton;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	float TimeToMenu = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Default")
	TObjectPtr<UCodeGameInstance> GameInstance;

	UFUNCTION(BlueprintCallable, Category = "Default")
	void WinConditionMet();

	UFUNCTION(BlueprintCallable, Category = "Default")
	void LoadMainMenu();

private:
	UFUNCTION()
	void OnQuitButtonClicked();

	UFUNCTION()
	void OnReplayButtonClicked();

	UFUNCTION()
	void OnReturnButtonClicked();

	FTimerHandle TimerHandle_MenuTimer;
};

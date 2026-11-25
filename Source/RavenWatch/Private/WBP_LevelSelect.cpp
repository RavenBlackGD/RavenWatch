// Fill out your copyright notice in the Description page of Project Settings.

#include "WBP_LevelSelect.h"
#include "WBP_ButtonWithText.h"
#include "CodeGameInstance.h"
#include "Engine/World.h"

void UWBP_LevelSelect::NativeConstruct()
{
	Super::NativeConstruct();

	if (Level1Button)
	{
		Level1Button->WidgetClickResponceEvent.AddUniqueDynamic(this, &UWBP_LevelSelect::OnLevel1Clicked);
	}

	if (Level2Button)
	{
		Level2Button->WidgetClickResponceEvent.AddUniqueDynamic(this, &UWBP_LevelSelect::OnLevel2Clicked);
	}

	if (Level3Button)
	{
		Level3Button->WidgetClickResponceEvent.AddUniqueDynamic(this, &UWBP_LevelSelect::OnLevel3Clicked);
	}

	if (HubButton)
	{
		HubButton->WidgetClickResponceEvent.AddUniqueDynamic(this, &UWBP_LevelSelect::OnHubClicked);
	}

	if (LeaveMenuButton)
	{
		LeaveMenuButton->WidgetClickResponceEvent.AddUniqueDynamic(this, &UWBP_LevelSelect::OnLeaveMenuClicked);
	}
}

void UWBP_LevelSelect::OnLevel1Clicked()
{
	LoadLevelIndex(Level1Index);
}

void UWBP_LevelSelect::OnLevel2Clicked()
{
	LoadLevelIndex(Level2Index);
}

void UWBP_LevelSelect::OnLevel3Clicked()
{
	LoadLevelIndex(Level3Index);
}

void UWBP_LevelSelect::LoadLevelIndex(int32 Index)
{
	if (UWorld* World = GetWorld())
	{
		if (UCodeGameInstance* GameInst = World->GetGameInstance<UCodeGameInstance>())
		{
			GameInst->LoadLevelSafe(Index);
		}
	}
}

// Even though this function is now vestigial, I'm (Deven) leaving it so it doesn't break anything. Will need to come back to this 
// when the mission selector is more fleshed out and we have more levels to select from.
void UWBP_LevelSelect::OnHubClicked()
{

	if (APlayerController* PC = GetOwningPlayer())
	{
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);

		PC->bShowMouseCursor = false;

		PC->ClientTravel(TEXT("HubLevel"), ETravelType::TRAVEL_Absolute);
	}
}

void UWBP_LevelSelect::OnLeaveMenuClicked()
{
	APlayerController* PlayerController = GetOwningPlayer();
	if (PlayerController)
	{
		FInputModeGameOnly InputMode;
		PlayerController->SetInputMode(InputMode);

		PlayerController->bShowMouseCursor = false;
		PlayerController->SetPause(false);
	}

	RemoveFromParent();

}

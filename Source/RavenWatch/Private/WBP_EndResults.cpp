// Fill out your copyright notice in the Description page of Project Settings.


#include "WBP_EndResults.h"
#include "Components/WidgetSwitcher.h"
#include "Components/VerticalBox.h"
#include "WBP_ButtonWithText.h"
#include "CodeGameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "TimerManager.h"

void UWBP_EndResults::NativeConstruct()
{
	Super::NativeConstruct();

	GameInstance = Cast<UCodeGameInstance>(GetGameInstance());
	if (!GameInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("Error you have the wrong game instance socketed into the project settings"));
	}

	if (ButtonArea)
	{
		ButtonArea->SetVisibility(ESlateVisibility::Visible);
	}

	if (QuitButton)
	{
		QuitButton->WidgetClickResponceEvent.AddUniqueDynamic(this, &UWBP_EndResults::OnQuitButtonClicked);
	}

	if (ReplayButton)
	{
		ReplayButton->WidgetClickResponceEvent.AddUniqueDynamic(this, &UWBP_EndResults::OnReplayButtonClicked);
	}

	if (ReturnButton)
	{
		ReturnButton->WidgetClickResponceEvent.AddUniqueDynamic(this, &UWBP_EndResults::OnReturnButtonClicked);
	}

	if (APlayerController* PC = GetOwningPlayer())
	{
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = true;
	}
}

void UWBP_EndResults::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TimerHandle_MenuTimer);
	}

	Super::NativeDestruct();
}

void UWBP_EndResults::WinConditionMet()
{
	if (ResultsSwitch)
	{
		ResultsSwitch->SetActiveWidgetIndex(1);
	}

	if (ButtonArea)
	{
		ButtonArea->SetVisibility(ESlateVisibility::Visible);
	}

	if (TimeToMenu > 0.0f)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(TimerHandle_MenuTimer, this, &UWBP_EndResults::LoadMainMenu, TimeToMenu, false);
		}
	}
}

void UWBP_EndResults::LoadMainMenu()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TimerHandle_MenuTimer);
	}

	if (GameInstance)
	{
		GameInstance->QuitToMenu();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("UWBP_EndResults: no UCodeGameInstance, cannot go back to the main menu."));
	}
}

void UWBP_EndResults::OnQuitButtonClicked()
{
	if (UWorld* World = GetWorld())
	{
		if (UCodeGameInstance* GameInst = World->GetGameInstance<UCodeGameInstance>())
		{
			GameInst->QuitTheGame();
		}
	}
}

void UWBP_EndResults::OnReplayButtonClicked()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(TimerHandle_MenuTimer);

	if (GameInstance)
	{
		GameInstance->ReplayCurrentLevel();
	}
	else if (APlayerController* PC = GetOwningPlayer())
	{
		FString MapName = World->GetMapName();
		MapName.RemoveFromStart(World->StreamingLevelsPrefix);
		PC->ClientTravel(MapName, ETravelType::TRAVEL_Absolute);
	}
}

void UWBP_EndResults::OnReturnButtonClicked()
{
	if (UWorld* World = GetWorld())
	{
		if (UCodeGameInstance* GameInst = World->GetGameInstance<UCodeGameInstance>())
		{
			GameInst->QuitToMenu();
		}
	}
}
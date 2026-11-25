// Fill out your copyright notice in the Description page of Project Settings.

#include "WBP_PauseMenu.h"
#include "WBP_ButtonWithText.h"
#include "WBP_Settings.h"
#include "CodeGameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h" 
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "RavenGameMode.h"

void UWBP_PauseMenu::NativeConstruct()
{
	Super::NativeConstruct();

	if (ResumeButton)
	{
		ResumeButton->WidgetClickResponceEvent.AddUniqueDynamic(this, &UWBP_PauseMenu::OnResumeClicked);
	}

	if (ReplayButton)
	{
		ReplayButton->WidgetClickResponceEvent.AddUniqueDynamic(this, &UWBP_PauseMenu::OnReplayButtonClicked);
	}

	if (SettingsButton)
	{
		SettingsButton->WidgetClickResponceEvent.AddUniqueDynamic(this, &UWBP_PauseMenu::OnSettingsButtonClicked);
	}

	if (QuitButton)
	{
		QuitButton->WidgetClickResponceEvent.AddUniqueDynamic(this, &UWBP_PauseMenu::OnQuitButtonClicked);
	}

	if (ReturnButton)
	{
		ReturnButton->WidgetClickResponceEvent.AddUniqueDynamic(this, &UWBP_PauseMenu::OnReturnButtonClicked);
	}

	if (SaveButton)
	{
		SaveButton->WidgetClickResponceEvent.AddUniqueDynamic(this, &UWBP_PauseMenu::OnSaveButtonClicked);
	}

	if (SaveAndQuitButton)
	{
		SaveAndQuitButton->WidgetClickResponceEvent.AddUniqueDynamic(this, &UWBP_PauseMenu::OnSaveAndQuitButtonClicked);
	}

	if (PauseMusic)
	{
		PauseMusicComponent = UGameplayStatics::SpawnSound2D(this, PauseMusic);
	}

	if (UWorld* World = GetWorld())
	{
		if (ARavenGameMode* GameMode = Cast<ARavenGameMode>(World->GetAuthGameMode()))
		{
			GameMode->SetLevelMusicPaused(true);
		}
	}
}

void UWBP_PauseMenu::NativeDestruct()
{
	if (PauseMusicComponent)
	{
		PauseMusicComponent->Stop();
	}

	if (UWorld* World = GetWorld())
	{
		if (ARavenGameMode* GameMode = Cast<ARavenGameMode>(World->GetAuthGameMode()))
		{
			GameMode->SetLevelMusicPaused(false);
		}
	}

	Super::NativeDestruct();
}

void UWBP_PauseMenu::OnResumeClicked()
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetPause(false);
		PC->bShowMouseCursor = false;
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
	}

	RemoveFromParent();
}

void UWBP_PauseMenu::OnQuitButtonClicked()
{
	if (UWorld* World = GetWorld())
	{
		if (UCodeGameInstance* GameInst = World->GetGameInstance<UCodeGameInstance>())
		{
			GameInst->QuitTheGame();
		}
		else if (APlayerController* PC = GetOwningPlayer())
		{
			PC->ConsoleCommand("quit");
		}
	}
}

void UWBP_PauseMenu::OnSettingsButtonClicked()
{
	APlayerController* PlayerController = Cast<APlayerController>(GetOwningPlayer());
	if (PlayerController == nullptr)
	{
		return;
	}

	if (SettingsClass && !SettingsWidget)
	{
		SettingsWidget = CreateWidget<UWBP_Settings>(PlayerController, SettingsClass);
	}

	if (SettingsWidget)
	{
		SettingsWidget->OnSettingsClosedEvent.AddUniqueDynamic(this, &UWBP_PauseMenu::HandleSettingsClosed);

		SettingsWidget->AddToViewport(100);

		this->SetVisibility(ESlateVisibility::Hidden);

		PlayerController->bShowMouseCursor = true;

		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(SettingsWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PlayerController->SetInputMode(InputMode);
	}
}

void UWBP_PauseMenu::OnReplayButtonClicked()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetPause(false);
	}

	if (UCodeGameInstance* GameInst = World->GetGameInstance<UCodeGameInstance>())
	{
		GameInst->ReplayCurrentLevel();
	}
	else if (APlayerController* PC = GetOwningPlayer())
	{
		FString MapName = World->GetMapName();
		MapName.RemoveFromStart(World->StreamingLevelsPrefix);
		PC->ClientTravel(MapName, ETravelType::TRAVEL_Absolute);
	}
}

void UWBP_PauseMenu::OnReturnButtonClicked()
{
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = GetOwningPlayer())
		{
			PC->SetPause(false);
		}

		if (UCodeGameInstance* GameInst = World->GetGameInstance<UCodeGameInstance>())
		{
			GameInst->QuitToMenu();
		}
	}
}

void UWBP_PauseMenu::OnSaveButtonClicked()
{
	if (UWorld* World = GetWorld())
	{
		if (UCodeGameInstance* GameInst = World->GetGameInstance<UCodeGameInstance>())
		{
			GameInst->SaveSession();
		}
	}
}

void UWBP_PauseMenu::OnSaveAndQuitButtonClicked()
{
	if (UWorld* World = GetWorld())
	{
		if (UCodeGameInstance* GameInst = World->GetGameInstance<UCodeGameInstance>())
		{
			GameInst->SaveAndQuit();
		}
	}
}

void UWBP_PauseMenu::HandleSettingsClosed()
{
	this->SetVisibility(ESlateVisibility::Visible);

	if (APlayerController* PC = Cast<APlayerController>(GetOwningPlayer()))
	{
		PC->bShowMouseCursor = true;

		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(this->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
	}
}
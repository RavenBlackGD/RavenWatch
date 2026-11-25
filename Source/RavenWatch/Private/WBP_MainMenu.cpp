// Fill out your copyright notice in the Description page of Project Settings.


#include "WBP_MainMenu.h"
#include "WBP_ButtonWithText.h"
#include "WBP_Settings.h"
#include "CodeGameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

void UWBP_MainMenu::NativeConstruct()
{
	Super::NativeConstruct();

	if (PlayGameButton)
	{
		PlayGameButton->WidgetClickResponceEvent.AddUniqueDynamic(this, &UWBP_MainMenu::OnPlayGameClicked);
		PlayGameButton->FocusInnerButton();
	}

	if (QuitGameButton)
	{
		QuitGameButton->WidgetClickResponceEvent.AddUniqueDynamic(this, &UWBP_MainMenu::OnQuitGameClicked);
	}

	if (SettingsButton)
	{
		SettingsButton->WidgetClickResponceEvent.AddUniqueDynamic(this, &UWBP_MainMenu::OnSettingsClicked);
	}

	bool bSaveExists = false;

	if (UWorld* World = GetWorld())
	{
		if (UCodeGameInstance* GameInstance =
			World->GetGameInstance<UCodeGameInstance>())
		{
			bSaveExists = GameInstance->DoesSaveExist();
		}
	}

	if (LoadGameButton)
	{
		LoadGameButton->WidgetClickResponceEvent.AddUniqueDynamic(this, &UWBP_MainMenu::OnLoadGameClicked);

		LoadGameButton->SetVisibility(bSaveExists ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	if (DeleteSaveButton)
	{
		DeleteSaveButton->WidgetClickResponceEvent.AddUniqueDynamic(this, &UWBP_MainMenu::OnDeleteSaveClicked);

		
		DeleteSaveButton->SetVisibility(bSaveExists ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UWBP_MainMenu::OnPlayGameClicked()
{
	if (UWorld* World = GetWorld())
	{
		if (UCodeGameInstance* GameInstance =
			World->GetGameInstance<UCodeGameInstance>())
		{
			GameInstance->LoadFirstLevel();
			return;
		}
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("UWBP_MainMenu: Could not find UCodeGameInstance."));
}

void UWBP_MainMenu::OnQuitGameClicked()
{
	if (UWorld* World = GetWorld())
	{
		if (UCodeGameInstance* GameInst = World->GetGameInstance<UCodeGameInstance>())
		{
			GameInst->QuitTheGame();
		}
	}
}

void UWBP_MainMenu::OnLoadGameClicked()
{
	if (UWorld* World = GetWorld())
	{
		if (UCodeGameInstance* GameInst = World->GetGameInstance<UCodeGameInstance>())
		{
			GameInst->ContinueSavedSession();
		}
	}
}

void UWBP_MainMenu::OnDeleteSaveClicked()
{
	if (UWorld* World = GetWorld())
	{
		if (UCodeGameInstance* GameInst = World->GetGameInstance<UCodeGameInstance>())
		{
			GameInst->DeleteSave();

			if (LoadGameButton)
			{
				LoadGameButton->SetVisibility(ESlateVisibility::Collapsed);
			}
			
			if (DeleteSaveButton)
			{
				DeleteSaveButton->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}
}

void UWBP_MainMenu::OnSettingsClicked()
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
		SettingsWidget->OnSettingsClosedEvent.AddUniqueDynamic(this, &UWBP_MainMenu::HandleSettingsClosed);
		SettingsWidget->AddToViewport(100);
		this->SetVisibility(ESlateVisibility::Hidden);

		PlayerController->bShowMouseCursor = true;
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(SettingsWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PlayerController->SetInputMode(InputMode);
	}
}

void UWBP_MainMenu::HandleSettingsClosed()
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
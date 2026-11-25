// Fill out your copyright notice in the Description page of Project Settings.


#include "BP_MainMenuController.h"
#include "WBP_MainMenu.h"
#include "Blueprint/UserWidget.h"

ABP_MainMenuController::ABP_MainMenuController()
{
	MenuMusicComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("MenuMusicComponent"));

	MenuMusicComponent->bAutoActivate = false;
}

void ABP_MainMenuController::BeginPlay()
{
	Super::BeginPlay();

	if (MenuMusicComponent && MenuMusicComponent->Sound)
	{
		MenuMusicComponent->Play();
	}

	if (MenuClass)
	{
		if (UWBP_MainMenu* MenuWidget = CreateWidget<UWBP_MainMenu>(this, MenuClass))
		{
			MenuWidget->AddToViewport();

			FInputModeUIOnly InputMode;
			InputMode.SetWidgetToFocus(MenuWidget->TakeWidget());
			SetInputMode(InputMode);

		}
	}

	bShowMouseCursor = true;

}


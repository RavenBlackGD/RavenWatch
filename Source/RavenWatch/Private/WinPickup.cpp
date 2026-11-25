// Fill out your copyright notice in the Description page of Project Settings.

#include "WinPickup.h"
#include "BasePlayer.h"
#include "CodeGameInstance.h"
#include "Kismet/KismetSystemLibrary.h"
#include "TimerManager.h"
#include "WBP_EndResults.h"
#include "Blueprint/UserWidget.h"

AWinPickup::AWinPickup()
{
	bAlreadyTriggered = false;
}

bool AWinPickup::CanPickup(AActor* OtherActor) const
{
	if (bAlreadyTriggered)
	{
		return false;
	}

	return Cast<ABasePlayer>(OtherActor) != nullptr;
}

void AWinPickup::HandlePickup(AActor* OtherActor, const FHitResult& SweepResult)
{
	bAlreadyTriggered = true;

	UKismetSystemLibrary::PrintString(this, TEXT("YOU WIN"), true, true, FLinearColor::Green, 3.0f);
	UE_LOG(LogTemp, Log, TEXT("WinPickup: Player won."));

	if (ABasePlayer* Player = Cast<ABasePlayer>(OtherActor))
	{
		if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
		{
			Player->DisableInput(PC);
			PC->bShowMouseCursor = true;
		}
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("WinPickup: World was null."));
		return;
	}

	if (EndResultsWidgetClass)
	{
		if (UWBP_EndResults* EndResultsWidget = CreateWidget<UWBP_EndResults>(World, EndResultsWidgetClass))
		{
			EndResultsWidget->AddToViewport();

			// Trigger the win state on the widget to switch to the Win screen
			EndResultsWidget->WinConditionMet();
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("WinPickup: EndResultsWidgetClass is not assigned in the Blueprint."));
		}
	}
	/*UCodeGameInstance* GameInst = World->GetGameInstance<UCodeGameInstance>();
	if (GameInst == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("WinPickup: CodeGameInstance was null. Set it in Project Settings."));
		return;
	}*/

	/*FTimerHandle MenuTimer;
	World->GetTimerManager().SetTimer(MenuTimer, GameInst, &UCodeGameInstance::QuitToMenu, 2.0f, false);*/
}

void AWinPickup::PostPickup()
{
	// Empty on purpose - the win box stays in the level
}
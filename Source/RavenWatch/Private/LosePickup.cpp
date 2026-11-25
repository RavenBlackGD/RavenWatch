// Fill out your copyright notice in the Description page of Project Settings.

#include "LosePickup.h"
#include "BasePlayer.h"
#include "CodeGameInstance.h"
#include "Kismet/KismetSystemLibrary.h"
#include "TimerManager.h"
#include "WBP_EndResults.h"
#include "Blueprint/UserWidget.h"

ALosePickup::ALosePickup()
{
	bAlreadyTriggered = false;
}

bool ALosePickup::CanPickup(AActor* OtherActor) const
{
	if (bAlreadyTriggered)
	{
		return false;
	}

	return Cast<ABasePlayer>(OtherActor) != nullptr;
}

void ALosePickup::HandlePickup(AActor* OtherActor, const FHitResult& SweepResult)
{
	bAlreadyTriggered = true;

	UKismetSystemLibrary::PrintString(this, TEXT("YOU LOSE"), true, true, FLinearColor::Red, 3.0f);
	UE_LOG(LogTemp, Log, TEXT("LosePickup: Player lost."));

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
		UE_LOG(LogTemp, Warning, TEXT("LosePickup: World was null."));
		return;
	}

	if (EndResultsWidgetClass)
	{
		if (UWBP_EndResults* EndResultsWidget = CreateWidget<UWBP_EndResults>(World, EndResultsWidgetClass))
		{
			EndResultsWidget->AddToViewport();
			// Don't call WinConditionMet() here, so the WidgetSwitcher stays on the Lose screen
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("LosePickup: EndResultsWidgetClass is not assigned in the Blueprint."));
	}

	/*UCodeGameInstance* GameInst = World->GetGameInstance<UCodeGameInstance>();
	if (GameInst == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("LosePickup: CodeGameInstance was null. Set it in Project Settings."));
		return;
	}

	FTimerHandle MenuTimer;
	World->GetTimerManager().SetTimer(MenuTimer, GameInst, &UCodeGameInstance::QuitToMenu, 2.0f, false);*/
}

void ALosePickup::PostPickup()
{
	// Empty on purpose - the lose box stays in the level
}
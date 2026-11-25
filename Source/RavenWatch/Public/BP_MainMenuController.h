// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Components/AudioComponent.h"
#include "BP_MainMenuController.generated.h"

class UUserWidget;

/**
 * 
 */
UCLASS()
class RAVENWATCH_API ABP_MainMenuController : public APlayerController
{
	GENERATED_BODY()

public:

	ABP_MainMenuController();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> MenuClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Audio")
	UAudioComponent* MenuMusicComponent;

protected:
	virtual void BeginPlay() override;

};

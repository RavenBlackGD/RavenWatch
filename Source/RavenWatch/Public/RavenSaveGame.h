// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "CodeGameInstance.h"
#include "RavenSaveGame.generated.h"

/**
 * 
 */
UCLASS()
class RAVENWATCH_API URavenSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save")
	int32 SaveVersion = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save")
	FGameSessionData SessionData;
	
};

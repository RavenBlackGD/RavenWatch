// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BasePickup.h"
#include "WinPickup.generated.h"

class UWBP_EndResults;

UCLASS()
class RAVENWATCH_API AWinPickup : public ABasePickup
{
	GENERATED_BODY()

public:
	AWinPickup();

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UWBP_EndResults> EndResultsWidgetClass;
protected:
	virtual bool CanPickup(AActor* OtherActor) const override;
	virtual void HandlePickup(AActor* OtherActor, const FHitResult& SweepResult) override;
	virtual void PostPickup() override;

	// Stops double-trigger while we wait for the menu
	bool bAlreadyTriggered = false;
};
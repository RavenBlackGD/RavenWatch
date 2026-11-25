// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BasePickup.h"
#include "LosePickup.generated.h"

class UWBP_EndResults;

UCLASS()
class RAVENWATCH_API ALosePickup : public ABasePickup
{
	GENERATED_BODY()

public:
	ALosePickup();

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UWBP_EndResults> EndResultsWidgetClass;
protected:
	virtual bool CanPickup(AActor* OtherActor) const override;
	virtual void HandlePickup(AActor* OtherActor, const FHitResult& SweepResult) override;
	virtual void PostPickup() override;

	bool bAlreadyTriggered = false;
};
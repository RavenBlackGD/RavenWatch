// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Tools/PlayerToolBase.h"
#include "TestTool.generated.h"

/**
 * 
 */
UCLASS()
class RAVENWATCH_API ATestTool : public APlayerToolBase
{
	GENERATED_BODY()

public: 
	ATestTool();

	virtual void InitializeTool() override;

	virtual void ActivateAbility(int32 AbilityIndex) override;

protected:
	virtual void BeginPlay() override;

	// Add effect actors, projectiles, or traces here for the abilities. This class owns the ability-specific behavior.
	
};

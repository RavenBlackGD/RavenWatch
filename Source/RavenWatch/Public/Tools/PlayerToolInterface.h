// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PlayerToolInterface.generated.h"

struct FPlayerToolData;
struct FToolAbility;

class UToolAttributeSet;

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UPlayerToolInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class RAVENWATCH_API IPlayerToolInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	// Returns the tool data for this tool
	virtual FPlayerToolData GetToolData() const = 0;

	// Returns the ability data for the specified ability index (1 or 2)
	virtual FToolAbility GetAbilityData(int32 AbilityIndex) const = 0;

	// Returns the ability attributes for the specified ability index (1 or 2)
	virtual UToolAttributeSet* GetAbilityAttributes(int32 AbilityIndex) const = 0;

	// Activate the ability at the given index (1 or 2)
	virtual void ActivateAbility(int32 AbilityIndex) = 0;
};

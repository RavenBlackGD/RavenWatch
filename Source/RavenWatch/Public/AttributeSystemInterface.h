// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "AttributeSystemInterface.generated.h"

class UAttributeSystem;
class UAttributeEffectComponent;

// This class does not need to be modified.
UINTERFACE(MinimalAPI, BlueprintType)
class UAttributeSystemInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class RAVENWATCH_API IAttributeSystemInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	virtual UAttributeSystem* GetAttributeSystem() const = 0;

	virtual UAttributeEffectComponent* GetAttributeEffectComponent() const = 0;
};

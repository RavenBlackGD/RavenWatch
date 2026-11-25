// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSetBase.h"
#include "PlayerAttributeSet.generated.h"

/**
 * 
 */
UCLASS()
class RAVENWATCH_API UPlayerAttributeSet : public UAttributeSetBase
{
	GENERATED_BODY()
	
public:
	UPlayerAttributeSet();

	UPROPERTY(EditDefaultsOnly, Category = "Player|Primary")
	FAttribute Strength;

	UPROPERTY(EditDefaultsOnly, Category = "Player|Primary")
	FAttribute Dexterity;

	UPROPERTY(EditDefaultsOnly, Category = "Player|Primary")
	FAttribute Intelligence;

	UPROPERTY(EditDefaultsOnly, Category = "Player|Resources")
	FAttribute MaxMana;

	UPROPERTY(EditDefaultsOnly, Category = "Player|Resources")
	FAttribute CurrentMana;

	// Mana gained per second while the player is not spending any
	UPROPERTY(EditDefaultsOnly, Category = "Player|Resources")
	FAttribute ManaRegenRate;

	// Seconds after spending mana before it starts coming back
	UPROPERTY(EditDefaultsOnly, Category = "Player|Resources")
	FAttribute ManaRegenDelay;

	// Keeps CurrentMana between 0 and MaxMana
	virtual bool PreAttributeChange(const FName& AttributeName, float& NewValue) override;
	virtual bool PreAttributeBaseChange(const FName& AttributeName, float& NewValue) override;
};

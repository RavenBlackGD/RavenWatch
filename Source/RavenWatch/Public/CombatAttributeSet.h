// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSetBase.h"
#include "CombatAttributeSet.generated.h"

/**
 * 
 */
UCLASS()
class RAVENWATCH_API UCombatAttributeSet : public UAttributeSetBase
{
	GENERATED_BODY()
	
public:
	UCombatAttributeSet();

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Health")
	FAttribute CurrentHealth;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Health")
	FAttribute MaxHealth;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Defense")
	FAttribute Armor;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Defense")
	FAttribute EvadeChance;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Offense")
	FAttribute DamageAmount;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Offense")
	FAttribute AttackSpeed;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Offense")
	FAttribute CriticalChance;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Offense")
	FAttribute CriticalDamage;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Movement")
	FAttribute MovementSpeed;
};

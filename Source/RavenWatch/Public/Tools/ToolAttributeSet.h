// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSetBase.h"
#include "ToolAttributeSet.generated.h"

/**
 * 
 */
UCLASS()
class RAVENWATCH_API UToolAttributeSet : public UAttributeSetBase
{
	GENERATED_BODY()

public:

	UToolAttributeSet();

	UPROPERTY(EditDefaultsOnly, Category = "Tool")
	FAttribute CooldownTime;

	UPROPERTY(EditDefaultsOnly, Category = "Tool")
	FAttribute ManaCost;

	UPROPERTY(EditDefaultsOnly, Category = "Tool")
	FAttribute Range;

	UPROPERTY(EditDefaultsOnly, Category = "Tool")
	FAttribute AreaOfEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Tool")
	FAttribute Duration;

	UPROPERTY(EditDefaultsOnly, Category = "Tool")
	FAttribute ProjectileSpeed;

	UPROPERTY(EditDefaultsOnly, Category = "Tool")
	FAttribute AbilityDamage;

	UPROPERTY(EditDefaultsOnly, Category = "Tool")
	FAttribute AbilityMagnitude;

	// The Synergy attribute is to be used to determine the values for effects that trigger when two abilities synergize.
	// For example, the Dysfoam Sprayer triggers a disintegration effect when its foam is activated by its Primer Blast ability.
	// The synergy value can be used to determine the damage of this combined effect,
	// and can be calculated by using the synergy attribute of both abilities. 
	// This allows for synergy between different tool abilities as well. Calculations must be determined on an ability by ability basis.
	UPROPERTY(EditDefaultsOnly, Category = "Tool")
	FAttribute Synergy;

	
};

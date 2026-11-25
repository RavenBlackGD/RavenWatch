// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSetBase.h"
#include "AbilityEffectAttributeSet.generated.h"

/**
 * This attribute set is for actors created by ability effects, such as projectiles or area of effect actors. 
 * It contains attributes that are relevant to the effect's behavior that are not provided by the ability's tool attribute set.
 * These attributes can be modified by the ability's tool attribute set when the effect is created, 
 * but are primarily for tracking the actors attributes that are independant from the ability that created them.
 * These attributes should be statistics like the health of an object created by the ability, 
 * or for secondary effects that aren't modified by the ability's tool attribute set.
 * 
 * No current tools implement abilities that require this right now. 11/19/2026
 */
UCLASS()
class RAVENWATCH_API UAbilityEffectAttributeSet : public UAttributeSetBase
{
	GENERATED_BODY()
	
public:

	UAbilityEffectAttributeSet();

	UPROPERTY(EditDefaultsOnly, Category = "AbilityEffect")
	FAttribute EffectDuration;

	UPROPERTY(EditDefaultsOnly, Category = "AbilityEffect")
	FAttribute SpecialEffectMagnitude;

	UPROPERTY(EditDefaultsOnly, Category = "AbilityEffect")
	FAttribute CurrentHealth;

	UPROPERTY(EditDefaultsOnly, Category = "AbilityEffect")
	FAttribute MaxHealth;
};

// Fill out your copyright notice in the Description page of Project Settings.


#include "Tools/AbilityEffectAttributeSet.h"

UAbilityEffectAttributeSet::UAbilityEffectAttributeSet()
{
	EffectDuration.Initialize(0.0f); 
	SpecialEffectMagnitude.Initialize(0.0f);
	CurrentHealth.Initialize(0.0f);
	MaxHealth.Initialize(0.0f);
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "Tools/ToolAttributeSet.h"

UToolAttributeSet::UToolAttributeSet()
{
	
	// Default values should be zero, and changed on ability creation. 
	// Not all attributes will be used for every ability, so they should be initialized to zero to avoid unintended effects.
	CooldownTime.Initialize(1.0f);
	ManaCost.Initialize(0.0f);
	Range.Initialize(0.0f);
	AreaOfEffect.Initialize(0.0f);
	Duration.Initialize(0.0f);
	ProjectileSpeed.Initialize(0.0f);
	AbilityDamage.Initialize(0.0f);
	AbilityMagnitude.Initialize(0.0f);
	Synergy.Initialize(0.0f);

}

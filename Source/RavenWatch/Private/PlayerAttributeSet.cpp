// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerAttributeSet.h"

UPlayerAttributeSet::UPlayerAttributeSet()
{
	Strength.Initialize(10.0f);
	Dexterity.Initialize(10.0f);
	Intelligence.Initialize(10.0f);
	MaxMana.Initialize(100.0f);
	CurrentMana.Initialize(100.0f);
	ManaRegenRate.Initialize(5.0f);
	ManaRegenDelay.Initialize(2.0f);
}

bool UPlayerAttributeSet::PreAttributeChange(const FName& AttributeName, float& NewValue)
{
	if (AttributeName == GET_MEMBER_NAME_CHECKED(UPlayerAttributeSet, CurrentMana))
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, FMath::Max(0.0f, MaxMana.GetCurrentValue()));
	}

	return true;
}

bool UPlayerAttributeSet::PreAttributeBaseChange(const FName& AttributeName, float& NewValue)
{
	if (AttributeName == GET_MEMBER_NAME_CHECKED(UPlayerAttributeSet, CurrentMana))
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, FMath::Max(0.0f, MaxMana.GetCurrentValue()));
	}

	return true;
}

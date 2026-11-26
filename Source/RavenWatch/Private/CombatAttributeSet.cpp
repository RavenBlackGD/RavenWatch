// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatAttributeSet.h"


UCombatAttributeSet::UCombatAttributeSet()
{
	CurrentHealth.Initialize(100.0f);
	MaxHealth.Initialize(100.0f);
	Armor.Initialize(0.0f);
	EvadeChance.Initialize(0.0f);
	DamageAmount.Initialize(10.0f);
	AttackSpeed.Initialize(100.0f);
	CriticalChance.Initialize(5.0f);
	CriticalDamage.Initialize(150.0f);
	MovementSpeed.Initialize(100.0f);
	FlatFireDamage.Initialize(0.0f);
	FlatColdDamage.Initialize(0.0f);
	FlatLightningDamage.Initialize(0.0f);
	IncreasedFireDamage.Initialize(0.0f);
	IncreasedColdDamage.Initialize(0.0f);
	IncreasedLightningDamage.Initialize(0.0f);
}


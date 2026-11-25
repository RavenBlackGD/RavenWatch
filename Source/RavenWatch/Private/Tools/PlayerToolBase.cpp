// Fill out your copyright notice in the Description page of Project Settings.


#include "Tools/PlayerToolBase.h"

// Sets default values
APlayerToolBase::APlayerToolBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

void APlayerToolBase::InitializeTool()
{
}

// Called when the game starts or when spawned
void APlayerToolBase::BeginPlay()
{
	Super::BeginPlay();
	
	EnsureAbilityAttributes();
}

FToolAbility APlayerToolBase::GetAbilityData(int32 AbilityIndex) const
{
	const FToolAbility* Ability = ToolData.GetAbility(AbilityIndex);
	return Ability != nullptr ? *Ability : FToolAbility();
}

void APlayerToolBase::ActivateAbility(int32 AbilityIndex)
{
}

UToolAttributeSet* APlayerToolBase::GetAbilityAttributes(int32 AbilityIndex) const
{
	return ToolData.GetAbilityAttributes(AbilityIndex);
}

void APlayerToolBase::SetAbilityAttributes(
	int32 AbilityIndex,
	UToolAttributeSet* NewAttributes)
{
	FToolAbility* Ability = ToolData.GetAbility(AbilityIndex);

	if (Ability != nullptr)
	{
		Ability->AbilityAttributes = NewAttributes;
	}
}

void APlayerToolBase::EnsureAbilityAttributes()
{
	if (ToolData.Ability1.AbilityAttributes == nullptr)
	{
		ToolData.Ability1.AbilityAttributes =
			NewObject<UToolAttributeSet>(this);
	}

	if (ToolData.Ability2.AbilityAttributes == nullptr)
	{
		ToolData.Ability2.AbilityAttributes =
			NewObject<UToolAttributeSet>(this);
	}
}
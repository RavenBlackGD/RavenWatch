// Fill out your copyright notice in the Description page of Project Settings.


#include "Tools/TestTool.h"

ATestTool::ATestTool()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ATestTool::InitializeTool()
{
	// For concrete tools, we can set the values for the data in the editor.
	// Doing it in code is functional if done as follows, but changes should be made in the editor for concrete tools, 
	// including the tool icon and ability icons.
	if (ToolData.ToolName.IsNone())
	{
		ToolData.ToolName = FName(TEXT("Test Tool"));
	}
	if (ToolData.ToolDescription.IsEmpty())
	{
		ToolData.ToolDescription = FText::FromString(TEXT("A test tool for demonstration purposes."));
	}

	if (ToolData.Ability1.AbilityName.IsNone())
	{
		ToolData.Ability1.AbilityName = FName(TEXT("Test Ability 1"));
	}
	if (ToolData.Ability1.AbilityDescription.IsEmpty())
	{
		ToolData.Ability1.AbilityDescription = FText::FromString(TEXT("This is the first test ability."));
	}

	if (ToolData.Ability2.AbilityName.IsNone())
	{
		ToolData.Ability2.AbilityName = FName(TEXT("Test Ability 2"));
	}
	if (ToolData.Ability2.AbilityDescription.IsEmpty())
	{
		ToolData.Ability2.AbilityDescription = FText::FromString(TEXT("This is the second test ability."));
	}


	if (GetAbilityAttributes(1) != nullptr)
	{
		GetAbilityAttributes(1)->ManaCost.Initialize(10.0f);
		GetAbilityAttributes(1)->CooldownTime.Initialize(1.0f);
		GetAbilityAttributes(1)->AbilityDamage.Initialize(20.0f);
	}

	if (GetAbilityAttributes(2) != nullptr)
	{
		GetAbilityAttributes(2)->ManaCost.Initialize(25.0f);
		GetAbilityAttributes(2)->CooldownTime.Initialize(3.0f);
		GetAbilityAttributes(2)->AbilityDamage.Initialize(50.0f);
	}
}

void ATestTool::ActivateAbility(int32 AbilityIndex)
{
	UToolAttributeSet* Attributes = GetAbilityAttributes(AbilityIndex);

	if (Attributes == nullptr)
	{
		return;
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Activated %s ability. Mana cost: %.2f, damage: %.2f"),
		*GetAbilityData(AbilityIndex).AbilityName.ToString(),
		Attributes->ManaCost.GetCurrentValue(),
		Attributes->AbilityDamage.GetCurrentValue());

	// Spawn the effect, projectile, trace, or gameplay effect here.
	// This class owns the ability-specific behavior.
}

void ATestTool::BeginPlay()
{
	Super::BeginPlay();
}

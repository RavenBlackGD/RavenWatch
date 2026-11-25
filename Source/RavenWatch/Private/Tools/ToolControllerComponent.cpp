// Fill out your copyright notice in the Description page of Project Settings.


#include "Tools/ToolControllerComponent.h"
#include "AttributeEffectTypes.h"
#include "AttributeSystem.h"
#include "AttributeEffectDefinitions.h"
#include "AttributeEffectComponent.h"
#include "BasePlayer.h"
#include "PlayerAttributeSet.h"

// Sets default values for this component's properties
UToolControllerComponent::UToolControllerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;


}

void UToolControllerComponent::EquipToolOne(APlayerToolBase* NewTool)
{
	if (!IsValid(NewTool))
	{
		return;
	}

	EquippedToolOne = NewTool;
}

bool UToolControllerComponent::UseToolOneAbility(int32 AbilityIndex)
{
	UE_LOG(LogTemp, Log, TEXT("UseToolOneAbility called with AbilityIndex: %d"), AbilityIndex);

	if (!IsValid(EquippedToolOne) ||
		(AbilityIndex != 1 && AbilityIndex != 2))
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid tool or ability index."));
		return false;
	}

	if (!IsValid(PlayerAttributeSystem) ||
		!IsValid(PlayerEffectComponent))
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerAttributeSystem or PlayerEffectComponent is not valid."));
		return false;
	}

	UToolAttributeSet* ToolAttributes =
		EquippedToolOne->GetAbilityAttributes(AbilityIndex);

	if (ToolAttributes == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Tool attributes not found for ability index: %d"), AbilityIndex);
		return false;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();

	const float* CooldownEndTime =
		AbilityCooldownEndTimes.Find(AbilityIndex);

	if (CooldownEndTime != nullptr &&
		CurrentTime < *CooldownEndTime)
	{
		UE_LOG(LogTemp, Warning, TEXT("Ability %d is on cooldown. Time remaining: %f seconds"), AbilityIndex, *CooldownEndTime - CurrentTime);
		return false;
	}

	const float ManaCost =
		FMath::Max(0.0f, ToolAttributes->ManaCost.GetCurrentValue());

	float CurrentMana = 0.0f;

	if (!PlayerAttributeSystem->GetAttributeValue(
		UPlayerAttributeSet::StaticClass(),
		GET_MEMBER_NAME_CHECKED(UPlayerAttributeSet, CurrentMana),
		CurrentMana))
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to retrieve current mana from PlayerAttributeSystem."));
		return false;
	}

	if (CurrentMana < ManaCost)
	{
		UE_LOG(LogTemp, Warning, TEXT("Not enough mana to activate ability %d. Current mana: %f, required: %f"), AbilityIndex, CurrentMana, ManaCost);
		return false;
	}


	FAttributeModifier ManaModifier;
	ManaModifier.AttributeSetClass = UPlayerAttributeSet::StaticClass();
	ManaModifier.AttributeName = GET_MEMBER_NAME_CHECKED(UPlayerAttributeSet, CurrentMana);
	ManaModifier.Operation = EAttributeModifierOperation::Add;
	ManaModifier.Magnitude = -ManaCost;

	if (!PlayerEffectComponent->ApplyInstantModifier(ManaModifier))
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to apply ManaCostEffect."));
		return false;
	}

	EquippedToolOne->ActivateAbility(AbilityIndex);

	UE_LOG(LogTemp, Log, TEXT("Tool ability %d activated. Mana cost: %f"), AbilityIndex, ManaCost);

	const float Cooldown =
		FMath::Max(
			0.0f,
			ToolAttributes->CooldownTime.GetCurrentValue());

	AbilityCooldownEndTimes.Add(
		AbilityIndex,
		CurrentTime + Cooldown);

	return true;
}

void UToolControllerComponent::BeginPlay()
{
	Super::BeginPlay();

	OwningPlayer = Cast<ABasePlayer>(GetOwner());

	if (!IsValid(OwningPlayer))
	{
		UE_LOG(LogTemp, Error, TEXT("ToolControllerComponent: Owning player is not valid."));
		return;
	}

	PlayerAttributeSystem = OwningPlayer->GetAttributeSystem();
	
	if (!IsValid(PlayerAttributeSystem))
	{
		UE_LOG(LogTemp, Error, TEXT("ToolControllerComponent: PlayerAttributeSystem is not valid."));
	}

	PlayerEffectComponent = OwningPlayer->GetAttributeEffectComponent();

	if (!IsValid(PlayerEffectComponent))
	{
		UE_LOG(LogTemp, Error, TEXT("ToolControllerComponent: PlayerEffectComponent is not valid."));
	}
}



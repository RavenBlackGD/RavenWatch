// Fill out your copyright notice in the Description page of Project Settings.


#include "AttributeEffectComponent.h"
#include "AttributeSystem.h"
#include "AttributeEffectDefinitions.h"
#include "AttributeSetBase.h"

// Sets default values for this component's properties
UAttributeEffectComponent::UAttributeEffectComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

}

bool UAttributeEffectComponent::ApplyEffect(UAttributeEffectDefinitions* EffectDefinition, FGuid& OutEffectHandle)
{
	OutEffectHandle.Invalidate();

	if (!EffectDefinition)
	{
		return false;
	}

	switch (EffectDefinition->DurationPolicy)
	{
	case EEffectDurationPolicy::Instant:
		return ApplyInstantEffect(EffectDefinition, OutEffectHandle);

	case EEffectDurationPolicy::Infinite:
		return ApplyPersistentEffect(EffectDefinition, OutEffectHandle, false);

	case EEffectDurationPolicy::HasDuration:
		return ApplyPersistentEffect(EffectDefinition, OutEffectHandle, true);

	default:
		return false;
	}
}

bool UAttributeEffectComponent::ApplyInstantModifier(TSubclassOf<UAttributeSetBase> AttributeSetClass, FName AttributeName, EAttributeModifierOperation Operation, float Magnitude)
{
	if (!AttributeSetClass || AttributeName.IsNone())
	{
		return false;
	}

	if (!AttributeSystem)
	{
		AttributeSystem = ResolveAttributeSystem();
	}

	if (!AttributeSystem)
	{
		return false;
	}

	if (!AttributeSystem->ExecuteInstantModifier(
		AttributeSetClass,
		AttributeName,
		Operation,
		Magnitude))
	{
		return false;
	}

	return true;
}

bool UAttributeEffectComponent::ApplyInstantModifier(FAttributeModifier Modifier)
{
	if (!Modifier.AttributeSetClass || Modifier.AttributeName.IsNone())
	{
		return false;
	}

	if (!AttributeSystem)
	{
		AttributeSystem = ResolveAttributeSystem();
	}

	if (!AttributeSystem)
	{
		return false;
	}

	if (!AttributeSystem->ExecuteInstantModifier(
		Modifier.AttributeSetClass,
		Modifier.AttributeName,
		Modifier.Operation,
		Modifier.Magnitude))
	{
		return false;
	}

	return true;
}

bool UAttributeEffectComponent::RemoveEffect(FGuid EffectHandle)
{
	if (!EffectHandle.IsValid())
	{
		return false;
	}

	const int32 ActiveEffectIndex =
		ActiveEffects.IndexOfByPredicate(
			[&EffectHandle](const FActiveAttributeEffect& Effect)
			{
				return Effect.Handle == EffectHandle;
			});

	if (ActiveEffectIndex == INDEX_NONE)
	{
		return false;
	}

	FActiveAttributeEffect& ActiveEffect =
		ActiveEffects[ActiveEffectIndex];

	if (!ActiveEffect.Definition || !AttributeSystem)
	{
		return false;
	}

	const TArray<FAttributeModifier>& Modifiers =
		ActiveEffect.Definition->Modifiers;

	// Validate all modifiers before changing any state.
	for (const FAttributeModifier& Modifier : Modifiers)
	{
		if (!Modifier.AttributeSetClass ||
			Modifier.AttributeName.IsNone())
		{
			return false;
		}
	}

	// Remove each unique attribute target once.
	for (int32 ModifierIndex = 0;
		ModifierIndex < Modifiers.Num();
		++ModifierIndex)
	{
		const FAttributeModifier& Modifier =
			Modifiers[ModifierIndex];

		bool bAlreadyProcessed = false;

		for (int32 PreviousIndex = 0;
			PreviousIndex < ModifierIndex;
			++PreviousIndex)
		{
			const FAttributeModifier& PreviousModifier =
				Modifiers[PreviousIndex];

			if (PreviousModifier.AttributeSetClass ==
					Modifier.AttributeSetClass &&
				PreviousModifier.AttributeName ==
					Modifier.AttributeName)
			{
				bAlreadyProcessed = true;
				break;
			}
		}

		if (bAlreadyProcessed)
		{
			continue;
		}

		if (!AttributeSystem->RemoveModifier(
			Modifier.AttributeSetClass,
			Modifier.AttributeName,
			EffectHandle))
		{
			return false;
		}
	}

	if (ActiveEffect.TimerHandle.IsValid())
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(
				ActiveEffect.TimerHandle);
		}
	}

	ActiveEffects.RemoveAt(ActiveEffectIndex);

	return true;
}


// Called when the game starts
void UAttributeEffectComponent::BeginPlay()
{
	Super::BeginPlay();

	AttributeSystem = ResolveAttributeSystem();
	
}

UAttributeSystem* UAttributeEffectComponent::ResolveAttributeSystem() const
{
	return GetOwner()
		? GetOwner()->FindComponentByClass<UAttributeSystem>()
		: nullptr;
}

bool UAttributeEffectComponent::ApplyInstantEffect(UAttributeEffectDefinitions* EffectDefinition, FGuid& OutEffectHandle)
{
	OutEffectHandle.Invalidate();

	if (!EffectDefinition ||
		EffectDefinition->DurationPolicy !=
		EEffectDurationPolicy::Instant)
	{
		return false;
	}

	if (!AttributeSystem)
	{
		AttributeSystem = ResolveAttributeSystem();
	}

	if (!AttributeSystem)
	{
		return false;
	}

	for (const FAttributeModifier& Modifier :
		EffectDefinition->Modifiers)
	{
		if (!Modifier.AttributeSetClass ||
			Modifier.AttributeName.IsNone())
		{
			return false;
		}

		if (!AttributeSystem->ExecuteInstantModifier(
			Modifier.AttributeSetClass,
			Modifier.AttributeName,
			Modifier.Operation,
			Modifier.Magnitude))
		{
			return false;
		}
	}

	return true;
}

bool UAttributeEffectComponent::ApplyPersistentEffect(UAttributeEffectDefinitions* EffectDefinition, FGuid& OutEffectHandle, bool bHasDuration)
{
	OutEffectHandle.Invalidate();

	if (!EffectDefinition ||
		(bHasDuration &&
			EffectDefinition->Duration <= 0.f))
	{
		return false;
	}

	if (!AttributeSystem)
	{
		AttributeSystem = ResolveAttributeSystem();
	}

	if (!AttributeSystem)
	{
		return false;
	}

	const FGuid EffectHandle = FGuid::NewGuid();
	TArray<FAttributeModifier> AppliedModifiers;

	for (const FAttributeModifier& Modifier :
		EffectDefinition->Modifiers)
	{
		if (!Modifier.AttributeSetClass ||
			Modifier.AttributeName.IsNone())
		{
			for (const FAttributeModifier& AppliedModifier :
				AppliedModifiers)
			{
				AttributeSystem->RemoveModifier(
					AppliedModifier.AttributeSetClass,
					AppliedModifier.AttributeName,
					EffectHandle);
			}

			return false;
		}

		if (!AttributeSystem->ApplyModifier(
			Modifier.AttributeSetClass,
			Modifier.AttributeName,
			EffectHandle,
			Modifier.Operation,
			Modifier.Magnitude))
		{
			// Roll back modifiers already applied by this effect.
			for (const FAttributeModifier& AppliedModifier :
				AppliedModifiers)
			{
				AttributeSystem->RemoveModifier(
					AppliedModifier.AttributeSetClass,
					AppliedModifier.AttributeName,
					EffectHandle);
			}

			return false;
		}

		AppliedModifiers.Add(Modifier);
	}

	FActiveAttributeEffect ActiveEffect;
	ActiveEffect.Handle = EffectHandle;
	ActiveEffect.Definition = EffectDefinition;

	if (bHasDuration)
	{
		UWorld* World = GetWorld();

		if (!World)
		{
			for (const FAttributeModifier& AppliedModifier :
				AppliedModifiers)
			{
				AttributeSystem->RemoveModifier(
					AppliedModifier.AttributeSetClass,
					AppliedModifier.AttributeName,
					EffectHandle);
			}

			return false;
		}

		FTimerDelegate ExpirationDelegate =
			FTimerDelegate::CreateUObject(
				this,
				&UAttributeEffectComponent::ExpireEffect,
				EffectHandle);

		World->GetTimerManager().SetTimer(
			ActiveEffect.TimerHandle,
			ExpirationDelegate,
			EffectDefinition->Duration,
			false);
	}

	ActiveEffects.Add(ActiveEffect);
	OutEffectHandle = EffectHandle;

	return true;
}

void UAttributeEffectComponent::ExpireEffect(FGuid EffectHandle)
{
	RemoveEffect(EffectHandle);
}



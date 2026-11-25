// Fill out your copyright notice in the Description page of Project Settings.


#include "AttributeSystem.h"
#include "AttributeSetBase.h"


// Sets default values for this component's properties
UAttributeSystem::UAttributeSystem()
{
	PrimaryComponentTick.bCanEverTick = false;
}


// Called when the game starts
void UAttributeSystem::BeginPlay()
{
	Super::BeginPlay();

	InitializeAttributeSets();
}

// Initializes the attribute sets based on the specified classes in AttributeSetClasses.
void UAttributeSystem::InitializeAttributeSets()
{
	for (const TSubclassOf<UAttributeSetBase>& AttributeSetClass :
		AttributeSetClasses)
	{
		AddAttributeSet(AttributeSetClass);
	}
}

// Adds an attribute set of the specified class to the system. Returns true if added successfully, false if the class is invalid or already exists.
bool UAttributeSystem::AddAttributeSet(
	TSubclassOf<UAttributeSetBase> AttributeSetClass)
{
	if (!AttributeSetClass || HasAttributeSet(AttributeSetClass))
	{
		return false;
	}

	UAttributeSetBase* AttributeSet =
		NewObject<UAttributeSetBase>(this, AttributeSetClass);

	if (!AttributeSet)
	{
		return false;
	}

	AttributeSets.Add(AttributeSet);
	return true;
}

// Checks if the attribute set of the specified class exists in the system.
bool UAttributeSystem::HasAttributeSet(
	TSubclassOf<UAttributeSetBase> AttributeSetClass) const
{
	return GetAttributeSet(AttributeSetClass) != nullptr;
}

// Retrieves the attribute set of the specified class. Returns nullptr if not found.
UAttributeSetBase* UAttributeSystem::GetAttributeSet(
	TSubclassOf<UAttributeSetBase> AttributeSetClass) const
{
	if (!AttributeSetClass)
	{
		return nullptr;
	}

	for (UAttributeSetBase* AttributeSet : AttributeSets)
	{
		if (AttributeSet && AttributeSet->GetClass() == AttributeSetClass)
		{
			return AttributeSet;
		}
	}

	return nullptr;
}

// Retrieves the attribute from the specified attribute set class and attribute name. 
// Returns true if found and outputs the attribute through OutAttribute, false otherwise.
bool UAttributeSystem::FindAttribute(
	TSubclassOf<UAttributeSetBase> AttributeSetClass,
	FName AttributeName,
	FAttribute*& OutAttribute)
{
	OutAttribute = nullptr;

	UAttributeSetBase* AttributeSet =
		GetAttributeSet(AttributeSetClass);

	return AttributeSet &&
		AttributeSet->FindAttribute(AttributeName, OutAttribute);
}

// Const version of FindAttribute
bool UAttributeSystem::FindAttribute(
	TSubclassOf<UAttributeSetBase> AttributeSetClass,
	FName AttributeName,
	const FAttribute*& OutAttribute) const
{
	OutAttribute = nullptr;

	const UAttributeSetBase* AttributeSet =
		GetAttributeSet(AttributeSetClass);

	return AttributeSet &&
		AttributeSet->FindAttribute(AttributeName, OutAttribute);
}

// Applies a modifier to an attribute based on the effect handle.
bool UAttributeSystem::ApplyModifier(TSubclassOf<UAttributeSetBase> AttributeSetClass, FName AttributeName, FGuid EffectHandle, EAttributeModifierOperation Operation, float Magnitude)
{
	// Validate input parameters
	if (!AttributeSetClass || AttributeName.IsNone() || !EffectHandle.IsValid())
	{
		return false;
	}

	// validate the operation is valid
	switch (Operation)
	{
	case EAttributeModifierOperation::Add:
	case EAttributeModifierOperation::Multiply:
	case EAttributeModifierOperation::Override:
		break;
	default:
		return false;
	}

	// Get the attribute set and attribute
	UAttributeSetBase* AttributeSet = GetAttributeSet(AttributeSetClass);

	if (!AttributeSet)
	{
		return false;
	}

	FAttribute* Attribute = nullptr;

	if (!FindAttribute(AttributeSetClass, AttributeName, Attribute))
	{
		return false;
	}

	// Create the modifier
	FAttributeModifier Modifier(Operation, Magnitude);

	// Store original modifiers to restore in case of failure
	TArray<FStoredAttributeModifier> OriginalModifiers;
	Attribute->CopyModifiers(OriginalModifiers);

	// Capture the old value before applying the modifier
	const float OldValue = Attribute->GetCurrentValue();

	// Create a variable to hold the new value after applying the modifier
	float NewValue = OldValue;

	// Apply the modifier to the attribute
	Attribute->AddModifier(Modifier, EffectHandle);

	// Recalculate the attribute value after applying the modifier
	Attribute->CalculateModifiedValue(NewValue);
	
	// Run the pre-change logic
	if (!AttributeSet->PreAttributeChange(AttributeName, NewValue))
	{
		// Restore the original modifiers if pre-change logic fails
		Attribute->SetModifiers(OriginalModifiers);
		return false;
	}

	// Update the attribute value with the new value
	Attribute->SetCurrentValue(NewValue);

	// If the value has changed, run post-change logic and broadcast the change
	if (!FMath::IsNearlyEqual(OldValue, NewValue))
	{
		AttributeSet->PostAttributeChange(AttributeName, OldValue, NewValue);
		OnAttributeChanged.Broadcast(AttributeSet, AttributeName, OldValue, NewValue);
	}

	return true;

}

// Removes a modifier from an attribute based on the effect handle. 
// Returns false if the attribute set or attribute is not found, or if no modifiers were removed.
bool UAttributeSystem::RemoveModifier(TSubclassOf<UAttributeSetBase> AttributeSetClass, FName AttributeName, FGuid EffectHandle)
{
	// Validate input parameters
	if (!AttributeSetClass || AttributeName.IsNone() || !EffectHandle.IsValid())
	{
		return false;
	}
	// Get the attribute set and attribute
	UAttributeSetBase* AttributeSet = GetAttributeSet(AttributeSetClass);
	if (!AttributeSet)
	{
		return false;
	}
	FAttribute* Attribute = nullptr;
	if (!FindAttribute(AttributeSetClass, AttributeName, Attribute))
	{
		return false;
	}

	// Store original modifiers to restore in case of failure
	TArray<FStoredAttributeModifier> OriginalModifiers;
	Attribute->CopyModifiers(OriginalModifiers);

	// Store removed modifiers to recalculate the value after removal
	TArray<FStoredAttributeModifier> RemovedModifiers;

	// Capture the old value before removing the modifier
	const float OldValue = Attribute->GetCurrentValue();

	if (!Attribute->RemoveModifiers(EffectHandle, RemovedModifiers))
	{
		return false; // No modifiers were removed
	}

	// Recalculate the attribute value after removing the modifier
	float NewValue = OldValue;
	Attribute->CalculateModifiedValue(NewValue);

	// Run the pre-change logic
	if (!AttributeSet->PreAttributeChange(AttributeName, NewValue))
	{
		Attribute->SetModifiers(OriginalModifiers); // Restore the removed modifiers
		return false; // Pre-change logic failed
	}
	// Update the attribute value with the new value
	Attribute->SetCurrentValue(NewValue);
	// If the value has changed, run post-change logic and broadcast the change
	if (!FMath::IsNearlyEqual(OldValue, NewValue))
	{
		AttributeSet->PostAttributeChange(AttributeName, OldValue, NewValue);
		OnAttributeChanged.Broadcast(AttributeSet, AttributeName, OldValue, NewValue);
	}
	return true;
}

bool UAttributeSystem::RemoveModifiersByGuid(FGuid EffectHandle)
{
	if (!EffectHandle.IsValid())
	{
		return false;
	}

	bool bRemovedAnyModifier = false;

	for (UAttributeSetBase* AttributeSet : AttributeSets)
	{
		if (!AttributeSet)
		{
			continue;
		}

		for (TFieldIterator<FProperty> PropertyIterator(
			AttributeSet->GetClass(),
			EFieldIteratorFlags::IncludeSuper);
			PropertyIterator;
			++PropertyIterator)
		{
			FStructProperty* StructProperty =
				CastField<FStructProperty>(*PropertyIterator);

			if (!StructProperty ||
				StructProperty->Struct != FAttribute::StaticStruct())
			{
				continue;
			}

			const bool bRemoved =
				RemoveModifier(
					AttributeSet->GetClass(),
					StructProperty->GetFName(),
					EffectHandle);

			bRemovedAnyModifier |= bRemoved;
		}
	}

	return bRemovedAnyModifier;
}

// Applies an instant modifier to an attribute, changing its base value directly.
// Returns false if the attribute set or attribute is not found.
bool UAttributeSystem::ExecuteInstantModifier(TSubclassOf<UAttributeSetBase> AttributeSetClass, FName AttributeName, EAttributeModifierOperation Operation, float Magnitude)
{
	if (!AttributeSetClass || AttributeName.IsNone())
	{
		return false;
	}

	FAttribute* Attribute = nullptr;

	if (!FindAttribute(AttributeSetClass, AttributeName, Attribute))
	{
		return false;
	}

	float NewBaseValue = Attribute->GetBaseValue();

	switch (Operation)
	{
	case EAttributeModifierOperation::Add:
		NewBaseValue += Magnitude;
		break;

	case EAttributeModifierOperation::Multiply:
		NewBaseValue *= 1.f + Magnitude / 100.f;
		break;

	case EAttributeModifierOperation::Override:
		NewBaseValue = Magnitude;
		break;

	default:
		return false;
	}

	return SetBaseAttributeValue(
		AttributeSetClass,
		AttributeName,
		NewBaseValue);
}


// Get the current value of an attribute. Returns false if the attribute set or attribute is not found.
bool UAttributeSystem::GetAttributeValue(
	TSubclassOf<UAttributeSetBase> AttributeSetClass,
	FName AttributeName,
	float& OutValue) const
{
	const FAttribute* Attribute = nullptr;

	if (!FindAttribute(AttributeSetClass, AttributeName, Attribute))
	{
		return false;
	}

	OutValue = Attribute->GetCurrentValue();
	return true;
}

bool UAttributeSystem::GetBaseAttributeValue(TSubclassOf<UAttributeSetBase> AttributeSetClass, FName AttributeName, float& OutValue) const
{
	const FAttribute* Attribute = nullptr;

	if (!FindAttribute(AttributeSetClass, AttributeName, Attribute))
	{
		return false;
	}

	OutValue = Attribute->GetBaseValue();
	return true;
}

// This function changes the current value directly, bypassing any modifiers.
// It is intended for use cases where you want to set the current value without affecting the base value or modifiers.
// Use with caution, as adding or removing modifiers will likely override this value. 
// Effects like damage or healing should be an instant modifier instead of directly setting the current value.
bool UAttributeSystem::SetCurrentAttributeValue(
	TSubclassOf<UAttributeSetBase> AttributeSetClass,
	FName AttributeName,
	float NewValue)
{
	FAttribute* Attribute = nullptr;

	if (!FindAttribute(AttributeSetClass, AttributeName, Attribute))
	{
		return false;
	}

	UAttributeSetBase* AttributeSet =
		GetAttributeSet(AttributeSetClass);

	const float OldValue = Attribute->GetCurrentValue();

	if (!AttributeSet->PreAttributeChange(AttributeName, NewValue))
	{
		return false;
	}

	if (FMath::IsNearlyEqual(OldValue, NewValue))
	{
		return true;
	}

	Attribute->SetCurrentValue(NewValue);

	AttributeSet->PostAttributeChange(
		AttributeName,
		OldValue,
		NewValue);

	OnAttributeChanged.Broadcast(
		AttributeSet,
		AttributeName,
		OldValue,
		NewValue);

	return true;
}


// Sets the base value of an attribute and recalculates the current value based on the new base value.
// For delegate broadcasting, current value changes are sent first, followed by base value changes.
bool UAttributeSystem::SetBaseAttributeValue(
	TSubclassOf<UAttributeSetBase> AttributeSetClass,
	FName AttributeName,
	float NewValue)
{
	FAttribute* Attribute = nullptr;

	if (!FindAttribute(AttributeSetClass, AttributeName, Attribute))
	{
		return false;
	}

	UAttributeSetBase* AttributeSet =
		GetAttributeSet(AttributeSetClass);

	if (!AttributeSet)
	{
		return false;
	}

	const float OldBaseValue = Attribute->GetBaseValue();
	const float OldCurrentValue = Attribute->GetCurrentValue();

	float NewCurrentValue = OldCurrentValue;

	if (!AttributeSet->PreAttributeBaseChange(
		AttributeName,
		NewValue))
	{
		return false;
	}

	if (FMath::IsNearlyEqual(OldBaseValue, NewValue))
	{
		return true;
	}

	// Update the base value of the attribute and recalculate the current value
	Attribute->SetBaseValue(NewValue);

	// Recalculate the current value based on the new base value
	Attribute->CalculateModifiedValue(NewCurrentValue);

	// pre-change logic for the current value change
	if (!AttributeSet->PreAttributeChange(
		AttributeName,
		NewCurrentValue))
	{
		// If pre-change logic fails, revert the base value and recalculate
		Attribute->SetBaseValue(OldBaseValue);
		Attribute->CalculateModifiedValue(NewCurrentValue);
		return false;
	}

	// check if the current value has changed after recalculation
	if (!FMath::IsNearlyEqual(Attribute->GetCurrentValue(), NewCurrentValue))
	{
		Attribute->SetCurrentValue(NewCurrentValue);
		// Call PostAttributeChange for the current value change
		AttributeSet->PostAttributeChange(
			AttributeName,
			OldCurrentValue,
			NewCurrentValue);
		OnAttributeChanged.Broadcast(
			AttributeSet,
			AttributeName,
			OldCurrentValue,
			NewCurrentValue);
	}

	// Call PostAttributeBaseChange for the base value change
	AttributeSet->PostAttributeBaseChange(
		AttributeName,
		OldBaseValue,
		NewValue);

	// Broadcast the base value change event
	OnAttributeBaseChanged.Broadcast(
		AttributeSet,
		AttributeName,
		OldBaseValue,
		NewValue);

	return true;
}


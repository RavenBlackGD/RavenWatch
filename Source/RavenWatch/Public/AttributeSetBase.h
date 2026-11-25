// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "UObject/UnrealType.h"
#include "AttributeEffectTypes.h"
#include "AttributeSetBase.generated.h"

/**
 * 
 */


// Effect handle used to keep track of the effect on individual actors correctly as the attribute modifier struct can be shared between actors.
USTRUCT()
struct RAVENWATCH_API FStoredAttributeModifier
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid EffectHandle;

	UPROPERTY()
	FAttributeModifier Modifier;
};

USTRUCT(BlueprintType)
struct FAttributeValue
{
	GENERATED_BODY()

	FAttributeValue()
		: BaseValue(0.f)
		, CurrentValue(0.f)
	{
	}

	explicit FAttributeValue(float Value)
		: BaseValue(Value)
		, CurrentValue(Value)
	{
	}

	float GetCurrentValue() const
	{
		return CurrentValue;
	}

	void SetCurrentValue(float NewValue)
	{
		CurrentValue = NewValue;
	}

	float GetBaseValue() const
	{
		return BaseValue;
	}

	void SetBaseValue(float NewValue)
	{
		BaseValue = NewValue;
	}

	// Not necessary to reset to base value as the current value is recalculated every time a modifier is added or removed.
	//void ResetToBase()
	//{
	//	CurrentValue = BaseValue;
	//}

	void Initialize(float InBaseValue)
	{
		BaseValue = InBaseValue;
		CurrentValue = InBaseValue;
	}

	void AddModifier(
		const FAttributeModifier& InModifier,
		const FGuid& InEffectHandle)
	{
		FStoredAttributeModifier StoredModifier;
		StoredModifier.EffectHandle = InEffectHandle;
		StoredModifier.Modifier = InModifier;

		ModifierHandles.Add(StoredModifier);
	}


	bool RemoveModifiers(
		const FGuid& EffectHandle,
		TArray<FStoredAttributeModifier>& OutRemovedModifiers)
	{
		OutRemovedModifiers.Reset();

		for (int32 Index = 0; Index < ModifierHandles.Num();)
		{
			if (ModifierHandles[Index].EffectHandle == EffectHandle)
			{
				OutRemovedModifiers.Add(ModifierHandles[Index]);
				ModifierHandles.RemoveAt(Index);
			}
			else
			{
				++Index;
			}
		}

		return OutRemovedModifiers.Num() > 0;
	}


	void SetModifiers(
		const TArray<FStoredAttributeModifier>& InModifiers)
	{
		ModifierHandles = InModifiers;
	}

	void CopyModifiers(
		TArray<FStoredAttributeModifier>& OutModifiers) const
	{
		OutModifiers = ModifierHandles;
	}



	// Recalculate order of operations should be as follows:
	// 1. Start with the base value.
	// 2. Apply all additive modifiers.
	// 3. Apply all multiplicative modifiers. Multiplicative modifiers should be applied as a percentage and not a float multiplier.
	//		For example, a 20% increase should be applied as 20 and not 1.2 and a 20% decrease should be applied as -20 and not 0.8.
	// 4. Apply the last override modifier, if any.
	// 5. Clamp the final value to a valid range (if applicable).

	void CalculateModifiedValue(float& OutNewValue)
	{
		float NewValue = BaseValue;
		float AdditiveValue = 0.f;
		float MultiplicativeValue = 1.f;
		bool bHasOverride = false;
		float OverrideValue = NewValue;

		for (const FStoredAttributeModifier& ModifierHandle : ModifierHandles)
		{
			const FAttributeModifier& Modifier = ModifierHandle.Modifier;
			switch (Modifier.Operation)
			{
			case EAttributeModifierOperation::Add:
				AdditiveValue += Modifier.Magnitude;
				break;
			case EAttributeModifierOperation::Multiply:
				MultiplicativeValue *= 1.f + Modifier.Magnitude / 100.f;
				break;
			case EAttributeModifierOperation::Override:
				bHasOverride = true;
				OverrideValue = Modifier.Magnitude;
				break;
			default:
				break;
			}
		}

		NewValue += AdditiveValue;
		NewValue *= MultiplicativeValue;

		if (bHasOverride)
		{
			NewValue = OverrideValue;
		}
		
		OutNewValue = NewValue;
	}


protected:

	UPROPERTY(BlueprintReadOnly, Category = "Attribute")
	float BaseValue = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Attribute")
	float CurrentValue = 0.f;

private:
	UPROPERTY()
	TArray<FStoredAttributeModifier> ModifierHandles;

};


USTRUCT(BlueprintType)
struct FAttribute
{
	GENERATED_BODY()

	FAttribute()
		: AttributeName(NAME_None)
		, AttributeValue()
		, bPersistent(true)
	{
	}

	explicit FAttribute(FName InAttributeName)
		: AttributeName(InAttributeName)
		, AttributeValue()
		, bPersistent(true)
	{
	}

	FAttribute(FName InAttributeName, FAttributeValue InAttributeValue)
		: AttributeName(InAttributeName)
		, AttributeValue(InAttributeValue)
		, bPersistent(true)
	{
	}

	explicit FAttribute(FName InAttributeName, bool bInPersistent)
		: AttributeName(InAttributeName)
		, AttributeValue()
		, bPersistent(bInPersistent)
	{
	}

	FAttribute(FName InAttributeName, FAttributeValue InAttributeValue, bool bInPersistent)
		: AttributeName(InAttributeName)
		, AttributeValue(InAttributeValue)
		, bPersistent(bInPersistent)
	{
	}

	bool IsValid() const
	{
		return !AttributeName.IsNone();
	}

	bool IsPersistent() const
	{
		return bPersistent;
	}

	void SetPersistent(bool bInPersistent)
	{
		bPersistent = bInPersistent;
	}

	FName GetName() const
	{
		return AttributeName;
	}

	float GetCurrentValue() const
	{
		return AttributeValue.GetCurrentValue();
	}

	float GetBaseValue() const
	{
		return AttributeValue.GetBaseValue();
	}

	void SetName(FName InAttributeName)
	{
		AttributeName = InAttributeName;
	}

	void SetCurrentValue(float NewValue)
	{
		AttributeValue.SetCurrentValue(NewValue);
	}

	void SetBaseValue(float NewValue)
	{
		AttributeValue.SetBaseValue(NewValue);
	}

	void Initialize(float InBaseValue)
	{
		AttributeValue.Initialize(InBaseValue);
	}

	//void ResetToBase()
	//{
	//	AttributeValue.ResetToBase();
	//}

	void AddModifier(
		const FAttributeModifier& InModifier,
		const FGuid& InEffectHandle)
	{
		AttributeValue.AddModifier(InModifier, InEffectHandle);
	}

	bool RemoveModifiers(
		const FGuid& EffectHandle,
		TArray<FStoredAttributeModifier>& OutRemovedModifiers)
	{
		return AttributeValue.RemoveModifiers(
			EffectHandle,
			OutRemovedModifiers);
	}

	void SetModifiers(
		const TArray<FStoredAttributeModifier>& InModifiers)
	{
		AttributeValue.SetModifiers(InModifiers);
	}

	void CopyModifiers(
		TArray<FStoredAttributeModifier>& OutModifiers) const
	{
		AttributeValue.CopyModifiers(OutModifiers);
	}

	void CalculateModifiedValue(float& OutNewValue)
	{
		AttributeValue.CalculateModifiedValue(OutNewValue);
	}

private:
	UPROPERTY(EditAnywhere, Category = "Attribute")
	FName AttributeName;

	UPROPERTY(EditAnywhere, Category = "Attribute")
	FAttributeValue AttributeValue;

	UPROPERTY(EditAnywhere, Category = "Attribute")
	bool bPersistent;
};

UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class RAVENWATCH_API UAttributeSetBase : public UObject
{
	GENERATED_BODY()

public:

	virtual void PostInitProperties() override;

	bool FindAttribute(
		FName AttributeName,
		FAttribute*& OutAttribute);

	bool FindAttribute(
		FName AttributeName,
		const FAttribute*& OutAttribute) const;

	virtual bool PreAttributeChange(const FName& AttributeName, float& NewValue) 
	{
		return true;
	}

	virtual void PostAttributeChange(const FName& AttributeName, float OldValue, float NewValue) 
	{
	}
	
	virtual bool PreAttributeBaseChange(
		const FName& AttributeName,
		float& NewValue)
	{
		return true;
	}

	virtual void PostAttributeBaseChange(
		const FName& AttributeName,
		float OldValue,
		float NewValue)
	{
	}

};

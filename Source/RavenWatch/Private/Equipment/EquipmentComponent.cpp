// Fill out your copyright notice in the Description page of Project Settings.


#include "Equipment/EquipmentComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "AttributeSystem.h"
#include "AttributeSetBase.h"
#include "CombatAttributeSet.h"
#include "PlayerAttributeSet.h"
#include "AttributeEffectTypes.h"

namespace
{
	struct FEquipmentAttributeTarget
	{
		TSubclassOf<UAttributeSetBase> SetClass;
		FName AttributeName;
	};

	FEquipmentAttributeTarget GetAttributeTarget(EEquipmentStat Stat)
	{
		switch (Stat)
		{
		case EEquipmentStat::Strength:
			return{ UPlayerAttributeSet::StaticClass(), TEXT("Strength") };

		case EEquipmentStat::Dexterity :
			return { UPlayerAttributeSet::StaticClass(), TEXT("Dexterity") };

		case EEquipmentStat::Intelligence:
			return { UPlayerAttributeSet::StaticClass(), TEXT("Intelligence") };

		case EEquipmentStat::MaxMana:
			return { UPlayerAttributeSet::StaticClass(), TEXT("MaxMana") };

		case EEquipmentStat::MaxHealth:
			return { UCombatAttributeSet::StaticClass(), TEXT("MaxHealth") };

		case EEquipmentStat::Armor:
			return { UCombatAttributeSet::StaticClass(), TEXT("Armor") };

		case EEquipmentStat::MovementSpeed:
			return { UCombatAttributeSet::StaticClass(), TEXT("MovementSpeed") };

		case EEquipmentStat::AttackDamage:
			return { UCombatAttributeSet::StaticClass(), TEXT("DamageAmount") };

		case EEquipmentStat::AttackSpeed:
			return { UCombatAttributeSet::StaticClass(), TEXT("AttackSpeed") };

		case EEquipmentStat::CriticalChance:
			return { UCombatAttributeSet::StaticClass(), TEXT("CriticalChance") };

		case EEquipmentStat::CriticalDamage:
			return { UCombatAttributeSet::StaticClass(), TEXT("CriticalDamage") };

		case EEquipmentStat::FlatFireDamage:
			return { UCombatAttributeSet::StaticClass(), TEXT("FlatFireDamage") };

		case EEquipmentStat::FlatColdDamage:
			return { UCombatAttributeSet::StaticClass(), TEXT("FlatColdDamage") };

		case EEquipmentStat::FlatLightningDamage:
			return { UCombatAttributeSet::StaticClass(), TEXT("FlatLightningDamage") };

		case EEquipmentStat::IncreasedFireDamage:
			return { UCombatAttributeSet::StaticClass(), TEXT("IncreasedFireDamage") };

		case EEquipmentStat::IncreasedColdDamage:
			return { UCombatAttributeSet::StaticClass(), TEXT("IncreasedColdDamage") };

		case EEquipmentStat::IncreasedLightningDamage:
			return { UCombatAttributeSet::StaticClass(), TEXT("IncreasedLightningDamage") };

		default:
			return { nullptr, NAME_None };
		}
	}
}

// Sets default values for this component's properties
UEquipmentComponent::UEquipmentComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}

bool UEquipmentComponent::EquipItem(const FEquipmentInstance& Item)
{
	FGuid NewHandle;

	if (!ApplyItemStats(Item, NewHandle))
	{
		return false;
	}

	for (FEquipmentInstance& EquippedItem : EquippedItems)
	{
		if (EquippedItem.Slot == Item.Slot)
		{
			const FGuid OldHandle = EquipmentModifierHandles.FindRef(Item.Slot);

			if (!RemoveItemStats(EquippedItem, OldHandle))
			{
				RemoveItemStats(Item, NewHandle);
				return false;
			}

			EquippedItem = Item;
			EquipmentModifierHandles.Add(Item.Slot, NewHandle);

			DebugPrintEquipment();
			return true;
		}
	}

	EquippedItems.Add(Item);
	EquipmentModifierHandles.Add(Item.Slot, NewHandle);

	DebugPrintEquipment();
	return true;
}

bool UEquipmentComponent::UnequipItem(EEquipmentSlot Slot)
{
	for (int32 Index = 0; Index < EquippedItems.Num(); ++Index)
	{
		if (EquippedItems[Index].Slot == Slot)
		{
			const FGuid Handle = EquipmentModifierHandles.FindRef(Slot);

			if (!RemoveItemStats(EquippedItems[Index], Handle))
			{
				return false;
			}

			EquippedItems.RemoveAt(Index);
			EquipmentModifierHandles.Remove(Slot);

			DebugPrintEquipment();
			return true;
		}
	}

	return false;
}

bool UEquipmentComponent::GetEquippedItem(EEquipmentSlot Slot, FEquipmentInstance& Item) const
{
	for (const FEquipmentInstance& EquippedItem : EquippedItems)
	{
		if (EquippedItem.Slot == Slot)
		{
			Item = EquippedItem;
			return true;
		}
	}

	Item = FEquipmentInstance();
	return false;
}

bool UEquipmentComponent::SetEquippedItems(const TArray<FEquipmentInstance>& Items)
{
	TSet<EEquipmentSlot> RequestedSlots;

	for (const FEquipmentInstance& Item : Items)
	{
		if (!ValidateItem(Item))
		{
			return false;
		}

		if (RequestedSlots.Contains(Item.Slot))
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("Equipment: Multiple items were supplied for slot %d."),
				static_cast<int32>(Item.Slot));

			return false;
		}

		RequestedSlots.Add(Item.Slot);
	}

	const TArray<FEquipmentInstance> PreviousItems = EquippedItems;

	// Remove modifiers from the currently equipped items first.
	for (const FEquipmentInstance& EquippedItem : EquippedItems)
	{
		const FGuid* Handle = EquipmentModifierHandles.Find(EquippedItem.Slot);

		if (Handle == nullptr || !Handle->IsValid())
		{
			UE_LOG(
				LogTemp,
				Error,
				TEXT("Equipment: Missing modifier handle for item %s."),
				*EquippedItem.ItemName.ToString());

			return false;
		}

		if (!RemoveItemStats(EquippedItem, *Handle))
		{
			UE_LOG(
				LogTemp,
				Error,
				TEXT("Equipment: Failed to remove modifiers from item %s."),
				*EquippedItem.ItemName.ToString());

			return false;
		}
	}

	EquippedItems.Reset();
	EquipmentModifierHandles.Reset();

	// Apply the replacement set.
	for (const FEquipmentInstance& Item : Items)
	{
		FGuid NewHandle;

		if (!ApplyItemStats(Item, NewHandle))
		{
			UE_LOG(
				LogTemp,
				Error,
				TEXT("Equipment: Failed to apply item %s. Restoring previous equipment."),
				*Item.ItemName.ToString());

			// Remove any replacement items that were already applied.
			for (const FEquipmentInstance& AppliedItem : EquippedItems)
			{
				const FGuid* AppliedHandle = EquipmentModifierHandles.Find(AppliedItem.Slot);

				if (AppliedHandle != nullptr && AppliedHandle->IsValid())
				{
					RemoveItemStats(AppliedItem, *AppliedHandle);
				}
			}

			EquippedItems.Reset();
			EquipmentModifierHandles.Reset();

			// Restore the previous equipment set.
			for (const FEquipmentInstance& PreviousItem : PreviousItems)
			{
				FGuid RestoredHandle;

				if (!ApplyItemStats(PreviousItem, RestoredHandle))
				{
					UE_LOG(
						LogTemp,
						Error,
						TEXT("Equipment: Failed to restore item %s."),
						*PreviousItem.ItemName.ToString());

					continue;
				}

				EquippedItems.Add(PreviousItem);
				EquipmentModifierHandles.Add(PreviousItem.Slot, RestoredHandle);
			}

			DebugPrintEquipment();
			return false;
		}

		EquippedItems.Add(Item);
		EquipmentModifierHandles.Add(Item.Slot, NewHandle);
	}

	DebugPrintEquipment();
	return true;
}



float UEquipmentComponent::GetTotalEquipmentBonus(EEquipmentStat Stat) const
{
	float Total = 0.0f;

	for (const FEquipmentInstance& Item : EquippedItems)
	{
		for (const FEquipmentModifier& Modifier : Item.Modifiers)
		{
			if (Modifier.Stat == Stat)
			{
				Total += Modifier.Value;
			}
		}
	}

	return Total;
}

TArray<FEquipmentInstance> UEquipmentComponent::GetAllEquippedItems() const
{
	return EquippedItems;
}

void UEquipmentComponent::DebugPrintEquipment() const
{
	FString Message = TEXT("=== PLAYER EQUIPMENT ===\n");

	const TArray<EEquipmentSlot> Slots = {
		EEquipmentSlot::Head,
		EEquipmentSlot::Chest,
		EEquipmentSlot::Legs,
		EEquipmentSlot::Hands,
		EEquipmentSlot::Feet,
		EEquipmentSlot::Weapon
	};

	for (EEquipmentSlot Slot : Slots)
	{
		const FString SlotName = StaticEnum<EEquipmentSlot>()->GetNameStringByValue(static_cast<int64> (Slot));

		FEquipmentInstance Item;

		if (GetEquippedItem(Slot, Item))
		{
			Message += FString::Printf(TEXT("\n%s: %s\n"), *SlotName, *Item.ItemName.ToString());

			for (const FEquipmentModifier& Modifier : Item.Modifiers)
			{
				const FString StatName = StaticEnum<EEquipmentStat>()->GetNameStringByValue(static_cast<int64>(Modifier.Stat));

				const FString AffixName = StaticEnum<EAffixType>()->GetNameStringByValue(static_cast<int64>(Modifier.AffixType));

				Message += FString::Printf(TEXT("  [%s] +%.1f %s\n"), *AffixName, Modifier.Value, *StatName);
			}
		}
		else
		{
			Message += FString::Printf(TEXT("\n%s: EMPTY\n"), *SlotName);
		}
	}
	
	UKismetSystemLibrary::PrintString(this, Message, true, true, FLinearColor::Green, 12.0f);
}

UAttributeSystem* UEquipmentComponent::GetAttributeSystem() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<UAttributeSystem>() : nullptr;
}

bool UEquipmentComponent::ValidateItem(const FEquipmentInstance& Item) const
{
	if (Item.ItemName.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("Equipment: Item has no name."));
		return false;
	}

	UAttributeSystem* System = GetAttributeSystem();
	if (System == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Equipment: Attribute system was not found."));
		return false;
	}

	for (const FEquipmentModifier& Modifier : Item.Modifiers)
	{
		if (!FMath::IsFinite(Modifier.Value))
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("Equipment: Item %s contains a non-finite modifier."),
				*Item.ItemName.ToString());

			return false;
		}

		const FEquipmentAttributeTarget Target = GetAttributeTarget(Modifier.Stat);

		if (!Target.SetClass || Target.AttributeName.IsNone())
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("Equipment: Item %s contains an invalid stat."),
				*Item.ItemName.ToString());

			return false;
		}

		FAttribute* Attribute = nullptr;
		if (!System->FindAttribute(Target.SetClass, Target.AttributeName, Attribute))
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("Equipment: Attribute %s was not found for item %s."),
				*Target.AttributeName.ToString(),
				*Item.ItemName.ToString());

			return false;
		}
	}

	return true;
}

bool UEquipmentComponent::ApplyItemStats(const FEquipmentInstance& Item, FGuid& OutHandle)
{
	OutHandle.Invalidate();

	UAttributeSystem* System = GetAttributeSystem();

	if (!System)
	{
		return false;
	}

	for (const FEquipmentModifier& Modifier : Item.Modifiers)
	{
		const FEquipmentAttributeTarget Target = GetAttributeTarget(Modifier.Stat);

		FAttribute* Attribute = nullptr;

		if (!Target.SetClass || !System->FindAttribute(Target.SetClass, Target.AttributeName, Attribute))
		{
			UE_LOG(LogTemp, Error, TEXT("Equipment: Could not find attribute %s"), *Target.AttributeName.ToString());

			return false;
		}
	}

	const FGuid Handle = FGuid::NewGuid();

	TArray<FEquipmentAttributeTarget> AppliedTargets;

	for (const FEquipmentModifier& Modifier : Item.Modifiers)
	{
		const FEquipmentAttributeTarget Target = GetAttributeTarget(Modifier.Stat);

		if (!System->ApplyModifier(Target.SetClass, Target.AttributeName, Handle, EAttributeModifierOperation::Add, Modifier.Value))
		{
			for (const FEquipmentAttributeTarget& Applied : AppliedTargets)
			{
				System->RemoveModifier(Applied.SetClass, Applied.AttributeName, Handle);
			}

			return false;
		}

		if (!AppliedTargets.ContainsByPredicate([&Target](const FEquipmentAttributeTarget& Existing)
			{
				return Existing.SetClass == Target.SetClass && Existing.AttributeName == Target.AttributeName;
			}))
		{
			AppliedTargets.Add(Target);
		}
	}

	OutHandle = Handle;
	return true;
}

bool UEquipmentComponent::RemoveItemStats(const FEquipmentInstance& Item, const FGuid& Handle)
{
	UAttributeSystem* System = GetAttributeSystem();

	if (!System || !Handle.IsValid())
	{
		return false;
	}

	TArray<FName> RemovedNames;

	for (const FEquipmentModifier& Modifier : Item.Modifiers)
	{
		const FEquipmentAttributeTarget Target = GetAttributeTarget(Modifier.Stat);

		if (!Target.SetClass || Target.AttributeName.IsNone())
		{
			return false;
		}

		if (RemovedNames.Contains(Target.AttributeName))
		{
			continue;
		}

		if (!System->RemoveModifier(Target.SetClass, Target.AttributeName, Handle))
		{
			return false;
		}

		RemovedNames.Add(Target.AttributeName);
	}

	return true;
}

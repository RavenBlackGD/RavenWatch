// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryComponent.h"
#include "Equipment/EquipmentComponent.h"
#include "AttributeSystem.h"
#include "CombatAttributeSet.h"
#include "PlayerAttributeSet.h"

// Sets default values for this component's properties
UInventoryComponent::UInventoryComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}

bool UInventoryComponent::AddItem(const FEquipmentInstance& Item)
{
	if (IsFull() || Item.ItemName.IsNone())
	{
		return false;
	}

	InventoryItems.Add(Item);
	OnInventoryChanged.Broadcast();

	return true;
}

bool UInventoryComponent::RemoveItem(int32 Index)
{
	if (!InventoryItems.IsValidIndex(Index))
	{
		return false;
	}

	InventoryItems.RemoveAt(Index);
	OnInventoryChanged.Broadcast();

	return true;
}

bool UInventoryComponent::EquipItemAtIndex(int32 Index)
{
	if (!InventoryItems.IsValidIndex(Index))
	{
		return false;
	}

	UEquipmentComponent* Equipment = GetOwner()->FindComponentByClass<UEquipmentComponent>();

	if (!Equipment)
	{
		return false;
	}

	const FEquipmentInstance ItemToEquip = InventoryItems[Index];

	FEquipmentInstance PreviousItem;

	const bool bReplacingItem = Equipment->GetEquippedItem(ItemToEquip.Slot, PreviousItem);

	if (!Equipment->EquipItem(ItemToEquip))
	{
		return false;
	}

	if (bReplacingItem)
	{
		InventoryItems[Index] = PreviousItem;
	}
	else
	{
		InventoryItems.RemoveAt(Index);
	}

	OnInventoryChanged.Broadcast();

	return true;
}

bool UInventoryComponent::UnequipItemToInventory(EEquipmentSlot Slot)
{
	if (IsFull())
	{
		return false;
	}

	AActor* Owner = GetOwner();

	if (!Owner)
	{
		return false;
	}

	UEquipmentComponent* Equipment = Owner->FindComponentByClass<UEquipmentComponent>();

	if (!Equipment)
	{
		return false;
	}

	FEquipmentInstance EquippedItem;

	if (!Equipment->GetEquippedItem(Slot, EquippedItem))
	{
		return false;
	}

	if (!Equipment->UnequipItem(Slot))
	{
		return false;
	}

	InventoryItems.Add(EquippedItem);
	OnInventoryChanged.Broadcast();

	return true;
}

TArray<FEquipmentInstance> UInventoryComponent::GetItems() const
{
	return InventoryItems;
}

bool UInventoryComponent::SetItems(const TArray<FEquipmentInstance>& NewItems)
{
	if (NewItems.Num() > MaxInventorySize)
	{
		return false;
	}
	InventoryItems = NewItems;
	OnInventoryChanged.Broadcast();
	return true;
}

int32 UInventoryComponent::GetItemCount() const
{
	return InventoryItems.Num();
}

bool UInventoryComponent::IsFull() const
{
	return InventoryItems.Num() >= MaxInventorySize;
}


FText UInventoryComponent::GetCharacterStatsSummary() const
{
	const AActor* Owner = GetOwner();

	if (!Owner)
	{
		return FText::GetEmpty();
	}

	const UAttributeSystem* Attributes = Owner->FindComponentByClass<UAttributeSystem>();

	if (!Attributes)
	{
		return FText::FromString(TEXT("Attributes unavailable"));
	}

	FString Summary = TEXT("CHARACTER STATS:\n");

	auto AddStat = [&](const TCHAR* Label, TSubclassOf<UAttributeSetBase> SetClass, FName AttributeName, bool bPercent = false)
		{
			float Value = 0.0f;

			if (Attributes->GetAttributeValue(SetClass, AttributeName, Value))
			{
				FString StatText = bPercent
					? FString::Printf(TEXT("%s: %.0f%%"), Label, Value)
					: FString::Printf(TEXT("%s: %.0f"), Label, Value);

				Summary += FString::Printf(TEXT("%-40s"), *StatText);
			}
		};

	const auto Combat = UCombatAttributeSet::StaticClass();
	const auto Player = UPlayerAttributeSet::StaticClass();

	AddStat(TEXT("Max Health"), Combat, TEXT("MaxHealth"));
	AddStat(TEXT("Armor"), Combat, TEXT("Armor"));
	Summary += TEXT("\n");
	AddStat(TEXT("Damage"), Combat, TEXT("DamageAmount"));
	AddStat(TEXT("Attack Speed"), Combat, TEXT("AttackSpeed"), true);
	Summary += TEXT("\n");
	AddStat(TEXT("Crit Chance"), Combat, TEXT("CriticalChance"), true);
	AddStat(TEXT("Crit Damage"), Combat, TEXT("CriticalDamage"), true);
	Summary += TEXT("\n");
	AddStat(TEXT("Movement Speed"), Combat, TEXT("MovementSpeed"), true);
	AddStat(TEXT("Evade Chance"), Combat, TEXT("EvadeChance"), true);
	Summary += TEXT("\n");
	AddStat(TEXT("Strength"), Player, TEXT("Strength"));
	AddStat(TEXT("Dexterity"), Player, TEXT("Dexterity"));
	Summary += TEXT("\n");
	AddStat(TEXT("Intelligence"), Player, TEXT("Intelligence"));
	AddStat(TEXT("Max Mana"), Player, TEXT("MaxMana"));

	return FText::FromString(Summary);
}

// Called when the game starts
void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...

}


// Called every frame
void UInventoryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}


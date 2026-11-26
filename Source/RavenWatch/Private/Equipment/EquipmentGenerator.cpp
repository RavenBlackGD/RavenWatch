// Fill out your copyright notice in the Description page of Project Settings.


#include "Equipment/EquipmentGenerator.h"

namespace
{
	TArray<FEquipmentModifierDefinition> BuildModifierPool()
	{
		TArray<FEquipmentModifierDefinition> Pool;

		const TArray<EEquipmentSlot> AllSlots = {
			EEquipmentSlot::Head,
			EEquipmentSlot::Chest,
			EEquipmentSlot::Legs,
			EEquipmentSlot::Hands,
			EEquipmentSlot::Feet,
			EEquipmentSlot::Weapon
		};

		const TArray<EEquipmentSlot> ArmorSlots = {
			EEquipmentSlot::Head,
			EEquipmentSlot::Chest,
			EEquipmentSlot::Legs,
			EEquipmentSlot::Hands,
			EEquipmentSlot::Feet
		};

		auto AddModifier = [&Pool](EEquipmentStat Stat, EAffixType Affix, float MinValue, float MaxValue, const TArray<EEquipmentSlot>& Slots)
			{
				FEquipmentModifierDefinition Mod;
				Mod.Stat = Stat;
				Mod.AffixType = Affix;
				Mod.MinValue = MinValue;
				Mod.MaxValue = MaxValue;
				Mod.AllowedSlots = Slots;

				Pool.Add(Mod);
			};

		// ---- PREFIXES ----

		AddModifier(EEquipmentStat::Strength, EAffixType::Prefix, 1, 10, AllSlots);

		AddModifier(EEquipmentStat::Dexterity, EAffixType::Prefix, 1, 10, AllSlots);

		AddModifier(EEquipmentStat::Intelligence, EAffixType::Prefix, 1, 10, AllSlots);

		AddModifier(EEquipmentStat::MaxHealth, EAffixType::Prefix, 10, 50, AllSlots);

		AddModifier(EEquipmentStat::MaxMana, EAffixType::Prefix, 10, 40, AllSlots);

		AddModifier(EEquipmentStat::Armor, EAffixType::Prefix, 5, 30, ArmorSlots);

		AddModifier(EEquipmentStat::AttackDamage, EAffixType::Prefix, 5, 20, {EEquipmentSlot::Weapon});

		AddModifier(EEquipmentStat::FlatFireDamage, EAffixType::Prefix, 3, 15, AllSlots);

		AddModifier(EEquipmentStat::FlatColdDamage, EAffixType::Prefix, 3, 15, AllSlots);

		AddModifier(EEquipmentStat::FlatLightningDamage, EAffixType::Prefix, 3, 15, AllSlots);

		// ---- SUFFIXES ----

		AddModifier(EEquipmentStat::MovementSpeed, EAffixType::Suffix, 1, 5, AllSlots);

		AddModifier(EEquipmentStat::AttackSpeed, EAffixType::Suffix, 1, 10, AllSlots);

		AddModifier(EEquipmentStat::CriticalChance, EAffixType::Suffix, 1, 5, AllSlots);

		AddModifier(EEquipmentStat::CriticalDamage, EAffixType::Suffix, 5, 20, AllSlots);

		AddModifier(EEquipmentStat::AttackDamage, EAffixType::Suffix, 2, 10, AllSlots);

		AddModifier(EEquipmentStat::IncreasedFireDamage, EAffixType::Suffix, 5, 25, AllSlots);

		AddModifier(EEquipmentStat::IncreasedColdDamage, EAffixType::Suffix, 5, 25, AllSlots);

		AddModifier(EEquipmentStat::IncreasedLightningDamage, EAffixType::Suffix, 5, 25, AllSlots);

		return Pool;
	}

	FEquipmentModifier RollModifier(const FEquipmentModifierDefinition& Definition)
	{
		FEquipmentModifier Result;

		Result.Stat = Definition.Stat;
		Result.AffixType = Definition.AffixType;
		Result.Value = FMath::FRandRange(Definition.MinValue, Definition.MaxValue);

		//NOTE: Converting all modifiers to whole-numbers for now. Likely will need to change this late
		Result.Value = FMath::RoundToFloat(Result.Value);

		return Result;
	}
}

FEquipmentInstance UEquipmentGenerator::GenerateEquipment(EEquipmentSlot Slot, FName ItemName)
{
	FEquipmentInstance Item;

	Item.Slot = Slot;
	Item.ItemName = ItemName;

	const TArray<FEquipmentModifierDefinition> ModifierPool = BuildModifierPool();

	TArray<FEquipmentModifierDefinition> PrefixPool;
	TArray<FEquipmentModifierDefinition> SuffixPool;

	for (const FEquipmentModifierDefinition& Modifier : ModifierPool)
	{
		if (!Modifier.AllowedSlots.Contains(Slot))
		{
			continue;
		}

		if (Modifier.AffixType == EAffixType::Prefix)
		{
			PrefixPool.Add(Modifier);
		}
		else if (Modifier.AffixType == EAffixType::Suffix)
		{
			SuffixPool.Add(Modifier);
		}
	}


	constexpr int32 MaxPrefixes = 2;
	constexpr int32 MaxSuffixes = 2;

	// Roll up to 2 distinct prefixes.
	for (int32 i = 0; i < MaxPrefixes && !PrefixPool.IsEmpty(); ++i)
	{
		const int32 Index = FMath::RandRange(0, PrefixPool.Num() - 1);

		Item.Modifiers.Add(RollModifier(PrefixPool[Index]));

		// Prevent the same modifier from being rolled twice.
		PrefixPool.RemoveAtSwap(Index);
	}

	// Roll up to 2 distinct suffixes.
	for (int32 i = 0; i < MaxSuffixes && !SuffixPool.IsEmpty(); ++i)
	{
		const int32 Index = FMath::RandRange(0, SuffixPool.Num() - 1);

		Item.Modifiers.Add(RollModifier(SuffixPool[Index]));

		SuffixPool.RemoveAtSwap(Index);
	}


	return Item;
}
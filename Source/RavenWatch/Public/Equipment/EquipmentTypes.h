#pragma once

#include "CoreMinimal.h"
#include "EquipmentTypes.generated.h"

UENUM(BlueprintType)
enum class EEquipmentSlot : uint8
{
	Head UMETA(DisplayName = "Head"),
	Chest UMETA(DisplayName = "Chest"),
	Legs UMETA(DisplayName = "Legs"),
	Hands UMETA(DisplayName = "Hands"),
	Feet UMETA(DisplayName = "Feet"),
	Weapon UMETA(DisplayName = "Weapon")
};

UENUM(BlueprintType)
enum class EAffixType : uint8
{
	Prefix UMETA(DisplayName = "Prefix"),
	Suffix UMETA(DisplayName = "Suffix")
};

UENUM(BlueprintType)
enum class EEquipmentStat : uint8
{
	Strength,
	Dexterity,
	Intelligence,

	MaxHealth,
	MaxMana,

	Armor,
	MovementSpeed,

	AttackDamage,
	AttackSpeed,
	CriticalChance,
	CriticalDamage,

	FlatFireDamage,
	FlatColdDamage,
	FlatLightningDamage,

	IncreasedFireDamage,
	IncreasedColdDamage,
	IncreasedLightningDamage
};

USTRUCT(BlueprintType)
struct FEquipmentModifierDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EEquipmentStat Stat = EEquipmentStat::Strength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EAffixType AffixType = EAffixType::Prefix;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinValue = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxValue = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<EEquipmentSlot> AllowedSlots;
};

USTRUCT(BlueprintType)
struct FEquipmentModifier
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EEquipmentStat Stat = EEquipmentStat::Strength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EAffixType AffixType = EAffixType::Prefix;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Value = 0.0f;
};

USTRUCT(BlueprintType)
struct FEquipmentInstance
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ItemName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EEquipmentSlot Slot = EEquipmentSlot::Head;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FEquipmentModifier> Modifiers;
};
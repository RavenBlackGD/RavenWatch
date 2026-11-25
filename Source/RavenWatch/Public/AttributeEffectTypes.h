#pragma once
#include "CoreMinimal.h"
#include "AttributeEffectTypes.generated.h"

UENUM(BlueprintType)
enum class EEffectDurationPolicy : uint8
{
	Instant,
	Infinite,
	HasDuration
};

UENUM(BlueprintType)
enum class EAttributeModifierOperation : uint8
{
	Add,
	Multiply,
	Override
};

USTRUCT(BlueprintType)
struct RAVENWATCH_API FAttributeModifier
{
	GENERATED_BODY()

	FAttributeModifier()
		: AttributeSetClass(nullptr)
		, AttributeName(NAME_None)
		, Operation(EAttributeModifierOperation::Add)
		, Magnitude(0.f)
	{
	}

	FAttributeModifier(EAttributeModifierOperation InOperation, float InMagnitude)
		: AttributeSetClass(nullptr)
		, AttributeName(NAME_None)
		, Operation(InOperation)
		, Magnitude(InMagnitude)
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<class UAttributeSetBase> AttributeSetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName AttributeName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EAttributeModifierOperation Operation =
		EAttributeModifierOperation::Add;

	// Magnitude should be a percentage number and not a decimal number. 
	// For example, if you want to multiply by 1.5, you should set the magnitude to 50.
	// If you want to multiply by 0.5, you should set the magnitude to -50.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Magnitude = 0.f;
};
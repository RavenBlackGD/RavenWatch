// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AttributeEffectTypes.h"
#include "AttributeEffectDefinitions.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class RAVENWATCH_API UAttributeEffectDefinitions : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect")
	EEffectDurationPolicy DurationPolicy =
		EEffectDurationPolicy::Instant;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect",
		meta = (EditCondition = "DurationPolicy == EEffectDurationPolicy::HasDuration"))
	float Duration = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effect")
	TArray<FAttributeModifier> Modifiers;

	
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "EquipmentTypes.h"
#include "EquipmentGenerator.generated.h"

/**
 * 
 */
UCLASS()
class RAVENWATCH_API UEquipmentGenerator : public UObject
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	static FEquipmentInstance GenerateEquipment(EEquipmentSlot Slot, FName ItemName);
	
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EquipmentTypes.h"
#include "EquipmentComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class RAVENWATCH_API UEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UEquipmentComponent();

	//Equip an item and replace the existing item in that slot, if there is one.
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool EquipItem(const FEquipmentInstance& Item);

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool UnequipItem(EEquipmentSlot Slot);

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool GetEquippedItem(EEquipmentSlot Slot, FEquipmentInstance& Item) const;

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool SetEquippedItems(const TArray<FEquipmentInstance>& Items);

	UFUNCTION(BlueprintPure, Category = "Equipment")
	float GetTotalEquipmentBonus(EEquipmentStat Stat) const;

	UFUNCTION(BlueprintPure, Category = "Equipment")
	TArray<FEquipmentInstance> GetAllEquippedItems() const;

	UFUNCTION(BlueprintCallable, Category = "Equipment|Debug")
	void DebugPrintEquipment() const;

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	TArray<FEquipmentInstance> EquippedItems;

private:

	TMap<EEquipmentSlot, FGuid> EquipmentModifierHandles;

	class UAttributeSystem* GetAttributeSystem() const;

	bool ValidateItem(const FEquipmentInstance& Item) const;

	bool ApplyItemStats(const FEquipmentInstance& Item, FGuid& OutHandle);

	bool RemoveItemStats(const FEquipmentInstance& Item, const FGuid& Handle);
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Equipment/EquipmentTypes.h"
#include "InventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class RAVENWATCH_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UInventoryComponent();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItem(const FEquipmentInstance& Item);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool EquipItemAtIndex(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool UnequipItemToInventory(EEquipmentSlot Slot);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	TArray<FEquipmentInstance> GetItems() const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool SetItems(const TArray<FEquipmentInstance>& NewItems);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetItemCount() const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsFull() const;

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryChanged OnInventoryChanged;

	UFUNCTION(BlueprintPure, Category = "Inventory|Stats")
	FText GetCharacterStatsSummary() const;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = "1"))
	int32 MaxInventorySize = 24;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TArray<FEquipmentInstance> InventoryItems;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};

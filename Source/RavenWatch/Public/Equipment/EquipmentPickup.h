// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Equipment/EquipmentTypes.h"
#include "InteractableObjectInterface.h"
#include "EquipmentPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class RAVENWATCH_API AEquipmentPickup : public AActor, public IInteractableObjectInterface
{
	GENERATED_BODY()
	
public:	
	AEquipmentPickup();

	// Called before BeginPlay so GenerateEquipment uses this slot and name.
	void PrepareDrop(UStaticMesh* Mesh, EEquipmentSlot Slot, FName ItemName);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	USphereComponent* PickupCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	UStaticMeshComponent* PickupMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	EEquipmentSlot EquipmentSlot = EEquipmentSlot::Weapon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	FName EquipmentName = FName(TEXT("Random Equipment"));

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	FEquipmentInstance GeneratedItem;

public:	

	virtual void BeginFocus() override;
	virtual void EndFocus() override;
	virtual void BeginInteract() override;
	virtual void EndInteract() override;
	virtual void Interact() override;

};

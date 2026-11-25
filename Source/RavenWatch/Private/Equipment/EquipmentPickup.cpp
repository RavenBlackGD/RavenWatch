// Fill out your copyright notice in the Description page of Project Settings.


#include "Equipment/EquipmentPickup.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Equipment/EquipmentComponent.h"
#include "InventoryComponent.h"
#include "Equipment/EquipmentGenerator.h"

// Sets default values
AEquipmentPickup::AEquipmentPickup()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	PickupCollision = CreateDefaultSubobject<USphereComponent>(TEXT("PickupCollision"));

	RootComponent = PickupCollision;

	PickupCollision->InitSphereRadius(120.f);
	PickupCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupCollision->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	PickupCollision->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	PickupCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	PickupCollision->SetGenerateOverlapEvents(true);

	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));

	PickupMesh->SetupAttachment(RootComponent);
	PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	InteractionData.InteractionName = FText::FromString(TEXT("Pick Up Equipment"));

	InteractionData.InteractionDescription = FText::FromString(TEXT("Collect Equipment"));

	InteractionData.InteractionDuration = 0.0f;
}

void AEquipmentPickup::PrepareDrop(UStaticMesh* Mesh, EEquipmentSlot Slot, FName ItemName)
{
	EquipmentSlot = Slot;
	EquipmentName = ItemName;

	if (PickupMesh && Mesh)
	{
		PickupMesh->SetStaticMesh(Mesh);
	}
}

// Called when the game starts or when spawned
void AEquipmentPickup::BeginPlay()
{
	Super::BeginPlay();
	
	GeneratedItem = UEquipmentGenerator::GenerateEquipment(EquipmentSlot, EquipmentName);

	if (PickupMesh)
	{
		PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (PickupCollision == nullptr)
	{
		return;
	}
}

void AEquipmentPickup::BeginFocus()
{
	if (PickupMesh)
	{
		PickupMesh->SetRenderCustomDepth(true);
	}
}

void AEquipmentPickup::EndFocus()
{
	if (PickupMesh)
	{
		PickupMesh->SetRenderCustomDepth(false);
	}
}

void AEquipmentPickup::BeginInteract()
{
}

void AEquipmentPickup::EndInteract()
{
}

void AEquipmentPickup::Interact()
{
	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;

	APawn* PlayerPawn = PC ? PC->GetPawn() : nullptr;

	if (!PlayerPawn)
	{
		return;
	}

	UInventoryComponent* Inventory = PlayerPawn->FindComponentByClass<UInventoryComponent>();

	if (!Inventory)
	{
		return;
	}

	if (Inventory->AddItem(GeneratedItem))
	{
		Destroy();
	}
}

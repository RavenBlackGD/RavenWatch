// Fill out your copyright notice in the Description page of Project Settings.

#include "BasePickup.h"
#include "Components/BoxComponent.h"

ABasePickup::ABasePickup()
{
	PrimaryActorTick.bCanEverTick = false;

	BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollision"));
	if (BoxCollision)
	{
		SetRootComponent(BoxCollision);
		BoxCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		BoxCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
		BoxCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		BoxCollision->OnComponentBeginOverlap.AddDynamic(this, &ABasePickup::HandleOverlap);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("BasePickup: BoxCollision failed to create."));
	}
}

void ABasePickup::BeginPlay()
{
	Super::BeginPlay();
}

void ABasePickup::HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("BasePickup: Overlap had no other actor."));
		return;
	}

	if (!CanPickup(OtherActor))
	{
		return;
	}

	HandlePickup(OtherActor, SweepResult);
	PostPickup();
}

bool ABasePickup::CanPickup(AActor* OtherActor) const
{
	return true;
}

void ABasePickup::HandlePickup(AActor* OtherActor, const FHitResult& SweepResult)
{}

void ABasePickup::PostPickup()
{
	Destroy();
}
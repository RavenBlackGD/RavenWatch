// Fill out your copyright notice in the Description page of Project Settings.


#include "PotionPickup.h"
#include "BasePlayer.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerController.h"

APotionPickup::APotionPickup()
{
    PrimaryActorTick.bCanEverTick = false;

    HealAmount = 40.0f;
    Effect = EPotionEffect::Health;
    SpeedMultiplier = 1.5f;
    EffectDuration = 15.0f;
    ManaAmount = 40.0f;
    PickupRadius = 120.0f;
    MeshScale = FVector(1.0f, 1.0f, 1.0f);

    PickupCollision = CreateDefaultSubobject<USphereComponent>(TEXT("PickupCollision"));
    RootComponent = PickupCollision;

    // Only the player's interaction trace needs to find this
    PickupCollision->InitSphereRadius(PickupRadius);
    PickupCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    PickupCollision->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
    PickupCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    PickupCollision->SetGenerateOverlapEvents(false);

    PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
    PickupMesh->SetupAttachment(RootComponent);
    PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PickupMesh->SetRelativeScale3D(MeshScale);

    InteractionData.InteractionName = FText::FromString(TEXT("Pick Up Potion"));
    InteractionData.InteractionDescription = FText::FromString(TEXT("Use Potion"));
    InteractionData.InteractionDuration = 0.0f;
}

void APotionPickup::PrepareDrop(UStaticMesh* Mesh, EPotionEffect InEffect, float InHealAmount, float InSpeedMultiplier, float InDuration, float InManaAmount)
{
    Effect = InEffect;

    if (InHealAmount > 0.0f)
    {
        HealAmount = InHealAmount;
    }

    if (InSpeedMultiplier > 0.0f)
    {
        SpeedMultiplier = InSpeedMultiplier;
    }

    if (InDuration > 0.0f)
    {
        EffectDuration = InDuration;
    }

    if (InManaAmount > 0.0f)
    {
        ManaAmount = InManaAmount;
    }

    if (PickupMesh && Mesh)
    {
        PickupMesh->SetStaticMesh(Mesh);
    }
}

void APotionPickup::BeginPlay()
{
    Super::BeginPlay();

    if (PickupMesh)
    {
        PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        PickupMesh->SetRelativeScale3D(MeshScale);
    }

    // Prompt reads "Pick Up Health", "Pick Up Speed" or "Pick Up Mana" from the effect's display name
    InteractionData.InteractionName = FText::Format(
        NSLOCTEXT("Potion", "PickUpFormat", "Pick Up {0}"),
        StaticEnum<EPotionEffect>()->GetDisplayNameTextByValue(static_cast<int64>(Effect)));
}

bool APotionPickup::GivePotionTo(ABasePlayer* Player)
{
    if (!IsValid(Player))
    {
        return false;
    }

    float Amount = HealAmount;
    if (Effect == EPotionEffect::Speed)
    {
        Amount = SpeedMultiplier;
    }
    else if (Effect == EPotionEffect::Mana)
    {
        Amount = ManaAmount;
    }

    return Player->ApplyPotion(Effect, Amount, EffectDuration);
}

void APotionPickup::BeginFocus()
{
    if (PickupMesh)
    {
        PickupMesh->SetRenderCustomDepth(true);
    }
}

void APotionPickup::EndFocus()
{
    if (PickupMesh)
    {
        PickupMesh->SetRenderCustomDepth(false);
    }
}

void APotionPickup::BeginInteract()
{
}

void APotionPickup::EndInteract()
{
}

void APotionPickup::Interact()
{
    APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
    ABasePlayer* Player = PC ? Cast<ABasePlayer>(PC->GetPawn()) : nullptr;

    if (GivePotionTo(Player))
    {
        Destroy();
    }
}
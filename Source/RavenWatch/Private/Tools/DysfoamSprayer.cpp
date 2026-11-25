// Fill out your copyright notice in the Description page of Project Settings.


#include "Tools/DysfoamSprayer.h"

#include "BasePlayer.h"
#include "Tools/DysfoamProjectile.h"
#include "Tools/DysfoamShockwave.h"
#include "Tools/ToolAttributeSet.h"

ADysfoamSprayer::ADysfoamSprayer()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ADysfoamSprayer::BeginPlay()
{
	Super::BeginPlay();

	InitializeTool();
}

void ADysfoamSprayer::InitializeTool()
{
	if (ToolData.ToolName.IsNone())
	{
		ToolData.ToolName =
			FName(TEXT("Dysfoam Sprayer"));
	}

	if (ToolData.ToolDescription.IsEmpty())
	{
		ToolData.ToolDescription =
			FText::FromString(
				TEXT("Sprays Dissasembly Foam rapidly which can be primed to disintegrate anything its in contact with by the Primer Shockwave"));
	}

	if (ToolData.Ability1.AbilityName.IsNone())
	{
		ToolData.Ability1.AbilityName =
			FName(TEXT("Disassembly Foam"));
	}

	if (ToolData.Ability1.AbilityDescription.IsEmpty())
	{
		ToolData.Ability1.AbilityDescription =
			FText::FromString(
				TEXT("Continuously sprays Dissasembly Foam rapidly. Base formula is harmless on its own but sticky enough to slow anything moving through it."));
	}

	ToolData.Ability1.bIsHeldAbility = true;

	if (ToolData.Ability2.AbilityName.IsNone())
	{
		ToolData.Ability2.AbilityName =
			FName(TEXT("Priming Shockwave"));
	}

	if (ToolData.Ability2.AbilityDescription.IsEmpty())
	{
		ToolData.Ability2.AbilityDescription =
			FText::FromString(
				TEXT("Expands a shockwave that knocks back targets and primes attached Dissasembly Foam."));
	}

	UToolAttributeSet* FoamAttributes =
		GetAbilityAttributes(1);

	if (FoamAttributes)
	{
		FoamAttributes->ManaCost.Initialize(2.0f);
		FoamAttributes->CooldownTime.Initialize(0.12f);
		FoamAttributes->Duration.Initialize(5.0f);
		FoamAttributes->AbilityMagnitude.Initialize(-50.0f);
		FoamAttributes->Synergy.Initialize(100.0f);
		FoamAttributes->ProjectileSpeed.Initialize(700.0f);
	}

	UToolAttributeSet* ShockwaveAttributes =
		GetAbilityAttributes(2);

	if (ShockwaveAttributes)
	{
		ShockwaveAttributes->ManaCost.Initialize(35.0f);
		ShockwaveAttributes->CooldownTime.Initialize(1.0f);
		ShockwaveAttributes->Duration.Initialize(0.2f);
		ShockwaveAttributes->Range.Initialize(450.0f);
		ShockwaveAttributes->AreaOfEffect.Initialize(120.0f);
		ShockwaveAttributes->AbilityMagnitude.Initialize(900.0f);
	}
}

AActor* ADysfoamSprayer::GetAbilityOwner() const
{
	return GetOwner();
}

void ADysfoamSprayer::ActivateAbility(int32 AbilityIndex)
{
	if (GetWorld() == nullptr)
	{
		return;
	}

	AActor* OwnerActor =
		GetAbilityOwner();

	ABasePlayer* Player =
		Cast<ABasePlayer>(OwnerActor);

	if (OwnerActor == nullptr ||
		Player == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("DysfoamSprayer: Owner actor or player is not valid."));
		return;
	}

	const FVector SpawnLocation =
		OwnerActor->GetActorLocation() +
		Player->GetBaseAimRotation().Vector() *
		SpawnDistance;

	const FRotator SpawnRotation =
		Player->GetBaseAimRotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerActor;
	SpawnParams.Instigator = Player;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	switch (AbilityIndex)
	{
	case 1:
	{
		if (FoamProjectileClass == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("DysfoamSprayer: FoamProjectileClass is not set."));
			return;
		}

		ADysfoamProjectile* Foam =
			GetWorld()->SpawnActor<ADysfoamProjectile>(
				FoamProjectileClass,
				SpawnLocation,
				SpawnRotation,
				SpawnParams);

		if (Foam)
		{
			Foam->InitializeFoamAttributes(
				GetAbilityAttributes(1));
		}

		break;
	}

	case 2:
	{
		if (ShockwaveClass == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("DysfoamSprayer: ShockwaveClass is not set."));
			return;
		}

		ADysfoamShockwave* Shockwave =
			GetWorld()->SpawnActor<ADysfoamShockwave>(
				ShockwaveClass,
				SpawnLocation,
				SpawnRotation,
				SpawnParams);

		if (Shockwave)
		{
			Shockwave->InitializeShockwaveAttributes(
				GetAbilityAttributes(2));
		}

		break;
	}

	default:
		break;
	}
}


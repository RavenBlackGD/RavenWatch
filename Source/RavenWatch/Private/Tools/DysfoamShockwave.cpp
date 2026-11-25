// Fill out your copyright notice in the Description page of Project Settings.


#include "Tools/DysfoamShockwave.h"

#include "BasePlayer.h"
#include "GameFramework/Character.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "PhysicsEngine/BodyInstance.h"
#include "Components/SphereComponent.h"
#include "Tools/DysfoamProjectile.h"

ADysfoamShockwave::ADysfoamShockwave()
{
	PrimaryActorTick.bCanEverTick = true;

	DestroyTime = 0.25f;
	GravityScale = 0.0f;
	ShotSpeed = 0.0f;
}

void ADysfoamShockwave::InitializeShockwaveAttributes(
	UToolAttributeSet* InAttributes)
{
	ShockwaveAttributes = InAttributes;

	if (ShockwaveAttributes == nullptr)
	{
		return;
	}

	const float Duration =
		FMath::Max(
			0.01f,
			ShockwaveAttributes->Duration.GetCurrentValue());

	const float TravelRange =
		FMath::Max(
			0.0f,
			ShockwaveAttributes->Range.GetCurrentValue());

	// The shockwave travels its entire Range during its active duration.
	const float TravelSpeed =
		TravelRange / Duration;

	SetLifeSpan(Duration);

	if (ProjectileMovement)
	{
		ProjectileMovement->InitialSpeed = TravelSpeed;
		ProjectileMovement->MaxSpeed = TravelSpeed;
		ProjectileMovement->ProjectileGravityScale = 0.0f;
		ProjectileMovement->Velocity =
			GetActorForwardVector() * TravelSpeed;
	}
}

void ADysfoamShockwave::BeginPlay()
{
	Super::BeginPlay();

	if (SphereCollision)
	{
		SphereCollision->SetCollisionEnabled(
			ECollisionEnabled::QueryOnly);
		SphereCollision->SetCollisionResponseToAllChannels(
			ECR_Overlap);
		SphereCollision->SetGenerateOverlapEvents(true);

		// AProjectile's default effective sphere radius is 8 units.
		SphereCollision->SetRelativeScale3D(
			FVector(StartRadius / 8.0f));
	}

	SetActorScale3D(FVector(0.1f));
}

void ADysfoamShockwave::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (ShockwaveAttributes == nullptr)
	{
		return;
	}

	ElapsedTime += DeltaSeconds;

	const float Duration =
		FMath::Max(
			0.01f,
			ShockwaveAttributes->Duration.GetCurrentValue());

	const float FinalRadius =
		FMath::Max(
			StartRadius,
			ShockwaveAttributes->AreaOfEffect.GetCurrentValue());

	const float Alpha = FMath::Clamp(
		ElapsedTime / Duration,
		0.0f,
		1.0f);

	const float CurrentRadius =
		FMath::Lerp(
			StartRadius,
			FinalRadius,
			Alpha);

	// AProjectile's default effective sphere radius is 8 units.
	SetActorScale3D(
		FVector(CurrentRadius / 8.0f));
}

bool ADysfoamShockwave::IsPlayerActor(
	const AActor* Actor) const
{
	return Actor != nullptr &&
		Actor->IsA<ABasePlayer>();
}

void ADysfoamShockwave::HandleOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (OtherActor == nullptr ||
		OtherActor == this ||
		OtherActor == GetOwner() ||
		AffectedActors.Contains(OtherActor))
	{
		return;
	}

	AffectedActors.Add(OtherActor);

	// Foam uses its own Ability 1 Synergy attribute.
	if (ADysfoamProjectile* Foam =
		Cast<ADysfoamProjectile>(OtherActor))
	{
		Foam->ActivateSynergy();
		return;
	}

	// The shockwave cannot knock back the player.
	if (IsPlayerActor(OtherActor))
	{
		return;
	}

	ApplyKnockback(OtherActor, OtherComp);
}

void ADysfoamShockwave::ApplyKnockback(
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent)
{
	if (OtherActor == nullptr ||
		ShockwaveAttributes == nullptr)
	{
		return;
	}

	const float KnockbackStrength =
		FMath::Max(
			0.0f,
			ShockwaveAttributes
			->AbilityMagnitude
			.GetCurrentValue());

	if (KnockbackStrength <= 0.0f)
	{
		return;
	}

	const FVector Direction =
		(OtherActor->GetActorLocation() -
			GetActorLocation()).GetSafeNormal();

	if (ACharacter* Character =
		Cast<ACharacter>(OtherActor))
	{
		Character->LaunchCharacter(
			Direction * KnockbackStrength,
			true,
			true);

		return;
	}

	if (OtherComponent != nullptr &&
		OtherComponent->IsSimulatingPhysics())
	{
		OtherComponent->AddImpulse(
			Direction * KnockbackStrength,
			NAME_None,
			true);
	}
}


// Fill out your copyright notice in the Description page of Project Settings.


#include "Tools/DysfoamProjectile.h"

#include "AttributeEffectComponent.h"
#include "AttributeEffectDefinitions.h"
#include "AttributeEffectTypes.h"
#include "CombatAttributeSet.h"
#include "BasePlayer.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"

ADysfoamProjectile::ADysfoamProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	DestroyTime = 5.0f;
	GravityScale = 1.0f;
	ShotSpeed = 700.0f;

	// set default values for collision and mesh scale if not set in editor
	if (CollisionScale.IsNearlyZero())
	{
		CollisionScale = FVector(2.5f);
	}
	if (MeshScale.IsNearlyZero())
	{
		MeshScale = FVector(0.5f);
	}
}

void ADysfoamProjectile::InitializeFoamAttributes(
	UToolAttributeSet* InAttributes)
{
	FoamAttributes = InAttributes;
}

bool ADysfoamProjectile::IsIgnoredActor(
	const AActor* Actor) const
{
	return Actor == nullptr ||
		Actor == this ||
		Actor == GetOwner() ||
		Actor == GetInstigator() ||
		Actor->IsA<ABasePlayer>();
}

void ADysfoamProjectile::AttachToSurface(
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	const FHitResult& Hit)
{
	if (bAttached ||
		OtherComponent == nullptr)
	{
		return;
	}

	if (OtherActor == nullptr)
	{
		OtherActor = OtherComponent->GetOwner();
	}

	if (IsIgnoredActor(OtherActor))
	{
		return;
	}

	bAttached = true;
	AttachedActor = OtherActor;
	AttachedComponent = OtherComponent;

	if (ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->Deactivate();
	}

	if (AttachedComponent)
	{
		AttachToComponent(
			AttachedComponent,
			FAttachmentTransformRules::KeepWorldTransform);
	}
	else if (AttachedActor)
	{
		AttachToActor(
			AttachedActor,
			FAttachmentTransformRules::KeepWorldTransform);
	}


	if (Hit.bBlockingHit)
	{
		const float CollisionRadius =
			SphereCollision->GetScaledSphereRadius();

		const float EmbedDepth =
			CollisionRadius * SurfaceEmbedFraction;

		const FVector SurfaceLocation =
			Hit.ImpactPoint +
			Hit.ImpactNormal *
			(CollisionRadius - EmbedDepth);

		SetActorLocation(SurfaceLocation);

		// Align the projectile's local up axis with the surface normal.
		const FRotator SurfaceRotation =
			FRotationMatrix::MakeFromZ(
				Hit.ImpactNormal).Rotator();

		SetActorRotation(SurfaceRotation);
	}

}

bool ADysfoamProjectile::HasAttachmentIgnoreTag(
	const AActor* Actor) const
{
	if (Actor == nullptr)
	{
		return false;
	}

	for (const FName& IgnoredTag : AttachmentIgnoreTags)
	{
		if (!IgnoredTag.IsNone() &&
			Actor->ActorHasTag(IgnoredTag))
		{
			return true;
		}
	}

	return false;
}

void ADysfoamProjectile::HandleOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (OtherActor == nullptr ||
		OtherActor == this)
	{
		return;
	}

	// These tags mean the foam should ignore the actor completely.
	if (HasIgnoredActorTag(OtherActor))
	{
		return;
	}

	// Every non-ignored actor receives the foam effect.
	ApplyFoamEffect(OtherActor);

	// An attached foam projectile does not attach again.
	if (bAttached)
	{
		return;
	}

	// The firing owner and instigator can be slowed, but foam cannot
	// attach to them.
	if (OtherActor == GetOwner() ||
		OtherActor == GetInstigator())
	{
		return;
	}

	// These actors can be slowed but cannot receive attached foam.
	if (HasAttachmentIgnoreTag(OtherActor))
	{
		return;
	}

	AttachToSurface(
		OtherActor,
		OtherComp,
		SweepResult);
}

void ADysfoamProjectile::HandleOverlapEnd(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	RemoveFoamEffect(OtherActor);
}

void ADysfoamProjectile::HandleHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	if (OtherActor == nullptr ||
		OtherActor == this)
	{
		return;
	}

	if (HasIgnoredActorTag(OtherActor))
	{
		return;
	}

	// A blocking surface normally does not represent a traversable actor,
	// so it can be attached to unless explicitly excluded.
	if (OtherActor != GetOwner() &&
		OtherActor != GetInstigator() &&
		!HasAttachmentIgnoreTag(OtherActor))
	{
		AttachToSurface(
			OtherActor,
			OtherComp,
			Hit);
	}
}

void ADysfoamProjectile::ApplyFoamEffect(AActor* Actor)
{
	if (Actor == nullptr ||
		ActiveSlowEffects.Contains(Actor) ||
		FoamAttributes == nullptr)
	{
		return;
	}

	UAttributeEffectComponent* EffectComponent =
		Actor->FindComponentByClass<UAttributeEffectComponent>();

	if (EffectComponent == nullptr)
	{
		return;
	}

	const float AbilityMagnitude =
		FoamAttributes->AbilityMagnitude.GetCurrentValue();

	if (FMath::IsNearlyZero(AbilityMagnitude))
	{
		return;
	}

	UAttributeEffectDefinitions* EffectDefinition =
		NewObject<UAttributeEffectDefinitions>(this);

	if (EffectDefinition == nullptr)
	{
		return;
	}

	EffectDefinition->DurationPolicy =
		EEffectDurationPolicy::Infinite;

	FAttributeModifier MovementModifier;
	MovementModifier.AttributeSetClass =
		UCombatAttributeSet::StaticClass();
	MovementModifier.AttributeName =
		TEXT("MovementSpeed");
	MovementModifier.Operation =
		EAttributeModifierOperation::Multiply;
	MovementModifier.Magnitude =
		AbilityMagnitude;

	EffectDefinition->Modifiers.Add(MovementModifier);

	FGuid EffectHandle;
	if (EffectComponent->ApplyEffect(
		EffectDefinition,
		EffectHandle))
	{
		ActiveSlowEffects.Add(Actor, EffectHandle);
	}
}

void ADysfoamProjectile::RemoveFoamEffect(AActor* Actor)
{
	if (Actor == nullptr)
	{
		return;
	}

	FGuid* EffectHandle = ActiveSlowEffects.Find(Actor);
	if (EffectHandle == nullptr)
	{
		return;
	}

	if (UAttributeEffectComponent* EffectComponent =
		Actor->FindComponentByClass<UAttributeEffectComponent>())
	{
		EffectComponent->RemoveEffect(*EffectHandle);
	}

	ActiveSlowEffects.Remove(Actor);
}

void ADysfoamProjectile::ActivateSynergy()
{
	if (!bAttached ||
		AttachedActor == nullptr ||
		FoamAttributes == nullptr)
	{
		return;
	}

	const float SynergyDamage =
		FoamAttributes->Synergy.GetCurrentValue();

	if (SynergyDamage > 0.0f)
	{
		UGameplayStatics::ApplyDamage(
			AttachedActor,
			SynergyDamage,
			GetInstigatorController(),
			this,
			nullptr);
	}

	Destroy();
}

void ADysfoamProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (SphereCollision == nullptr)
	{
		return;
	}

	// Dynamic actors remain overlaps so foam can slow them.
	// Static level geometry blocks the projectile and generates a hit.
	SphereCollision->SetCollisionResponseToChannel(
		ECC_WorldStatic,
		ECR_Block);

	SphereCollision->SetNotifyRigidBodyCollision(true);

	SphereCollision->OnComponentHit.AddDynamic(
		this,
		&ADysfoamProjectile::HandleHit);
}

void ADysfoamProjectile::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	for (const TPair<TObjectPtr<AActor>, FGuid>& Entry :
		ActiveSlowEffects)
	{
		if (Entry.Key == nullptr)
		{
			continue;
		}

		if (UAttributeEffectComponent* EffectComponent =
			Entry.Key->FindComponentByClass<UAttributeEffectComponent>())
		{
			EffectComponent->RemoveEffect(Entry.Value);
		}
	}

	ActiveSlowEffects.Empty();

	Super::EndPlay(EndPlayReason);
}


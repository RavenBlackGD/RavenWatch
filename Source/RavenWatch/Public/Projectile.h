// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GenericTeamAgentInterface.h"
#include "Projectile.generated.h"

/**
 * Flying bullet actor.
 * Uses overlap to apply damage, then destroys itself.
 */
UCLASS()
class RAVENWATCH_API AProjectile : public AActor
{
    GENERATED_BODY()

public:
    // Sets default values for this actor
    AProjectile();

    virtual void InitializeDamage(float InDamage, bool bInCriticalHit);

protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

    // Root collision sphere (overlap)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Projectile)
    class USphereComponent* SphereCollision;

    // Visual sphere mesh (no collision)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Projectile)
    class UStaticMeshComponent* SphereMesh;

    // Moves the projectile forward
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Projectile)
    class UProjectileMovementComponent* ProjectileMovement;

    // How long the bullet lives if it hits nothing (seconds)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Projectile)
    float DestroyTime;

    // How much damage this bullet deals
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Projectile)
    float Damage;

    // Scale of the overlap sphere. 0.25 keeps the old radius of 8.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Projectile)
    FVector CollisionScale;

    // Scale of the visible mesh. The mesh asset stays on the Blueprint.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Projectile)
    FVector MeshScale;

    // 0 flies flat. Raise it when a shot should drop.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Projectile)
    float GravityScale;

    // Units per second.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Projectile)
    float ShotSpeed;

	// Tags of actors that this projectile should ignore when overlapping. For example, other projectiles.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Collision")
    TArray<FName> IgnoredActorTags;

    bool HasIgnoredActorTag(const AActor* Actor) const;

    // Overlap callback
    UFUNCTION()
    virtual void HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
    virtual void HandleOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

private:
    // Timer handle for auto destroy
    FTimerHandle ProjectileTimer;

    bool bCriticalHit = false;

    // The shooter's team, saved when fired so it still counts if the shooter dies mid-flight
    FGenericTeamId ShooterTeam = FGenericTeamId::NoTeam;

};
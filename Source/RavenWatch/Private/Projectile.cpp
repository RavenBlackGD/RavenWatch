// Fill out your copyright notice in the Description page of Project Settings.

#include "Projectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

// Sets default values
AProjectile::AProjectile()
{
    // Tick off for performance
    PrimaryActorTick.bCanEverTick = false;

    DestroyTime = 3.0f;
    Damage = 10.0f;
    CollisionScale = FVector(0.25f);
    MeshScale = FVector(0.15f);
    GravityScale = 0.0f;
    ShotSpeed = 3000.0f;

    // Sphere collision
    SphereCollision = CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollision"));
    if (SphereCollision)
    {
        SetRootComponent(SphereCollision);
        SphereCollision->SetRelativeScale3D(CollisionScale);
        SphereCollision->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
        SphereCollision->SetGenerateOverlapEvents(true);
    }
    else
    {
        // Log after error checking
        UE_LOG(LogTemp, Error, TEXT("Projectile: SphereCollision failed to create."));
    }

    // Visual mesh (asset is set on the Blueprint)
    SphereMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SphereMesh"));
    if (SphereMesh)
    {
        SphereMesh->SetCollisionProfileName(TEXT("NoCollision"));
        SphereMesh->SetRelativeScale3D(MeshScale);
        if (SphereCollision)
        {
            SphereMesh->SetupAttachment(SphereCollision);
        }
    }
    else
    {
        // Log after error checking
        UE_LOG(LogTemp, Error, TEXT("Projectile: SphereMesh failed to create."));
    }

    // Projectile movement
    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    if (ProjectileMovement)
    {
        ProjectileMovement->ProjectileGravityScale = GravityScale;
        ProjectileMovement->InitialSpeed = ShotSpeed;
        ProjectileMovement->MaxSpeed = ShotSpeed;
        ProjectileMovement->bRotationFollowsVelocity = true;
        if (SphereCollision)
        {
            ProjectileMovement->SetUpdatedComponent(SphereCollision);
        }
    }
    else
    {
        // Log after error checking
        UE_LOG(LogTemp, Error, TEXT("Projectile: ProjectileMovement failed to create."));
    }
}

void AProjectile::InitializeDamage(float InDamage, bool bInCriticalHit)
{
    Damage = FMath::Max(0.0f, InDamage);
    bCriticalHit = bInCriticalHit;
}

// Called when the game starts or when spawned
void AProjectile::BeginPlay()
{
    Super::BeginPlay();

    // Remember whose side fired this
    ShooterTeam = FGenericTeamId::GetTeamIdentifier(GetInstigator());

    // Bind overlap
    if (SphereCollision)
    {
        SphereCollision->OnComponentBeginOverlap.AddDynamic(this, &AProjectile::HandleOverlap);
		SphereCollision->OnComponentEndOverlap.AddDynamic(this, &AProjectile::HandleOverlapEnd);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Projectile: SphereCollision was null. Overlap not bound."));
    }

    // Auto destroy if we hit nothing
    UWorld* World = GetWorld();
    if (World)
    {
        World->GetTimerManager().SetTimer(ProjectileTimer, this, &AActor::K2_DestroyActor, DestroyTime, false);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Projectile: GetWorld() was null. Destroy timer not set."));
    }
}

bool AProjectile::HasIgnoredActorTag(
    const AActor* Actor) const
{
    if (Actor == nullptr)
    {
        return false;
    }

    for (const FName& IgnoredTag : IgnoredActorTags)
    {
        if (!IgnoredTag.IsNone() &&
            Actor->ActorHasTag(IgnoredTag))
        {
            return true;
        }
    }

    return false;
}

// Overlap: damage the other actor, then destroy this bullet
void AProjectile::HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    // Ignore empty, self, the shooter, and the shooter's gun
    if (OtherActor == nullptr || OtherActor == this)
    {
        return;
    }
    if (HasIgnoredActorTag(OtherActor))
    {
        return;
	}
    if (OtherActor == GetOwner() || OtherActor == GetInstigator())
    {
        return;
    }
    if (GetInstigator() && OtherActor->GetOwner() == GetInstigator())
    {
        return;
    }

    // Fly through anything on the shooter's team. Actors with no team, like the shelter, still get hit.
    if (ShooterTeam != FGenericTeamId::NoTeam &&
        FGenericTeamId::GetAttitude(ShooterTeam, FGenericTeamId::GetTeamIdentifier(OtherActor)) == ETeamAttitude::Friendly)
    {
        return;
    }

    UGameplayStatics::ApplyDamage(OtherActor, Damage, GetInstigatorController(), this, nullptr);

    const FString DamageMessage = FString::Printf(TEXT("%s | Damage: %.1f | Critical: %s"), *OtherActor->GetName(), Damage, bCriticalHit ? TEXT("YES") : TEXT("NO"));

    UKismetSystemLibrary::PrintString(this, DamageMessage, true, true, bCriticalHit ? FLinearColor::Yellow : FLinearColor::White, 3.0f);

    Destroy();
}

void AProjectile::HandleOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
}



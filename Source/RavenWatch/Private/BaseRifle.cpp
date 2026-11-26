// Fill out your copyright notice in the Description page of Project Settings.

#include "BaseRifle.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "Projectile.h"
#include "CombatComponent.h"

// Sets default values
ABaseRifle::ABaseRifle()
{
    PrimaryActorTick.bCanEverTick = false;

    ProjectileClass = nullptr;
    MaxAmmo = 30;
    CurrentAmmo = 0;
    FireRate = 0.2f;
    MuzzleOffset = FVector(60.0f, 0.0f, 0.0f);
    bCoolingDown = false;
    bReloading = false;
    FireSound = nullptr;
    ReloadSound = nullptr;
    SoundAttenuation = nullptr;

    // Placeholder mesh (asset set on the Blueprint)
    RifleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RifleMesh"));
    if (RifleMesh)
    {
        SetRootComponent(RifleMesh);
        RifleMesh->SetCollisionProfileName(TEXT("NoCollision"));
    }
    else
    {
        // Log after error checking
        UE_LOG(LogTemp, Error, TEXT("BaseRifle: RifleMesh failed to create."));
    }

    // Muzzle point in front of the gun
    Muzzle = CreateDefaultSubobject<USceneComponent>(TEXT("Muzzle"));
    if (Muzzle)
    {
        if (RifleMesh)
        {
            Muzzle->SetupAttachment(RifleMesh);
        }
        Muzzle->SetRelativeLocation(MuzzleOffset);
    }
    else
    {
        // Log after error checking
        UE_LOG(LogTemp, Error, TEXT("BaseRifle: Muzzle failed to create."));
    }
}

// Called when the game starts or when spawned
void ABaseRifle::BeginPlay()
{
    Super::BeginPlay();

    // Start with a full clip and tell the HUD
    ReloadAmmo();
}

void ABaseRifle::RequestReload()
{
    if (bReloading || CurrentAmmo >= MaxAmmo)
    {
        return;
    }

    bReloading = true;
    OnReloadStart.Broadcast();
}

void ABaseRifle::EndReload()
{
    bReloading = false;
}

// Play the reload sound at the muzzle
void ABaseRifle::PlayReloadSound()
{
    if (ReloadSound == nullptr)
    {
        return;
    }

    const FVector SoundLocation = Muzzle ? Muzzle->GetComponentLocation() : GetActorLocation();
    UGameplayStatics::PlaySoundAtLocation(this, ReloadSound, SoundLocation, FRotator::ZeroRotator, 1.0f, 1.0f, 0.0f, SoundAttenuation);
}

void ABaseRifle::ReloadAmmo()
{
    CurrentAmmo = MaxAmmo;
    OnAmmoChanged.Broadcast(CurrentAmmo, MaxAmmo);

    // Log after error checking
    UE_LOG(LogTemp, Log, TEXT("BaseRifle: Reloaded to %d."), CurrentAmmo);
}

void ABaseRifle::ResetCooldown()
{
    bCoolingDown = false;
}

void ABaseRifle::Attack()
{
    // Gate: cooldown and ammo
    if (bReloading || bCoolingDown || CurrentAmmo <= 0)
    {
        return;
    }

    if (ProjectileClass == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("BaseRifle: Attack aborted. ProjectileClass is none."));
        return;
    }

    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("BaseRifle: Attack aborted. GetWorld() was null."));
        return;
    }

    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (OwnerPawn == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("BaseRifle: Attack aborted. Owner is not a pawn."));
        return;
    }

    UCombatComponent* Combat = OwnerPawn->FindComponentByClass<UCombatComponent>();

    // Spawn at the muzzle, aimed where the camera/controller looks
    const FVector SpawnLocation = Muzzle ? Muzzle->GetComponentLocation() : GetActorLocation();

    // Player uses crosshair aim. Enemies keep their controller aim.
    const FRotator SpawnRotation = OwnerPawn->GetBaseAimRotation();

    FActorSpawnParameters Params;
    Params.Owner = OwnerPawn;
    Params.Instigator = OwnerPawn;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AActor* Spawned = World->SpawnActor<AActor>(ProjectileClass, SpawnLocation, SpawnRotation, Params);
    if (Spawned == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("BaseRifle: SpawnActor failed for projectile."));
        return;
    }

    if (AProjectile* Projectile = Cast<AProjectile>(Spawned))
    {
        if (Combat)
        {
            const FCombatDamageResult AttackResult = Combat->CalculateAttack(10.0f);

            Projectile->InitializeDamage(AttackResult);
        }
    }

    // Play the gunshot at the muzzle
    if (FireSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, FireSound, SpawnLocation, FRotator::ZeroRotator, 1.0f, 1.0f, 0.0f, SoundAttenuation);
    }

    // Spend one bullet and tell the HUD
    CurrentAmmo = FMath::Max(CurrentAmmo - 1, 0);
    OnAmmoChanged.Broadcast(CurrentAmmo, MaxAmmo);

    // Start cooldown
    bCoolingDown = true;

    const float EffectiveFireRate = Combat ? Combat->GetAttackInterval(FireRate) : FireRate;

    World->GetTimerManager().SetTimer(FireTimerHandle, this, &ABaseRifle::ResetCooldown, EffectiveFireRate, false);
}
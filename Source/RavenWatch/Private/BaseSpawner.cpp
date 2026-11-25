// Fill out your copyright notice in the Description page of Project Settings.

#include "BaseSpawner.h"
#include "BaseEnemy.h"
#include "HealthComponent.h"
#include "RavenGameMode.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Particles/ParticleSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"

// Sets default values
ABaseSpawner::ABaseSpawner()
{

    // Tick drives the spin and bob
    PrimaryActorTick.bCanEverTick = true;

    EnemyClass = nullptr;
    TeamId = 1;
    SpawnInterval = 8.0f;
    FirstSpawnDelay = 1.0f;
    SpawnOffset = FVector(200.0f, 0.0f, 100.0f);
    MaxAlive = 3;
    DeathEffect = nullptr;
    DeathEffectScale = FVector(1.0f);
    DeathSound = nullptr;
    DeathSoundAttenuation = nullptr;
    StandUpRotation = FRotator(0.0f, 0.0f, 90.0f);
    SpinSpeed = 30.0f;
    BobHeight = 15.0f;
    BobSpeed = 0.5f;
    SwayAmount = 8.0f;
    SwaySpeed = 0.3f;
    SwaySideRatio = 0.7f;
    HoverHeight = 50.0f;
    IdleMaterial = nullptr;
    ActiveMaterial = nullptr;
    VisualMaterialSlot = 0;
    ActiveMotionScale = 1.5f;
    bVisualActive = false;
    MotionTime = 0.0f;
    SpinAngle = 0.0f;

    // Still root, the mesh moves under it
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    SpawnerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SpawnerMesh"));
    SpawnerMesh->SetupAttachment(SceneRoot);
    SpawnerMesh->SetRelativeRotation(StandUpRotation);
    SpawnerMesh->SetRelativeLocation(FVector(0.0f, 0.0f, HoverHeight));

    // Bullets use overlap, so overlap events must be on
    SpawnerMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    SpawnerMesh->SetCollisionObjectType(ECC_Pawn);
    SpawnerMesh->SetGenerateOverlapEvents(true);

    // It moves every frame, so keep it out of the navmesh or it rebuilds forever
    SpawnerMesh->SetCanEverAffectNavigation(false);

    // Health lives on a component
    HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
}

FGenericTeamId ABaseSpawner::GetGenericTeamId() const
{
    return FGenericTeamId(TeamId);
}

void ABaseSpawner::SetGenericTeamId(const FGenericTeamId& NewTeamId)
{
    TeamId = NewTeamId.GetId();
}

// Called when the game starts or when spawned
void ABaseSpawner::BeginPlay()
{
    Super::BeginPlay();

    // Keep the mesh looking the way it was placed
    if (SpawnerMesh)
    {
        BaseMeshRotation = SpawnerMesh->GetRelativeRotation().Quaternion();
        BaseMeshLocation = SpawnerMesh->GetRelativeLocation();
    }

    // Bind death
    if (HealthComponent)
    {
        HealthComponent->OnDeath.AddDynamic(this, &ABaseSpawner::HandleDeath);
    }

    if (SpawnInterval <= 0.0f || MaxAlive <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("BaseSpawner: Invalid interval or enemy limit."));
        bVisualActive = false;
        ApplySpawnerMaterial();
        return;
    }

    // First spawn after FirstSpawnDelay, then every SpawnInterval
    GetWorldTimerManager().SetTimer(SpawnTimer, this,
        &ABaseSpawner::SpawnEnemy, SpawnInterval, true, FirstSpawnDelay);

    bVisualActive = IsSpawning();
    ApplySpawnerMaterial();
}

// Spins the mesh and gently bobs and sways it with sine waves
void ABaseSpawner::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (SpawnerMesh == nullptr)
    {
        return;
    }

    const bool bNowActive = IsSpawning();
    if (bNowActive != bVisualActive)
    {
        bVisualActive = bNowActive;
        ApplySpawnerMaterial();
    }

    const float Rate = bVisualActive ? ActiveMotionScale : 1.0f;

    MotionTime += DeltaTime;
    SpinAngle = FMath::Fmod(SpinAngle + SpinSpeed * Rate * DeltaTime, 360.0f);

    const float TwoPi = 2.0f * PI;
    const float Bob = FMath::Sin(MotionTime * BobSpeed * Rate * TwoPi) * BobHeight;
    const float SwayX = FMath::Sin(MotionTime * SwaySpeed * Rate * TwoPi) * SwayAmount;
    const float SwayY = FMath::Sin(MotionTime * SwaySpeed * Rate * TwoPi * SwaySideRatio) * SwayAmount;

    // Add movement to the placed location
    SpawnerMesh->SetRelativeLocation(BaseMeshLocation + FVector(SwayX, SwayY, Bob));

    // Spin around the mesh's local Z axis, keeping the placed orientation
    const FQuat Spin(FVector::UpVector, FMath::DegreesToRadians(SpinAngle));
    SpawnerMesh->SetRelativeRotation(BaseMeshRotation * Spin);
}

bool ABaseSpawner::IsSpawning() const
{
    if (!GetWorldTimerManager().IsTimerActive(SpawnTimer))
    {
        return false;
    }

    int32 Alive = 0;
    for (const TWeakObjectPtr<ABaseEnemy>& Enemy : SpawnedEnemies)
    {
        if (Enemy.IsValid() && !Enemy->IsDead())
        {
            ++Alive;
        }
    }

    return Alive < MaxAlive;
}

void ABaseSpawner::ApplySpawnerMaterial()
{
    if (SpawnerMesh == nullptr)
    {
        return;
    }

    UMaterialInterface* Material = bVisualActive ? ActiveMaterial : IdleMaterial;
    if (Material == nullptr)
    {
        return;
    }

    SpawnerMesh->SetMaterial(VisualMaterialSlot, Material);
}

// Stop the spawn timer when removed
void ABaseSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    GetWorldTimerManager().ClearTimer(SpawnTimer);
    Super::EndPlay(EndPlayReason);
}

// Spawns one enemy near the spawner
void ABaseSpawner::SpawnEnemy()
{
    // Forget enemies that are gone or dead so the cap stays right
    SpawnedEnemies.RemoveAll([](const TWeakObjectPtr<ABaseEnemy>& Enemy)
        {
            return !Enemy.IsValid() || Enemy->IsDead();
        });

    if (SpawnedEnemies.Num() >= MaxAlive)
    {
        return;
    }

    if (!EnemyClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("BaseSpawner: Set EnemyClass on the Blueprint."));
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const FVector SpawnLocation = GetActorLocation()
        + GetActorRotation().RotateVector(SpawnOffset);

    // Try adjusting a blocked spot, skip this attempt if it still won't fit
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

    // Keep only the spawner's yaw so enemies spawn upright
    const FRotator SpawnRotation(0.0f, GetActorRotation().Yaw, 0.0f);

    ABaseEnemy* Enemy = World->SpawnActor<ABaseEnemy>(
        EnemyClass, SpawnLocation, SpawnRotation, Params);

    if (!Enemy)
    {
        UE_LOG(LogTemp, Log, TEXT("BaseSpawner: Spawn blocked or failed; will retry."));
        return;
    }

    // Try creating a controller if auto possession didn't give us one
    if (!Enemy->GetController())
    {
        Enemy->SpawnDefaultController();
    }

    if (!Enemy->GetController())
    {
        UE_LOG(LogTemp, Warning, TEXT("BaseSpawner: No controller; removing enemy."));
        Enemy->Destroy();
        return;
    }

    // Same side as the spawner, so its enemies can't shoot it or each other
    Enemy->SetGenericTeamId(GetGenericTeamId());

    // Count it toward MaxAlive
    SpawnedEnemies.Add(Enemy);

    if (ARavenGameMode* GameMode = World->GetAuthGameMode<ARavenGameMode>())
    {
        GameMode->RegisterTarget(Enemy);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("BaseSpawner: Game mode is not ARavenGameMode. Spawned enemy was not counted."));
    }

}

// Shot down, stop spawning and remove the spawner
void ABaseSpawner::HandleDeath(float Ratio)
{
    GetWorldTimerManager().ClearTimer(SpawnTimer);

    // Explode where the visible mesh is (it bobs and sways), not at the still root
    const FVector BlastLocation = SpawnerMesh ? SpawnerMesh->GetComponentLocation() : GetActorLocation();

    // Niagara and Cascade effects are spawned by different functions
    if (UNiagaraSystem* NiagaraEffect = Cast<UNiagaraSystem>(DeathEffect))
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, NiagaraEffect, BlastLocation, GetActorRotation(), DeathEffectScale);
    }
    else if (UParticleSystem* CascadeEffect = Cast<UParticleSystem>(DeathEffect))
    {
        UGameplayStatics::SpawnEmitterAtLocation(this, CascadeEffect, BlastLocation, GetActorRotation(), DeathEffectScale);
    }

    if (DeathSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, DeathSound, BlastLocation, FRotator::ZeroRotator,
            1.0f, 1.0f, 0.0f, DeathSoundAttenuation);
    }

    // Tell the objective before Destroy
    if (UWorld* World = GetWorld())
    {
        if (ARavenGameMode* GameMode = World->GetAuthGameMode<ARavenGameMode>())
        {
            GameMode->TargetDestroyed(this);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("BaseSpawner: Game mode is not ARavenGameMode. Spawner death was not counted."));
        }
    }
    Destroy();
}
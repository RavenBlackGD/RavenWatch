// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GenericTeamAgentInterface.h"
#include "BaseSpawner.generated.h"

class ABaseEnemy;
class UStaticMeshComponent;
class UHealthComponent;
class USceneComponent;
class UMaterialInterface;
class UMaterialInterface;
class UNiagaraSystem;
class UFXSystemAsset;
class USoundBase;
class USoundAttenuation;

// Only the mesh spins and bobs, the root stays still
UCLASS()
class RAVENWATCH_API ABaseSpawner : public AActor, public IGenericTeamAgentInterface
{
    GENERATED_BODY()

public:

    // Sets default values for this actor's properties
    ABaseSpawner();

    // The spawner and every enemy it makes share this team
    virtual FGenericTeamId GetGenericTeamId() const override;
    virtual void SetGenericTeamId(const FGenericTeamId& NewTeamId) override;

protected:

    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

    // Stops the spawn timer when removed
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // Spins and bobs the mesh every frame
    virtual void Tick(float DeltaTime) override;

    // Still root
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Spawner)
    USceneComponent* SceneRoot;

    // Base mesh rotation, Tick adds the spin to this
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawner|Motion")
    FRotator StandUpRotation;

    // Degrees per second of spin
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawner|Motion")
    float SpinSpeed;

    // How far it bobs along the root's Z axis
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawner|Motion")
    float BobHeight;

    // Bob cycles per second
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawner|Motion")
    float BobSpeed;

    // How far it sways along the root's X and Y axes
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawner|Motion")
    float SwayAmount;

    // Base sway frequency, Y uses a slower rate
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawner|Motion")
    float SwaySpeed;

    // Y sway rate compared with SwaySpeed. 0.7 keeps the old side-to-side lag.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawner|Motion",
        meta = (ClampMin = "0.0"))
    float SwaySideRatio;

    // Mesh offset along the root's Z axis
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawner|Motion")
    float HoverHeight;

    // Shown while this spawner is waiting at MaxAlive.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawner|Visual")
    UMaterialInterface* IdleMaterial;

    // Shown while this spawner can still spawn.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawner|Visual")
    UMaterialInterface* ActiveMaterial;

    // Material slot on SpawnerMesh. 0 is the first slot.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawner|Visual",
        meta = (ClampMin = "0"))
    int32 VisualMaterialSlot;

    // 1 keeps the idle bob speed. Higher speeds up bob, sway, and spin while active.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawner|Motion",
        meta = (ClampMin = "1.0"))
    float ActiveMotionScale;

    // Visible mesh, spins and bobs and takes the bullet hits
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Spawner)
    UStaticMeshComponent* SpawnerMesh;

    // Health Component, so the spawner can be shot down
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Health)
    UHealthComponent* HealthComponent;

    // Team number, given to every enemy this spawner makes. 255 means no team.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Team, meta = (ClampMax = "254"))
    uint8 TeamId;

    // Enemy Blueprint to spawn (BP_BaseEnemy)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Spawner)
    TSubclassOf<ABaseEnemy> EnemyClass;

    // Seconds between spawn attempts
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Spawner,
        meta = (ClampMin = "0.1"))
    float SpawnInterval;

    // Seconds before the first spawn. Later spawns use SpawnInterval.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Spawner,
        meta = (ClampMin = "0.0"))
    float FirstSpawnDelay;

    // Where enemies appear, relative to the spawner
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Spawner)
    FVector SpawnOffset;

    // Most living enemies from this spawner at once
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Spawner,
        meta = (ClampMin = "1"))
    int32 MaxAlive;

    // Effect played where the spawner is destroyed: a Niagara system or a Cascade particle system. Leave empty for none.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawner|Death")
    UFXSystemAsset* DeathEffect;

    // Size of the death effect
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawner|Death")
    FVector DeathEffectScale;

    // Sound played where the spawner is destroyed. Leave empty for none.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawner|Death")
    USoundBase* DeathSound;

    // How the death sound fades with distance. Leave empty to use the sound's own settings.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawner|Death")
    USoundAttenuation* DeathSoundAttenuation;

    // Bound to HealthComponent OnDeath
    UFUNCTION()
    void HandleDeath(float Ratio);

private:

    // Timer handle for spawning
    FTimerHandle SpawnTimer;

    // Running time and spin angle for the motion
    float MotionTime;
    float SpinAngle;

    // To remember the placed mesh transform so motion doesn't replace it
    FQuat BaseMeshRotation;
    FVector BaseMeshLocation;

    // Last visual state, so the material swaps only when the state changes.
    bool bVisualActive;

    // True while the spawn timer is running and the living count is under MaxAlive.
    bool IsSpawning() const;

    // Applies IdleMaterial or ActiveMaterial. A null material leaves the mesh alone.
    void ApplySpawnerMaterial();

    // Tracks enemies without keeping destroyed ones alive
    TArray<TWeakObjectPtr<ABaseEnemy>> SpawnedEnemies;

    // Spawns one enemy if under the cap
    void SpawnEnemy();
};
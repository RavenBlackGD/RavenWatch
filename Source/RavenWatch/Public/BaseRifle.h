// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseRifle.generated.h"

// Fired when current or max ammo changes
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAmmoChangedSignature, int32, Current, int32, Max);

// Broadcast when a reload starts. The character plays the animation.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRifleReloadSignature);

/**
 * Weapon actor held by the player.
 * Spawns projectiles from the Muzzle point.
 */
UCLASS()
class RAVENWATCH_API ABaseRifle : public AActor
{
    GENERATED_BODY()

public:
    // Sets default values
    ABaseRifle();

    // Try to shoot one bullet
    UFUNCTION(BlueprintCallable, Category = Weapon)
    void Attack();

    // Refill the clip
    UFUNCTION(BlueprintCallable, Category = Weapon)
    void ReloadAmmo();

    // Play the sound when a reload starts
    UFUNCTION(BlueprintCallable, Category = Weapon)
    void PlayReloadSound();

    // Locks firing and tells the owner to play the reload animation.
    UFUNCTION(BlueprintCallable, Category = Weapon)
    void RequestReload();

    // Clears the reload lock. The ReloadEnded notify calls this.
    UFUNCTION(BlueprintCallable, Category = Weapon)
    void EndReload();

    UFUNCTION(BlueprintPure, Category = Weapon)
    bool IsReloading() const { return bReloading; }

    // HUD binds to this
    UPROPERTY(BlueprintAssignable, Category = Weapon)
    FOnAmmoChangedSignature OnAmmoChanged;

    // Owner binds this and plays ReloadAnimation.
    UPROPERTY(BlueprintAssignable, Category = Weapon)
    FOnRifleReloadSignature OnReloadStart;

	//Current ammo in the clip
    UFUNCTION(BlueprintPure, Category = Weapon)
    int32 GetCurrentAmmo() const { return CurrentAmmo; }

	//Max ammo in the clip
    UFUNCTION(BlueprintPure, Category = Weapon)
    int32 GetMaxAmmo() const { return MaxAmmo; }

protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

    // Placeholder gun mesh (root)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Weapon)
    class UStaticMeshComponent* RifleMesh;

    // Where bullets spawn
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Weapon)
    class USceneComponent* Muzzle;

    // Muzzle position in rifle-mesh space. X is forward along the gun.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon)
    FVector MuzzleOffset;

    // Bullet class to spawn (set to BP_Projectile)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon)
    TSubclassOf<AActor> ProjectileClass;

    // Bullets left in the clip
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Weapon)
    int32 CurrentAmmo;

    // Bullets in a full clip
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon)
    int32 MaxAmmo;

    // Seconds between shots
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon)
    float FireRate;

    // Sound when the rifle fires
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon)
    class USoundBase* FireSound;

    // Sound when the rifle reloads
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon)
    class USoundBase* ReloadSound;

    // How fire and reload sounds fade with distance. Leave empty and they play at full volume everywhere.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon)
    class USoundAttenuation* SoundAttenuation;

private:
    // True while waiting for the next shot
    bool bCoolingDown;

    // True from RequestReload until EndReload.
    bool bReloading;

    FTimerHandle FireTimerHandle;

    // Timer callback, allows shooting again
    void ResetCooldown();
};
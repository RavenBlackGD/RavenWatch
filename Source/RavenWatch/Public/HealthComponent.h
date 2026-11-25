// HealthComponent.h - holds health for any character

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

// Sends the health ratio (0 to 1) to whoever is listening
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHealthChanged, float, Ratio);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class RAVENWATCH_API UHealthComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    // Sets default values for this component's properties
    UHealthComponent();

    // Fires when health goes down but is still above zero
    UPROPERTY(BlueprintAssignable, Category = Health)
    FHealthChanged OnHurt;

    // Fires once when health hits zero
    UPROPERTY(BlueprintAssignable, Category = Health)
    FHealthChanged OnDeath;

    // Fires when health goes up. The hurt reaction does not play.
    UPROPERTY(BlueprintAssignable, Category = Health)
    FHealthChanged OnHeal;

    // Current health divided by max health
    UFUNCTION(BlueprintPure, Category = Health)
    float GetHealthRatio() const;

    UFUNCTION(BlueprintCallable, Category = Health)
    void SetMaxHealth(float NewMaxHealth);

    UFUNCTION(BlueprintCallable, Category = Health)
    void InitializeHealth(float NewMaxHealth);

protected:
    // Called when the game starts
    virtual void BeginPlay() override;

    // Max health, change it on the Blueprint if you want or lower projectile damage
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Health)
    float MaxHealth;

    // Current health
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Health)
    float CurrentHealth;

    // Bound to the owner's OnTakeAnyDamage
    UFUNCTION()
    void HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);
};
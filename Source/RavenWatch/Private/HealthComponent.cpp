// HealthComponent.cpp - holds health for any character

#include "HealthComponent.h"
#include "GameFramework/Actor.h"
#include "CombatComponent.h"

// Sets default values for this component's properties
UHealthComponent::UHealthComponent()
{
    // Health does not need Tick
    PrimaryComponentTick.bCanEverTick = false;

    MaxHealth = 100.0f;
    CurrentHealth = MaxHealth;
}

// Called when the game starts
void UHealthComponent::BeginPlay()
{
    Super::BeginPlay();

    // Start full
    CurrentHealth = MaxHealth;

    // Listen for damage on whoever owns this component
    AActor* Owner = GetOwner();
    if (Owner)
    {
        Owner->OnTakeAnyDamage.AddDynamic(this, &UHealthComponent::HandleTakeAnyDamage);
    }
    else
    {
        // Log after error checking
        UE_LOG(LogTemp, Error, TEXT("HealthComponent: Owner was null. Damage was not bound."));
    }
}

float UHealthComponent::GetHealthRatio() const
{
    if (MaxHealth <= 0.0f)
    {
        return 0.0f;
    }
    return CurrentHealth / MaxHealth;
}

void UHealthComponent::SetMaxHealth(float NewMaxHealth)
{
    const float OldMaxHealth = MaxHealth;

    MaxHealth = FMath::Max(1.0f, NewMaxHealth);

    CurrentHealth = FMath::Clamp(CurrentHealth, 0.0f, MaxHealth);

    if (!FMath::IsNearlyEqual(OldMaxHealth, MaxHealth))
    {
        OnHeal.Broadcast(GetHealthRatio());
    }
}

void UHealthComponent::InitializeHealth(float NewMaxHealth)
{
    MaxHealth = FMath::Max(1.0f, NewMaxHealth);
    CurrentHealth = MaxHealth;

    OnHeal.Broadcast(GetHealthRatio());
}

// Runs every time the owner takes damage
void UHealthComponent::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
    // Ignore zero damage or damage after death
    if (CurrentHealth <= 0.0f || Damage == 0.0f)
    {
        return;
    }

    float FinalDamage = Damage;

    if (Damage > 0.0f)
    {
        if (const UCombatComponent* Combat = GetOwner()->FindComponentByClass<UCombatComponent>())
        {
            FinalDamage = Combat->CalculateMitigatedDamage(Damage);
        }

        if (FinalDamage <= 0.0f)
        {
            return;
        }
    }

    // Lower health but never below zero or above max
    CurrentHealth = FMath::Clamp(CurrentHealth - FinalDamage, 0.0f, MaxHealth);

    if (FinalDamage < 0.0f)
    {
        OnHeal.Broadcast(GetHealthRatio());
        UE_LOG(LogTemp, Log, TEXT("HealthComponent: Healed. Health is now %f."), CurrentHealth);
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("HealthComponent: Took %f damage. Health is now %f."), FinalDamage, CurrentHealth);

    if (CurrentHealth <= 0.0f)
    {
        OnDeath.Broadcast(0.0f);
    }
    else
    {
        OnHurt.Broadcast(GetHealthRatio());
    }
}
// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatComponent.h"
#include "AttributeSystem.h"
#include "CombatAttributeSet.h"
#include "AttributeEffectComponent.h"
#include "AttributeSystemInterface.h"

// Sets default values for this component's properties
UCombatComponent::UCombatComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}

float UCombatComponent::CalculateMitigatedDamage(float IncomingDamage) const
{
	if (IncomingDamage <= 0.0f)
	{
		return IncomingDamage;
	}

	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return IncomingDamage;
	}

	const UAttributeSystem* Attributes = Owner->FindComponentByClass<UAttributeSystem>();

	if (!Attributes)
	{
		return IncomingDamage;
	}

	float Armor = 0.0f;
	float EvadeChance = 0.0f;

	Attributes->GetAttributeValue(UCombatAttributeSet::StaticClass(), TEXT("Armor"), Armor);

	Attributes->GetAttributeValue(UCombatAttributeSet::StaticClass(), TEXT("EvadeChance"), EvadeChance);

	const float ClampedEvadeChance = FMath::Clamp(EvadeChance, 0.0f, 75.0f);

	if (FMath::FRand() < ClampedEvadeChance / 100.0f)
	{
		return 0.0f;
	}

	const float EffectiveArmor = FMath::Max(0.0f, Armor);

	const float FinalDamage = IncomingDamage * (100.0f / (100.0f + EffectiveArmor));

	return FMath::Max(0.0f, FinalDamage);
}



// Called when the game starts
void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}


FCombatDamageResult UCombatComponent::CalculateAttack(float DefaultDamage) const
{
    FCombatDamageResult Result;

    float AttackDamage = DefaultDamage;
    float CriticalChance = 0.0f;
    float CriticalDamage = 150.0f;

    float FlatFire = 0.0f;
    float FlatCold = 0.0f;
    float FlatLightning = 0.0f;

    float IncreasedFire = 0.0f;
    float IncreasedCold = 0.0f;
    float IncreasedLightning = 0.0f;

    if (const AActor* Owner = GetOwner())
    {
        if (const UAttributeSystem* Attributes = Owner->FindComponentByClass<UAttributeSystem>())
        {
            const auto Combat = UCombatAttributeSet::StaticClass();

            Attributes->GetAttributeValue(Combat, TEXT("DamageAmount"), AttackDamage);

            Attributes->GetAttributeValue(Combat, TEXT("CriticalChance"), CriticalChance);

            Attributes->GetAttributeValue(Combat, TEXT("CriticalDamage"), CriticalDamage);

            Attributes->GetAttributeValue(Combat, TEXT("FlatFireDamage"), FlatFire);

            Attributes->GetAttributeValue(Combat, TEXT("FlatColdDamage"), FlatCold);

            Attributes->GetAttributeValue(Combat, TEXT("FlatLightningDamage"), FlatLightning);

            Attributes->GetAttributeValue(Combat, TEXT("IncreasedFireDamage"), IncreasedFire);

            Attributes->GetAttributeValue(Combat, TEXT("IncreasedColdDamage"), IncreasedCold);

            Attributes->GetAttributeValue(Combat, TEXT("IncreasedLightningDamage"), IncreasedLightning);
        }
    }

    // Base damage is currently treated as physical damage.
    Result.PhysicalDamage = FMath::Max(0.0f, AttackDamage);

    // Flat elemental damage scales with its respective increase.
    Result.FireDamage = FMath::Max(0.0f, FlatFire * (1.0f + IncreasedFire / 100.0f));

    Result.ColdDamage = FMath::Max(0.0f, FlatCold * (1.0f + IncreasedCold / 100.0f));

    Result.LightningDamage = FMath::Max(0.0f, FlatLightning * (1.0f + IncreasedLightning / 100.0f));

    // Roll critical strike once for the entire hit.
    const float ClampedCritChance = FMath::Clamp(CriticalChance, 0.0f, 100.0f);

    Result.bWasCritical = FMath::FRand() < (ClampedCritChance / 100.0f);

    if (Result.bWasCritical)
    {
        const float CritMultiplier = FMath::Max(0.0f, CriticalDamage) / 100.0f;

        Result.PhysicalDamage *= CritMultiplier;
        Result.FireDamage *= CritMultiplier;
        Result.ColdDamage *= CritMultiplier;
        Result.LightningDamage *= CritMultiplier;
    }

    Result.Damage = Result.PhysicalDamage + Result.FireDamage + Result.ColdDamage + Result.LightningDamage;

    return Result;
}


float UCombatComponent::GetAttackInterval(float BaseInterval) const
{
	float AttackSpeed = 100.0f;

	if (const AActor* Owner = GetOwner())
	{
		if (const UAttributeSystem* Attributes = Owner->FindComponentByClass<UAttributeSystem>())
		{
			Attributes->GetAttributeValue(UCombatAttributeSet::StaticClass(), TEXT("AttackSpeed"), AttackSpeed);
		}
	}

	const float SpeedMultiplier = FMath::Max(0.01f, AttackSpeed / 100.0f);

	return FMath::Max(0.001f, BaseInterval) / SpeedMultiplier;
}

bool UCombatComponent::ApplyDamageToActor(AActor* TargetActor, float DamageAmount)
{
	if (!TargetActor || DamageAmount <= 0.0f)
	{
		return false;
	}

	IAttributeSystemInterface* AttributeSystemInterface = Cast<IAttributeSystemInterface>(TargetActor);

	if (!AttributeSystemInterface)
	{
		return false;
	}

	UAttributeEffectComponent* AttributeEffectComponent = AttributeSystemInterface->GetAttributeEffectComponent();

	if (!AttributeEffectComponent)
	{
		return false;
	}

	AttributeEffectComponent->ApplyInstantModifier(UCombatAttributeSet::StaticClass(), TEXT("CurrentHealth"), EAttributeModifierOperation::Add, -DamageAmount);

	return true;
}


// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"

USTRUCT(BlueprintType)
struct FCombatDamageResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	float Damage = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	float PhysicalDamage = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	float FireDamage = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	float ColdDamage = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	float LightningDamage = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	bool bWasCritical = false;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class RAVENWATCH_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCombatComponent();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	float CalculateMitigatedDamage(float IncomingDamage) const;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	FCombatDamageResult CalculateAttack(float Defaultdamage) const;

	UFUNCTION(BlueprintPure, Category = "Combat")
	float GetAttackInterval(float BaseInterval) const;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool ApplyDamageToActor(AActor* TargetActor, float DamageAmount);
};

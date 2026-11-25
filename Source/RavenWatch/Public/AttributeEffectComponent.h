// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "AttributeEffectComponent.generated.h"

class UAttributeEffectDefinitions;
class UAttributeSystem;
struct FAttributeModifier;


USTRUCT(BlueprintType)
struct RAVENWATCH_API FActiveAttributeEffect
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FGuid Handle;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UAttributeEffectDefinitions> Definition = nullptr;

	// Runtime timer handle for duration effects
	FTimerHandle TimerHandle;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class RAVENWATCH_API UAttributeEffectComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UAttributeEffectComponent();

	UFUNCTION(BlueprintCallable, Category = "Effects")
	bool ApplyEffect(
		UAttributeEffectDefinitions* EffectDefinition,
		FGuid& OutEffectHandle);

	UFUNCTION(BlueprintCallable, Category = "Effects")
	bool ApplyInstantModifier(TSubclassOf<UAttributeSetBase> AttributeSetClass,
		FName AttributeName,
		EAttributeModifierOperation Operation,
		float Magnitude);

	bool ApplyInstantModifier(FAttributeModifier Modifier);

	UFUNCTION(BlueprintCallable, Category = "Effects")
	bool RemoveEffect(FGuid EffectHandle);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TObjectPtr<UAttributeSystem> AttributeSystem;

	UPROPERTY()
	TArray<FActiveAttributeEffect> ActiveEffects;

	UAttributeSystem* ResolveAttributeSystem() const;

	UFUNCTION()
	bool ApplyInstantEffect(UAttributeEffectDefinitions* EffectDefinition, FGuid& OutEffectHandle);

	bool ApplyPersistentEffect(UAttributeEffectDefinitions* EffectDefinition, FGuid& OutEffectHandle, bool bHasDuration);

	void ExpireEffect(FGuid EffectHandle);
};

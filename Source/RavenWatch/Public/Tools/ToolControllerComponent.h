// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Tools/PlayerToolBase.h"
#include "ToolControllerComponent.generated.h"

class ABasePlayer;
class UAttributeSystem;
class UAttributeEffectComponent;


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class RAVENWATCH_API UToolControllerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UToolControllerComponent();

	UFUNCTION(BlueprintCallable, Category = "Tools")
	void EquipToolOne(APlayerToolBase* NewTool);

	UFUNCTION(BlueprintCallable, Category = "Tools")
	bool UseToolOneAbility(int32 AbilityIndex);

	// Activate tool two when more tools are implemented
	//UFUNCTION(BlueprintCallable, Category = "Tools")
	//void EquipToolTwo(APlayerToolBase* NewTool);
	//
	//UFUNCTION(BlueprintCallable, Category = "Tools")
	//bool UseToolTwoAbility(int32 AbilityIndex);

	UFUNCTION(BlueprintPure, Category = "Tools")
	APlayerToolBase* GetEquippedToolOne() const
	{
		return EquippedToolOne;
	}

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UPROPERTY()
	TObjectPtr<APlayerToolBase> EquippedToolOne;

	//UPROPERTY()
	//TObjectPtr<APlayerToolBase> EquippedToolTwo;

private:
	UPROPERTY()
	TMap<int32, float> AbilityCooldownEndTimes;

	// reference to owning player character to access attributes like mana
	UPROPERTY()
	TObjectPtr<ABasePlayer> OwningPlayer;

	UPROPERTY()
	TObjectPtr<UAttributeSystem> PlayerAttributeSystem;

	UPROPERTY()
	TObjectPtr<UAttributeEffectComponent> PlayerEffectComponent;
};

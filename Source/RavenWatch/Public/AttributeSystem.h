// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AttributeEffectTypes.h"
#include "AttributeSystem.generated.h"

class UAttributeSetBase;
struct FAttribute;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnAttributeChanged, UAttributeSetBase*, AttributeSet, FName, AttributeName, float, OldValue, float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnAttributeBaseChanged, UAttributeSetBase*, AttributeSet, FName, AttributeName, float, OldValue, float, NewValue);


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class RAVENWATCH_API UAttributeSystem : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UAttributeSystem();

	UFUNCTION(BlueprintCallable, Category = "Attributes")
	bool AddAttributeSet(TSubclassOf<UAttributeSetBase> AttributeSetClass);

	UFUNCTION(BlueprintPure, Category = "Attributes")
	UAttributeSetBase* GetAttributeSet(TSubclassOf<UAttributeSetBase> AttributeSetClass) const;

	UFUNCTION(BlueprintPure, Category = "Attributes")
	bool HasAttributeSet(TSubclassOf<UAttributeSetBase> AttributeSetClass) const;

	template<typename T>
	T* GetAttributeSet() const
	{
		static_assert(TIsDerivedFrom<T, UAttributeSetBase>::IsDerived, "T must be derived from UAttributeSetBase");

		return Cast<T>(GetAttributeSet(T::StaticClass()));
	}

	const TArray<TObjectPtr<UAttributeSetBase>>& GetAttributeSets() const
	{
		return AttributeSets;
	}

	UFUNCTION(BlueprintPure, Category = "Attributes")
	bool GetAttributeValue(
		TSubclassOf<UAttributeSetBase> AttributeSetClass,
		FName AttributeName,
		float& OutValue) const;

	UFUNCTION(BlueprintPure, Category = "Attributes")
	bool GetBaseAttributeValue(
		TSubclassOf<UAttributeSetBase> AttributeSetClass,
		FName AttributeName,
		float& OutValue) const;

	UFUNCTION(BlueprintCallable, Category = "Attributes")
	bool SetCurrentAttributeValue(
		TSubclassOf<UAttributeSetBase> AttributeSetClass,
		FName AttributeName,
		float NewValue);

	UFUNCTION(BlueprintCallable, Category = "Attributes")
	bool SetBaseAttributeValue(
		TSubclassOf<UAttributeSetBase> AttributeSetClass,
		FName AttributeName,
		float NewValue);

	bool FindAttribute(
		TSubclassOf<UAttributeSetBase> AttributeSetClass,
		FName AttributeName,
		FAttribute*& OutAttribute);

	bool FindAttribute(
		TSubclassOf<UAttributeSetBase> AttributeSetClass,
		FName AttributeName,
		const FAttribute*& OutAttribute) const;

	UFUNCTION(BlueprintCallable, Category = "Attributes")
	bool ApplyModifier(
		TSubclassOf<UAttributeSetBase> AttributeSetClass,
		FName AttributeName,
		FGuid EffectHandle,
		EAttributeModifierOperation Operation,
		float Magnitude);


	UFUNCTION(BlueprintCallable, Category = "Attributes")
	bool RemoveModifier(
		TSubclassOf<UAttributeSetBase> AttributeSetClass,
		FName AttributeName,
		FGuid EffectHandle);

	UFUNCTION(BlueprintCallable, Category = "Attributes")
	bool RemoveModifiersByGuid(FGuid EffectHandle);

	UFUNCTION(BlueprintCallable, Category = "Attributes")
	bool ExecuteInstantModifier(
		TSubclassOf<UAttributeSetBase> AttributeSetClass,
		FName AttributeName,
		EAttributeModifierOperation Operation,
		float Magnitude);



	UPROPERTY(BlueprintAssignable, Category = "Attributes")
	FOnAttributeChanged OnAttributeChanged;

	UPROPERTY(BlueprintAssignable, Category = "Attributes")
	FOnAttributeBaseChanged OnAttributeBaseChanged;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "Attributes")
	TArray<TSubclassOf<UAttributeSetBase>> AttributeSetClasses;

	UPROPERTY(VisibleAnywhere, Instanced, Category = "Attributes")
	TArray<TObjectPtr<UAttributeSetBase>> AttributeSets;

private:
	void InitializeAttributeSets();

		
};

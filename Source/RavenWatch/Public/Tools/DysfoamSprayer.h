// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Tools/PlayerToolBase.h"
#include "DysfoamSprayer.generated.h"

class ADysfoamProjectile;
class ADysfoamShockwave;

/**
 * 
 */
UCLASS()
class RAVENWATCH_API ADysfoamSprayer : public APlayerToolBase
{
	GENERATED_BODY()
	
public:
	ADysfoamSprayer();

	virtual void InitializeTool() override;
	virtual void ActivateAbility(int32 AbilityIndex) override;


protected:
	virtual void BeginPlay() override;

private:
	AActor* GetAbilityOwner() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dysfoam",
		meta = (AllowPrivateAccess = "true"))
	TSubclassOf<ADysfoamProjectile> FoamProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dysfoam",
		meta = (AllowPrivateAccess = "true"))
	TSubclassOf<ADysfoamShockwave> ShockwaveClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dysfoam",
		meta = (AllowPrivateAccess = "true"))
	float SpawnDistance = 100.0f;


};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Projectile.h"
#include "Tools/ToolAttributeSet.h"
#include "DysfoamShockwave.generated.h"

/**
 * 
 */
UCLASS()
class RAVENWATCH_API ADysfoamShockwave : public AProjectile
{
	GENERATED_BODY()

public:
	ADysfoamShockwave();

	void InitializeShockwaveAttributes(UToolAttributeSet* InAttributes);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	virtual void HandleOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult) override;

private:
	static constexpr float StartRadius = 20.0f;

	void ApplyKnockback(
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent);

	bool IsPlayerActor(const AActor* Actor) const;

	UPROPERTY()
	TObjectPtr<UToolAttributeSet> ShockwaveAttributes;

	UPROPERTY()
	TSet<TObjectPtr<AActor>> AffectedActors;

	float ElapsedTime = 0.0f;
};
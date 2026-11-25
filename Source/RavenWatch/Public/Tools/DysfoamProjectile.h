// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Projectile.h"
#include "Tools/ToolAttributeSet.h"
#include "DysfoamProjectile.generated.h"

/**
 * 
 */
UCLASS()
class RAVENWATCH_API ADysfoamProjectile : public AProjectile
{
	GENERATED_BODY()
	
public:
	ADysfoamProjectile();

	void InitializeFoamAttributes(UToolAttributeSet* InAttributes);
	void ActivateSynergy();

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void HandleOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult) override;

	virtual void HandleOverlapEnd(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex) override;

	UFUNCTION()
	void HandleHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit);

private:
	void ApplyFoamEffect(AActor* Actor);
	void RemoveFoamEffect(AActor* Actor);
	bool IsIgnoredActor(const AActor* Actor) const;
	void AttachToSurface(
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		const FHitResult& Hit);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Foam|Collision",
		meta = (AllowPrivateAccess = "true"))
	TArray<FName> AttachmentIgnoreTags;

	bool HasAttachmentIgnoreTag(const AActor* Actor) const;

	UPROPERTY()
	TObjectPtr<UToolAttributeSet> FoamAttributes;

	UPROPERTY()
	TMap<TObjectPtr<AActor>, FGuid> ActiveSlowEffects;

	UPROPERTY()
	TObjectPtr<AActor> AttachedActor;

	UPROPERTY()
	TObjectPtr<UPrimitiveComponent> AttachedComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Foam|Attachment",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SurfaceEmbedFraction = 0.3f;

	bool bAttached = false;

};

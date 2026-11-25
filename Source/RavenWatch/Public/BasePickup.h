#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/HitResult.h"
#include "BasePickup.generated.h"

class UBoxComponent;

UCLASS(Abstract)
class RAVENWATCH_API ABasePickup : public AActor
{
	GENERATED_BODY()

public:
	ABasePickup();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Collision)
	UBoxComponent* BoxCollision;

	// True if this actor may collect the pickup
	virtual bool CanPickup(AActor* OtherActor) const;

	// The Effect - children override this
	virtual void HandlePickup(AActor* OtherActor, const FHitResult& SweepResult);

	// Default destroys. Win/Lose leave this empty so the box stays
	virtual void PostPickup();

private:
	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
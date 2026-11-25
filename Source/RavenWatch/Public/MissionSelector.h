// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractableObjectInterface.h"
#include "WBP_LevelSelect.h"
#include "MissionSelector.generated.h"

UCLASS()
class RAVENWATCH_API AMissionSelector : public AActor, public IInteractableObjectInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMissionSelector();



protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "Mesh")
	UStaticMeshComponent* MeshComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> LevelSelectClass;


public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// ---------------------------------------------- //
	// IInteractableObjectInterface
	// ---------------------------------------------- //

	void BeginFocus() override;

	void EndFocus() override;

	void BeginInteract() override;

	void EndInteract() override;

	void Interact() override;

};

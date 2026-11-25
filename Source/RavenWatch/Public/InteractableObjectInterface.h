// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractableObjectInterface.generated.h"


// This struct holds data related to an interaction, such as its name, description, and duration. We can add more if needed.
// Objects implementing this will have to define each component on an per-instance basis, which can be automated within the 
// object's functions as needed.
USTRUCT(BlueprintType)
struct FInteractionData
{
	GENERATED_USTRUCT_BODY()

	FInteractionData()
		: InteractionName(FText::FromString("Default Interaction"))
		, InteractionDescription(FText::FromString("This is a default interaction description."))
		, InteractionDuration(0.0f)
	{
	};

	UPROPERTY(EditInstanceOnly, Category = "Interaction")
	FText InteractionName;

	UPROPERTY(EditInstanceOnly, Category = "Interaction")
	FText InteractionDescription;

	UPROPERTY(EditInstanceOnly, Category = "Interaction")
	float InteractionDuration;
	


};

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UInteractableObjectInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class RAVENWATCH_API IInteractableObjectInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	
	virtual void BeginFocus() = 0;
	virtual void EndFocus() = 0;
	virtual void BeginInteract() = 0;
	virtual void EndInteract() = 0;
	virtual void Interact() = 0;

	FInteractionData InteractionData;
};

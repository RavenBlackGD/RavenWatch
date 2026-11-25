// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Tools/ToolAttributeSet.h"
#include "Tools/PlayerToolInterface.h"
#include "PlayerToolBase.generated.h"

USTRUCT(BlueprintType)
struct FToolAbility
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tool Ability|Description")
	FName AbilityName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tool Ability|Description")
	FText AbilityDescription;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tool Ability|Icon")
	TObjectPtr<UTexture2D> AbilityIcon = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, Category = "Tool Ability")
	TObjectPtr<UToolAttributeSet> AbilityAttributes = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tool Ability")
	bool bIsHeldAbility = false;

};

USTRUCT(BlueprintType)
struct FPlayerToolData
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Tool|Description")
	FName ToolName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Tool|Description")
	FText ToolDescription;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Tool|Icon")
	TObjectPtr<UTexture2D> ToolIcon = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Tool|Abilities")
	FToolAbility Ability1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Tool|Abilities")
	FToolAbility Ability2;

	FPlayerToolData()
		: ToolName(NAME_None)
		, ToolIcon(nullptr)
		, Ability1()
		, Ability2()
	{
	}

	UToolAttributeSet* GetAbilityAttributes(int32 AbilityIndex) const
	{
		switch (AbilityIndex)
		{
		case 1:
			return Ability1.AbilityAttributes;
		case 2:
			return Ability2.AbilityAttributes;
		default:
			return nullptr;
		}
	}

	FToolAbility* GetAbility(int32 AbilityIndex)
	{
		switch (AbilityIndex)
		{
		case 1:
			return &Ability1;
		case 2:
			return &Ability2;
		default:
			return nullptr;
		}
	}

	const FToolAbility* GetAbility(int32 AbilityIndex) const
	{
		switch (AbilityIndex)
		{
		case 1:
			return &Ability1;
		case 2:
			return &Ability2;
		default:
			return nullptr;
		}
	}
};


// All children of this base class should implement the IPlayerToolInterface to ensure that they can be used as a player tool
UCLASS(Abstract, Blueprintable)
class RAVENWATCH_API APlayerToolBase : public AActor, public IPlayerToolInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APlayerToolBase();

	// Set up tool data and attributes for the tool. 
	// MUST be overridden in children for proper initialization of the tool. 
	// Should be called in BeginPlay or after spawning the tool.
	virtual void InitializeTool();

	// returns the tool data for this tool
	virtual FPlayerToolData GetToolData() const override { return ToolData; }

	// sets the tool data for this tool, should be used for save/load functionality and not for initialization
	virtual void SetToolData(const FPlayerToolData& NewToolData) { ToolData = NewToolData; }

	// returns the ability attributes for the given ability index (1 or 2)
	virtual UToolAttributeSet* GetAbilityAttributes(int32 AbilityIndex) const override;

	// sets the ability attributes for the given ability index (1 or 2), should be used for save/load functionality and not for initialization
	virtual void SetAbilityAttributes(int32 AbilityIndex, UToolAttributeSet* NewAttributes);

	// returns the ability data for the given ability index (1 or 2)
	virtual FToolAbility GetAbilityData(int32 AbilityIndex) const override;

	// Activate the ability at the given index (1 or 2)
	// Could be expanded to support more abilities in the future
	virtual void ActivateAbility(int32 AbilityIndex) override;


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void EnsureAbilityAttributes();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Tool", meta = (AllowPrivateAccess = "true"))
	FPlayerToolData ToolData;

};

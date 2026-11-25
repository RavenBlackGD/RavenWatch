// Fill out your copyright notice in the Description page of Project Settings.


#include "MissionSelector.h"


// Sets default values
AMissionSelector::AMissionSelector()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;



}

// Called when the game starts or when spawned
void AMissionSelector::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AMissionSelector::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AMissionSelector::BeginFocus()
{
	if (MeshComponent)
	{
		MeshComponent->SetRenderCustomDepth(true);
	}
}

void AMissionSelector::EndFocus()
{
	if (MeshComponent)
	{
		MeshComponent->SetRenderCustomDepth(false);
	}
}

void AMissionSelector::BeginInteract()
{
	UE_LOG(LogTemp, Warning, TEXT("BeginInteract called on MissionSelector"));
}

void AMissionSelector::EndInteract()
{
	UE_LOG(LogTemp, Warning, TEXT("EndInteract called on MissionSelector"));
}

void AMissionSelector::Interact()
{
	if (!LevelSelectClass)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("LevelSelectClass is not assigned on the Mission Selector Blueprint.")
		);

		return;
	}

	// If we move to a multiplayer setup, we will have to change this implementation to move away from GetFirstPlayerController()
	// as it will not work in multiplayer.
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	APlayerController* PlayerController = World->GetFirstPlayerController();
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Error, TEXT("Unable to find the player controller for MissionSelector."));
		return;
	}

	UUserWidget* LevelSelectWidget = CreateWidget<UUserWidget>(
		PlayerController,
		LevelSelectClass
	);

	if (!LevelSelectWidget)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create the mission select widget."));
		return;
	}

	LevelSelectWidget->AddToViewport();

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(LevelSelectWidget->TakeWidget());

	PlayerController->SetInputMode(InputMode);
	PlayerController->bShowMouseCursor = true;
}

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "ReloadNow.generated.h"

// Sits on the reload sequence. Fills the clip when the magazine seats.
UCLASS()
class RAVENWATCH_API UReloadNow : public UAnimNotify
{
    GENERATED_BODY()

public:

    virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

    virtual FString GetNotifyName_Implementation() const override;
};
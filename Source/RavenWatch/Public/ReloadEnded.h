// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "ReloadEnded.generated.h"

// Sits at the end of the reload sequence. Clears the rifle lock.
UCLASS()
class RAVENWATCH_API UReloadEnded : public UAnimNotify
{
    GENERATED_BODY()

public:

    virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

    virtual FString GetNotifyName_Implementation() const override;
};
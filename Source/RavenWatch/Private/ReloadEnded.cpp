// Fill out your copyright notice in the Description page of Project Settings.

#include "ReloadEnded.h"
#include "CharacterAnimation.h"
#include "Components/SkeletalMeshComponent.h"

void UReloadEnded::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
    Super::Notify(MeshComp, Animation, EventReference);

    if (MeshComp == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("ReloadEnded: MeshComp was null."));
        return;
    }

    UCharacterAnimation* CharacterAnim = Cast<UCharacterAnimation>(MeshComp->GetAnimInstance());
    if (CharacterAnim == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("ReloadEnded: Anim instance was null or was not a CharacterAnimation."));
        return;
    }

    CharacterAnim->OnReloadEnded.Broadcast();
}

FString UReloadEnded::GetNotifyName_Implementation() const
{
    return TEXT("Reload Ended");
}
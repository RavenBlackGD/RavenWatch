// Fill out your copyright notice in the Description page of Project Settings.


#include "ReloadNow.h"
#include "CharacterAnimation.h"
#include "Components/SkeletalMeshComponent.h"

void UReloadNow::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
    Super::Notify(MeshComp, Animation, EventReference);

    if (MeshComp == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("ReloadNow: MeshComp was null."));
        return;
    }

    UCharacterAnimation* CharacterAnim = Cast<UCharacterAnimation>(MeshComp->GetAnimInstance());
    if (CharacterAnim == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("ReloadNow: Anim instance was null or was not a CharacterAnimation."));
        return;
    }

    CharacterAnim->OnReloadNow.Broadcast();
}

FString UReloadNow::GetNotifyName_Implementation() const
{
    return TEXT("Reload Now");
}
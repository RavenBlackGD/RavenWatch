// Fill out your copyright notice in the Description page of Project Settings.

#include "Survivor.h"
#include "HealthComponent.h"
#include "AIController.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerStart.h"
#include "Engine/DamageEvents.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

// Sets default values
ASurvivor::ASurvivor()
{
    // Survivors do not need Tick
    PrimaryActorTick.bCanEverTick = false;

    RescueHoldTime = 2.0f;
    RescuePrompt = NSLOCTEXT("Survivor", "RescuePrompt", "Rescue");
    bInvulnerable = true;
    TeamId = 0;
    bHuntedByEnemies = false;
    ThreatenedHoldTime = 1.0f;
    LastThreatenedTime = -1000.0f;
    RunSpeed = 450.0f;
    EscapePoint = nullptr;
    EscapeRadius = 150.0f;
    EscapePointTag = TEXT("SurvivorExit");
    TrappedAnim = nullptr;
    StandUpAnim = nullptr;
    DeathAnim = nullptr;
    CorpseLifeSpan = 5.0f;
    FullBodySlotName = TEXT("FullBody");
    StandUpBlendTime = 0.4f;
    StandUpStartTime = 0.0f;
    RunBlendTime = 0.3f;
    DeathBlendTime = 0.2f;
    FleePlayRate = 1.0f;
    FleeSwitchInterval = 0.0f;
    FleeSwitchBlendTime = 0.5f;
    FleeAnimIndex = 0;
    FleeRunAnim = nullptr;
    FleeRunClipSpeed = 0.0f;
    FleeLookSlotName = TEXT("FleeLook");
    State = ESurvivorState::Trapped;

    // Move the mesh down to the capsule bottom and face it forward, same as BaseEnemy
    GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
    GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

    // The player's interact trace uses the Visibility channel
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

    // Turn toward the direction they run
    bUseControllerRotationYaw = false;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->MaxWalkSpeed = RunSpeed;

    // Health
    HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));

    // A plain AI controller is enough to run to a point
    AIControllerClass = AAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

FGenericTeamId ASurvivor::GetGenericTeamId() const
{
    return FGenericTeamId(TeamId);
}

void ASurvivor::SetGenericTeamId(const FGenericTeamId& NewTeamId)
{
    TeamId = NewTeamId.GetId();
}

// Called when the game starts or when spawned
void ASurvivor::BeginPlay()
{
    Super::BeginPlay();

    // The interface data is not editable in the Details panel, so fill it from our settings
    InteractionData.InteractionName = RescuePrompt;
    InteractionData.InteractionDuration = RescueHoldTime;

    // Pawns ignore all damage while this is false, A hunted survivor can always be hurt,
    SetCanBeDamaged(!bInvulnerable || bHuntedByEnemies);

    if (HealthComponent)
    {
        HealthComponent->OnDeath.AddDynamic(this, &ASurvivor::HandleDeath);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Survivor: %s has no health component."), *GetName());
    }

    if (TrappedAnim)
    {
        // Loops until they are freed. No blend in so they start already on the ground.
        PlayFullBody(TrappedAnim, 0.0f, 0.0f, 0.0f, true, false);
    }

    // An empty slot in Flee Anims would break the switching, so drop any
    FleeAnims.RemoveAll([](const UAnimSequenceBase* Anim) { return Anim == nullptr; });
}

void ASurvivor::BeginFocus()
{
    // Only trapped survivors get the outline
    if (State == ESurvivorState::Trapped)
    {
        GetMesh()->SetRenderCustomDepth(true);
    }
}

void ASurvivor::EndFocus()
{
    GetMesh()->SetRenderCustomDepth(false);
}

void ASurvivor::BeginInteract()
{
    UE_LOG(LogTemp, Log, TEXT("Survivor: %s rescue started."), *GetName());
}

void ASurvivor::EndInteract()
{
    // The player let go of E. BasePlayer already cancels the hold timer.
}

void ASurvivor::SetHuntedByEnemies(bool bHunted)
{
    bHuntedByEnemies = bHunted;
    if (bHunted)
    {
        SetCanBeDamaged(true);
    }
}

float ASurvivor::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    const float Taken = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    if (Taken > 0.0f)
    {
        MarkThreatened();
    }
    return Taken;
}

void ASurvivor::MarkThreatened()
{
    if (const UWorld* World = GetWorld())
    {
        LastThreatenedTime = World->GetTimeSeconds();
    }
}

bool ASurvivor::IsThreatened() const
{
    const UWorld* World = GetWorld();
    return World && World->GetTimeSeconds() - LastThreatenedTime <= ThreatenedHoldTime;
}

void ASurvivor::Interact()
{
    // Hold finished
    if (State != ESurvivorState::Trapped)
    {
        return;
    }

    State = ESurvivorState::StandingUp;
    UE_LOG(LogTemp, Log, TEXT("Survivor: %s freed."), *GetName());

    // Stop the player's trace from finding us again
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
    EndFocus();

    if (StandUpAnim && StandUpAnim->GetPlayLength() > 0.0f)
    {
        const float PlayLength = StandUpAnim->GetPlayLength();
        const float StartAt = FMath::Clamp(StandUpStartTime, 0.0f, PlayLength);

        // Blends in from the trapped pose, and blends out into the run at the end
        PlayFullBody(StandUpAnim, StandUpBlendTime, RunBlendTime, StartAt, false, false);

        // Start running as the blend out begins, so the legs pick up the run smoothly
        const float RunDelay = PlayLength - StartAt - RunBlendTime;
        if (RunDelay > 0.0f)
        {
            GetWorldTimerManager().SetTimer(StandUpTimerHandle, this, &ASurvivor::StartFleeing, RunDelay, false);
        }
        else
        {
            StartFleeing();
        }
    }
    else
    {
        StartFleeing();
    }
}

void ASurvivor::StartFleeing()
{
    // They may have died while standing up
    if (State != ESurvivorState::StandingUp)
    {
        return;
    }

    State = ESurvivorState::Fleeing;

    // Hand the body back to the Anim Blueprint so the run blend space plays
    GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
    GetCharacterMovement()->MaxWalkSpeed = RunSpeed;

    // Whole body run without a gun, sped up or slowed so the feet match Run Speed
    if (FleeRunAnim)
    {
        const float RunRate = FleeRunClipSpeed > 0.0f ? RunSpeed / FleeRunClipSpeed : 1.0f;
        PlayFullBody(FleeRunAnim, RunBlendTime, RunBlendTime, 0.0f, true, false, RunRate);
    }

    // Look left and right while running, switching between the clips.
    // Start on a random one so a group of survivors does not move in sync.
    if (FleeAnims.Num() > 0)
    {
        FleeAnimIndex = FMath::RandRange(0, FleeAnims.Num() - 1);
        PlayNextFleeAnim();
    }

    OnSurvivorFreed.Broadcast(this);

    AAIController* AI = Cast<AAIController>(GetController());
    if (AI == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("Survivor: %s has no AI controller. Counted as escaped so the mission can finish."), *GetName());
        Escape();
        return;
    }

    AI->ReceiveMoveCompleted.AddUniqueDynamic(this, &ASurvivor::HandleMoveCompleted);

    const EPathFollowingRequestResult::Type Request =
        AI->MoveToLocation(FindEscapeLocation(), EscapeRadius, true, true, true);

    if (Request == EPathFollowingRequestResult::Failed)
    {
        UE_LOG(LogTemp, Warning, TEXT("Survivor: %s has no path to the escape point. Check NavMesh. Counted as escaped so the mission can finish."), *GetName());
        Escape();
    }
    else if (Request == EPathFollowingRequestResult::AlreadyAtGoal)
    {
        Escape();
    }
}

FVector ASurvivor::FindEscapeLocation() const
{
    if (EscapePoint)
    {
        return EscapePoint->GetActorLocation();
    }

    // The nearest exit marked with the tag
    if (!EscapePointTag.IsNone())
    {
        TArray<AActor*> Exits;
        UGameplayStatics::GetAllActorsWithTag(this, EscapePointTag, Exits);

        const AActor* Nearest = nullptr;
        float NearestDistSq = TNumericLimits<float>::Max();
        for (const AActor* Exit : Exits)
        {
            const float DistSq = FVector::DistSquared(Exit->GetActorLocation(), GetActorLocation());
            if (DistSq < NearestDistSq)
            {
                NearestDistSq = DistSq;
                Nearest = Exit;
            }
        }

        if (Nearest)
        {
            return Nearest->GetActorLocation();
        }
    }

    TActorIterator<APlayerStart> It(GetWorld());
    if (It)
    {
        return It->GetActorLocation();
    }

    UE_LOG(LogTemp, Warning, TEXT("Survivor: %s found no Escape Point and no Player Start."), *GetName());
    return GetActorLocation();
}

void ASurvivor::HandleMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result)
{
    if (State != ESurvivorState::Fleeing)
    {
        return;
    }

    if (Result != EPathFollowingResult::Success)
    {
        UE_LOG(LogTemp, Warning, TEXT("Survivor: %s stopped before the escape point. Counted as escaped."), *GetName());
    }

    Escape();
}

void ASurvivor::Escape()
{
    if (State == ESurvivorState::Escaped || State == ESurvivorState::Lost)
    {
        return;
    }

    State = ESurvivorState::Escaped;
    UE_LOG(LogTemp, Log, TEXT("Survivor: %s escaped."), *GetName());

    OnSurvivorFinished.Broadcast(this, true);

    // They left the map
    Destroy();
}

void ASurvivor::HandleDeath(float Ratio)
{
    if (State == ESurvivorState::Escaped || State == ESurvivorState::Lost)
    {
        return;
    }

    State = ESurvivorState::Lost;
    UE_LOG(LogTemp, Log, TEXT("Survivor: %s died."), *GetName());

    GetWorldTimerManager().ClearTimer(StandUpTimerHandle);
    GetWorldTimerManager().ClearTimer(FleeSwitchTimerHandle);

    if (AAIController* AI = Cast<AAIController>(GetController()))
    {
        AI->StopMovement();
    }

    GetCharacterMovement()->DisableMovement();
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    EndFocus();

    // The head look slot is in its own group, so the death will not stop it on its own
    if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
    {
        AnimInstance->StopSlotAnimation(DeathBlendTime, FleeLookSlotName);
    }

    // Blend into the death and hold the last frame so the body stays down
    PlayFullBody(DeathAnim, DeathBlendTime, 0.0f, 0.0f, false, true);

    OnSurvivorFinished.Broadcast(this, false);

    SetLifeSpan(CorpseLifeSpan);
}

UAnimMontage* ASurvivor::PlayFullBody(UAnimSequenceBase* Anim, float BlendIn, float BlendOut, float StartTime, bool bLoop, bool bHoldLastFrame, float PlayRate, FName SlotOverride)
{
    if (Anim == nullptr)
    {
        return nullptr;
    }

    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
    if (AnimInstance == nullptr)
    {

        // No Anim Blueprint, so blending is not possible. Play it the old way.
        UE_LOG(LogTemp, Warning, TEXT("Survivor: %s has no Anim Blueprint. Playing %s without blending."), *GetName(), *Anim->GetName());
        GetMesh()->PlayAnimation(Anim, bLoop);
        return nullptr;

    }

    const FName Slot = SlotOverride.IsNone() ? FullBodySlotName : SlotOverride;

    UAnimMontage* Montage = AnimInstance->PlaySlotAnimationAsDynamicMontage(
        Anim, Slot, BlendIn, BlendOut, PlayRate, 1, -1.0f, StartTime);

    if (Montage == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("Survivor: %s could not play %s. Check the %s slot exists in the Anim Blueprint."),
            *GetName(), *Anim->GetName(), *Slot.ToString());
        return nullptr;
    }

    if (bLoop)
    {

        // A dynamic montage has one section. Pointing it at itself loops forever.
        const FName Section = Montage->GetSectionName(0);
        AnimInstance->Montage_SetNextSection(Section, Section, Montage);

    }

    if (bHoldLastFrame)
    {

        // Stop it from blending back to the standing idle when it ends
        Montage->bEnableAutoBlendOut = false;

    }

    return Montage;
}

void ASurvivor::PlayNextFleeAnim()
{
    if (State != ESurvivorState::Fleeing || FleeAnims.Num() == 0)
    {
        return;
    }

    UAnimSequenceBase* FleeAnim = FleeAnims[FleeAnimIndex];
    FleeAnimIndex = (FleeAnimIndex + 1) % FleeAnims.Num();

    // With a run clip, the look clips only drive the head. Without one, they drive the whole body like before.
    const FName LookSlot = FleeRunAnim ? FleeLookSlotName : FullBodySlotName;

    // Loops underneath, so a late switch never drops back to the gun run
    PlayFullBody(FleeAnim, FleeSwitchBlendTime, FleeSwitchBlendTime, 0.0f, true, false, FleePlayRate, LookSlot);

    // One clip just keeps looping
    if (FleeAnims.Num() < 2)
    {
        return;
    }

    // Switch after the interval, or after one pass of the clip. Start the crossfade early so it finishes on time.
    const float ClipTime = FleeAnim->GetPlayLength() / FMath::Max(FleePlayRate, 0.1f);
    const float Interval = FleeSwitchInterval > 0.0f ? FleeSwitchInterval : ClipTime;
    const float Delay = FMath::Max(Interval - FleeSwitchBlendTime, 0.1f);

    GetWorldTimerManager().SetTimer(FleeSwitchTimerHandle, this, &ASurvivor::PlayNextFleeAnim, Delay, false);
}
// Fill out your copyright notice in the Description page of Project Settings.

#include "BaseEnemy.h"
#include "Equipment/EquipmentPickup.h"
#include "PotionPickup.h"
#include "BaseEnemyController.h"
#include "BaseRifle.h"
#include "HealthComponent.h"
#include "CharacterAnimation.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "AttributeSystem.h"
#include "AttributeEffectComponent.h"
#include "CombatComponent.h"
#include "CombatAttributeSet.h"
#include "Components/WidgetComponent.h"

// Sets default values
ABaseEnemy::ABaseEnemy()
{
    // The controller does the thinking, no Tick needed
    PrimaryActorTick.bCanEverTick = false;

    WeaponSocketName = TEXT("WeaponSocket");
    TeamId = 1;
    RifleClass = nullptr;
    Rifle = nullptr;
    AttackCooldown = 0.6f;
    HitAsset = nullptr;
    DeathAsset = nullptr;
    ActionSlotName = TEXT("ActionSlot");
    CorpseLifeSpan = 5.0f;
    WalkSpeed = 500.0f;
    DropChance = 1.0f;
    DropOffset = FVector(0.0f, 0.0f, 40.0f);
    bDead = false;
    LastAttackTime = -1000.0f;

    // Move the mesh down to the capsule bottom and face it forward
    GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
    GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

    // Face where the controller looks instead of turning toward movement
    bUseControllerRotationYaw = true;
    GetCharacterMovement()->bOrientRotationToMovement = false;
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

    // Health
    HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));

    //Attribute System
    AttributeSystem = CreateDefaultSubobject<UAttributeSystem>(TEXT("AttributeSystem"));
    AttributeEffectComponent = CreateDefaultSubobject<UAttributeEffectComponent>(TEXT("AttributeEffectComponent"));

    //Combat component
    CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));

    //Health Bar
    HealthBarWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarWidget"));

    HealthBarWidget->SetupAttachment(GetRootComponent());
    HealthBarWidget->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
    HealthBarWidget->SetWidgetSpace(EWidgetSpace::Screen);
    HealthBarWidget->SetDrawSize(FVector2D(150.0f, 16.0f));
    HealthBarWidget->SetPivot(FVector2D(0.5f, 0.5f));
    HealthBarWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // AI
    AIControllerClass = ABaseEnemyController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

FGenericTeamId ABaseEnemy::GetGenericTeamId() const
{
    return FGenericTeamId(TeamId);
}

void ABaseEnemy::SetGenericTeamId(const FGenericTeamId& NewTeamId)
{
    TeamId = NewTeamId.GetId();
}

// Called when the game starts or when spawned
void ABaseEnemy::BeginPlay()
{
    Super::BeginPlay();

    if (AttributeSystem)
    {
        if (!AttributeSystem->HasAttributeSet(UCombatAttributeSet::StaticClass()))
        {
            AttributeSystem->AddAttributeSet(UCombatAttributeSet::StaticClass());
        }

        AttributeSystem->OnAttributeChanged.AddDynamic(this, &ABaseEnemy::HandleAttributeChanged);
    }

    InitializeEnemyScaling();

    if (HealthBarWidget)
    {
        HealthBarWidget->SetVisibility(false);
    }

    // Bind health events
    if (HealthComponent)
    {
        HealthComponent->OnHurt.AddDynamic(this, &ABaseEnemy::HandleHurt);
        HealthComponent->OnDeath.AddDynamic(this, &ABaseEnemy::HandleDeath);
        HealthComponent->OnHeal.AddDynamic(this, &ABaseEnemy::HandleHeal);

        UpdateEnemyHealthBar(HealthComponent->GetHealthRatio());
    }

    // Spawn the rifle and put it in the hand socket
    if (RifleClass)
    {
        FActorSpawnParameters Params;
        Params.Owner = this;
        Params.Instigator = this;

        Rifle = GetWorld()->SpawnActor<ABaseRifle>(
            RifleClass, GetActorTransform(), Params);

        if (Rifle)
        {
            Rifle->AttachToComponent(GetMesh(),
                FAttachmentTransformRules::SnapToTargetNotIncludingScale,
                WeaponSocketName);

            Rifle->OnReloadStart.AddDynamic(this, &ABaseEnemy::HandleReloadStarted);

            if (UCharacterAnimation* Anim = GetCharacterAnimation())
            {
                Anim->OnReloadNow.AddDynamic(this, &ABaseEnemy::HandleReloadNow);
                Anim->OnReloadEnded.AddDynamic(this, &ABaseEnemy::HandleReloadEnded);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("BaseEnemy: Anim instance was null. Reload notifies were not bound."));
            }

        }
        else
        {
            // Log after error checking
            UE_LOG(LogTemp, Warning, TEXT("BaseEnemy: Rifle failed to spawn."));
        }
    }
    else
    {
        // Log after error checking
        UE_LOG(LogTemp, Warning, TEXT("BaseEnemy: Set RifleClass on the Blueprint."));
    }
}

// The rifle is its own actor, so remove it
void ABaseEnemy::EndPlay(const EEndPlayReason::Type EndPlayReason)
{

    if (IsValid(Rifle))
    {
        Rifle->Destroy();
    }
    Rifle = nullptr;

    Super::EndPlay(EndPlayReason);
}

// Shoot one bullet, gated by dead, reloading, cooldown and ammo
void ABaseEnemy::Attack()
{
    if (bDead || !IsValid(Rifle) || Rifle->IsReloading())
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const float Now = World->GetTimeSeconds();

    const float EffectiveCooldown = CombatComponent ? CombatComponent->GetAttackInterval(AttackCooldown) : AttackCooldown;

    if (Now - LastAttackTime < EffectiveCooldown)
    {
        return;
    }

    // Empty clip, reload instead
    if (Rifle->GetCurrentAmmo() <= 0)
    {
        Reload();
        return;
    }

    // Only play the fire anim if the rifle used ammo
    const int32 AmmoBefore = Rifle->GetCurrentAmmo();
    Rifle->Attack();

    if (Rifle->GetCurrentAmmo() < AmmoBefore)
    {
        LastAttackTime = Now;

        if (UCharacterAnimation* Anim = GetCharacterAnimation())
        {
            Anim->FireAnimation();
        }
    }
}

// Play the reload anim, the timer refills the clip
void ABaseEnemy::Reload()
{
    if (bDead || !IsValid(Rifle))
    {
        return;
    }

    Rifle->RequestReload();
}

void ABaseEnemy::HandleReloadNow()
{
    if (!bDead && IsValid(Rifle))
    {
        Rifle->ReloadAmmo();
    }
}

void ABaseEnemy::HandleReloadEnded()
{
    if (IsValid(Rifle))
    {
        Rifle->EndReload();
    }
}

void ABaseEnemy::HandleAttributeChanged(UAttributeSetBase* AttributeSet, FName AttributeName, float OldValue, float NewValue)
{
    if (AttributeName == TEXT("MovementSpeed") && AttributeSet && AttributeSet->IsA<UCombatAttributeSet>())
    {
        ApplyMovementSpeed();
    }
}

void ABaseEnemy::HandleHeal(float Ratio)
{
    if (!bDead)
    {
        UpdateEnemyHealthBar(Ratio);
    }
}

void ABaseEnemy::HandleReloadStarted()
{
    if (bDead)
    {
        if (IsValid(Rifle))
        {
            Rifle->EndReload();
        }
        return;
    }

    UCharacterAnimation* Anim = GetCharacterAnimation();
    if (Anim && Anim->ReloadAnimation())
    {
        if (IsValid(Rifle))
        {
            Rifle->PlayReloadSound();
        }
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("BaseEnemy: Reload animation did not play. Rifle lock cleared."));
    if (IsValid(Rifle))
    {
        Rifle->EndReload();
    }
}

// Plays the hit anim in the ActionSlot
void ABaseEnemy::HandleHurt(float Ratio)
{

    if (bDead)
    {
        return;
    }

    if (HealthBarWidget)
    {
        HealthBarWidget->SetVisibility(true);
    }

    UpdateEnemyHealthBar(Ratio);

    UE_LOG(LogTemp, Log, TEXT("BaseEnemy: Health ratio %f."), Ratio);

    if (UCharacterAnimation* Anim = GetCharacterAnimation())
    {
        if (HitAsset)
        {
            Anim->PlaySlotAnimationAsDynamicMontage(HitAsset, ActionSlotName);
        }
    }
}

void ABaseEnemy::SetWandering(bool bWandering)
{
    bIsWandering = bWandering;
    ApplyMovementSpeed();
}

void ABaseEnemy::ApplyMovementSpeed()
{
    float MovementSpeed = 100.0f;

    if (AttributeSystem)
    {
        AttributeSystem->GetAttributeValue(UCombatAttributeSet::StaticClass(), TEXT("MovementSpeed"), MovementSpeed);
    }

    const float BaseSpeed = bIsWandering ? WanderSpeed : WalkSpeed;

    GetCharacterMovement()->MaxWalkSpeed = BaseSpeed * FMath::Max(0.0f, MovementSpeed) / 100.0f;
}

// Stops the AI, plays death, removes the body later
void ABaseEnemy::HandleDeath(float Ratio)
{
    if (bDead)
    {
        return;
    }

    bDead = true;

    UpdateEnemyHealthBar(0.0f);

    if (HealthBarWidget)
    {
        HealthBarWidget->SetVisibility(false);
    }

    if (IsValid(Rifle))
    {
        Rifle->EndReload();
    }

    GetCharacterMovement()->StopMovementImmediately();
    GetCharacterMovement()->DisableMovement();

    // Bullets and the player pass through the body
    SetActorEnableCollision(false);

    // Lets go of the AI controller so it stops thinking
    DetachFromControllerPendingDestroy();

    if (DeathAsset)
    {
        GetMesh()->PlayAnimation(DeathAsset, false);
    }

    UE_LOG(LogTemp, Log, TEXT("BaseEnemy: Died."));

    DropLoot();

    // Engine destroys us after this many seconds
    SetLifeSpan(FMath::Max(CorpseLifeSpan, 0.1f));
}

const FGearDropEntry* ABaseEnemy::ChooseDrop() const
{
    float TotalWeight = 0.0f;

    for (const FGearDropEntry& Entry : DropTable)
    {
        if (Entry.Mesh && Entry.DropWeight > 0.0f)
        {
            TotalWeight += Entry.DropWeight;
        }
    }

    if (TotalWeight <= 0.0f)
    {
        return nullptr;
    }

    float Roll = FMath::FRandRange(0.0f, TotalWeight);

    for (const FGearDropEntry& Entry : DropTable)
    {
        if (!Entry.Mesh || Entry.DropWeight <= 0.0f)
        {
            continue;
        }

        Roll -= Entry.DropWeight;
        if (Roll <= 0.0f)
        {
            return &Entry;
        }
    }

    return nullptr;
}

// Spawns one weighted gear mesh. BeginPlay on the pickup rolls the stats.
void ABaseEnemy::DropLoot()
{
    if (FMath::FRand() > DropChance)
    {
        return;
    }

    const FGearDropEntry* Chosen = ChooseDrop();
        if (Chosen == nullptr)
        {
            UE_LOG(LogTemp, Warning, TEXT("BaseEnemy: DropTable has no weighted mesh. No item spawned."));
            return;
        }

    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("BaseEnemy: GetWorld() was null. No item spawned."));
        return;
    }

    const FVector SpawnLocation = GetActorLocation() + DropOffset;
    const FRotator SpawnRotation = GetActorRotation();
    const FTransform SpawnTransform(SpawnRotation, SpawnLocation);

    if (Chosen->Kind == EEnemyDropKind::Potion)
    {
        if (PotionClass == nullptr)
        {
            UE_LOG(LogTemp, Warning, TEXT("BaseEnemy: PotionClass is none. No potion spawned."));
            return;
        }

        APotionPickup* Spawned = World->SpawnActorDeferred<APotionPickup>(
            PotionClass, SpawnTransform, this, nullptr,
            ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);

        if (Spawned == nullptr)
        {
            UE_LOG(LogTemp, Warning, TEXT("BaseEnemy: Potion drop failed to spawn."));
            return;
        }

        Spawned->PrepareDrop(Chosen->Mesh, Chosen->Effect, Chosen->HealAmount, Chosen->SpeedMultiplier, Chosen->EffectDuration, Chosen->ManaAmount);
        UE_LOG(LogTemp, Log, TEXT("BaseEnemy: Dropped %s."), *Chosen->ItemName.ToString());
        Spawned->FinishSpawning(SpawnTransform);
        return;
    }

    if (DropClass == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("BaseEnemy: DropClass is none. No item spawned."));
        return;
    }

    AEquipmentPickup* Spawned = World->SpawnActorDeferred<AEquipmentPickup>(
        DropClass, SpawnTransform, this, nullptr,
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);

    if (Spawned == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("BaseEnemy: Item drop failed to spawn."));
        return;
    }

    Spawned->PrepareDrop(Chosen->Mesh, Chosen->Slot, Chosen->ItemName);
    UE_LOG(LogTemp, Log, TEXT("BaseEnemy: Dropped %s."), *Chosen->ItemName.ToString());
    Spawned->FinishSpawning(SpawnTransform);
}

// Gets the C++ anim instance
UCharacterAnimation* ABaseEnemy::GetCharacterAnimation() const
{
    if (!GetMesh())
    {
        return nullptr;
    }

    return Cast<UCharacterAnimation>(GetMesh()->GetAnimInstance());
}

void ABaseEnemy::RollEnemyRarity()
{
    if (bOverrideRarity)
    {
        EnemyRarity = ForcedRarity;
        return;
    }

    const float Normal = FMath::Max(0.0f, NormalWeight);
    const float Magic = FMath::Max(0.0f, MagicWeight);
    const float Rare = FMath::Max(0.0f, RareWeight);

    const float TotalWeight = Normal + Magic + Rare;

    if (TotalWeight <= 0.0f)
    {
        EnemyRarity = EEnemyRarity::Normal;
        return;
    }

    const float Roll = FMath::FRand() * TotalWeight;

    if (Roll < Normal)
    {
        EnemyRarity = EEnemyRarity::Normal;
    }
    else if (Roll < Normal + Magic)
    {
        EnemyRarity = EEnemyRarity::Magic;
    }
    else
    {
        EnemyRarity = EEnemyRarity::Rare;
    }
}

void ABaseEnemy::InitializeEnemyScaling()
{
    if (!AttributeSystem)
    {
        return;
    }

    RollEnemyRarity();

    switch (EnemyDifficulty)
    {
    case EEnemyDifficulty::Level1:
        ApplyScalingProfile(Difficulty1Scaling);
        break;

    case EEnemyDifficulty::Level2:
        ApplyScalingProfile(Difficulty2Scaling);
        break;

    case EEnemyDifficulty::Level3:
        ApplyScalingProfile(Difficulty3Scaling);
        break;

    case EEnemyDifficulty::Level4:
        ApplyScalingProfile(Difficulty4Scaling);
        break;

    case EEnemyDifficulty::Level5:
        ApplyScalingProfile(Difficulty5Scaling);
        break;
    }

    switch (EnemyRarity)
    {
    case EEnemyRarity::Normal:
        ApplyScalingProfile(NormalScaling);
        break;

    case EEnemyRarity::Magic:
        ApplyScalingProfile(MagicScaling);
        break;

    case EEnemyRarity::Rare:
        ApplyScalingProfile(RareScaling);
        break;
    }

    float ScaledMaxHealth = 100.0f;

    if (HealthComponent && AttributeSystem->GetAttributeValue(UCombatAttributeSet::StaticClass(), TEXT("MaxHealth"), ScaledMaxHealth))
    {
        HealthComponent->InitializeHealth(ScaledMaxHealth);
    }

    ApplyMovementSpeed();

    UE_LOG(LogTemp, Log, TEXT("Enemy %s initialized. Difficulty: %d, Rarity: %d"), *GetName(), static_cast<int32>(EnemyDifficulty) + 1, static_cast<int32>(EnemyRarity));
}

void ABaseEnemy::ApplyScalingProfile(const FEnemyScalingProfile& Profile)
{
    if (!AttributeSystem)
    {
        return;
    }

    for (const FEnemyStatMultiplier& Entry : Profile.StatMultipliers)
    {
        const FName AttributeName = GetScaledAttributeName(Entry.Attribute);

        if (AttributeName.IsNone())
        {
            continue;
        }

        float BaseValue = 0.0f;

        if (!AttributeSystem->GetBaseAttributeValue(UCombatAttributeSet::StaticClass(), AttributeName, BaseValue))
        {
            continue;
        }

        const float ScaledValue = BaseValue * FMath::Max(0.0f, Entry.Multiplier);

        AttributeSystem->SetBaseAttributeValue(UCombatAttributeSet::StaticClass(), AttributeName, ScaledValue);
    }
}

FName ABaseEnemy::GetScaledAttributeName(EEnemyScaledStat Stat)
{
    switch (Stat)
    {
    case EEnemyScaledStat::MaxHealth:
        return TEXT("MaxHealth");

    case EEnemyScaledStat::Armor:
        return TEXT("Armor");

    case EEnemyScaledStat::DamageAmount:
        return TEXT("DamageAmount");

    case EEnemyScaledStat::AttackSpeed:
        return TEXT("AttackSpeed");

    case EEnemyScaledStat::CriticalChance:
        return TEXT("CriticalChance");

    case EEnemyScaledStat::CriticalDamage:
        return TEXT("CriticalDamage");

    case EEnemyScaledStat::MovementSpeed:
        return TEXT("MovementSpeed");

    case EEnemyScaledStat::EvadeChance:
        return TEXT("EvadeChance");

    default:
        return NAME_None;
    }
}

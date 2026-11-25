// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Equipment/EquipmentTypes.h"
#include "Engine/StaticMesh.h"
#include "PotionPickup.h"
#include "GenericTeamAgentInterface.h"
#include "BaseEnemy.generated.h"

class ABaseRifle;
class UHealthComponent;
class UCharacterAnimation;
class UAnimSequence;
class AEquipmentPickup;
class UAttributeSystem;
class UAttributeEffectComponent;
class UCombatComponent;
class UAttributeSetBase;
class UWidgetComponent;

UENUM(BlueprintType)
enum class EEnemyDropKind : uint8
{
    Gear UMETA(DisplayName = "Gear"),
    Potion UMETA(DisplayName = "Potion")
};

USTRUCT(BlueprintType)
struct FGearDropEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drop")
    EEnemyDropKind Kind = EEnemyDropKind::Gear;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drop")
    UStaticMesh* Mesh = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drop",
        meta = (EditCondition = "Kind == EEnemyDropKind::Gear", EditConditionHides))
    EEquipmentSlot Slot = EEquipmentSlot::Head;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drop")
    FName ItemName = NAME_None;

    // Which potion this row is. Gear rows ignore it.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drop",
        meta = (EditCondition = "Kind == EEnemyDropKind::Potion", EditConditionHides))
    EPotionEffect Effect = EPotionEffect::Health;

    // Trauma kit only.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drop",
        meta = (ClampMin = "0.0", EditCondition = "Kind == EEnemyDropKind::Potion && Effect == EPotionEffect::Health", EditConditionHides))
    float HealAmount = 75.0f;

    // Syringe only. Multiplies walk and sprint.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drop",
        meta = (ClampMin = "0.0", EditCondition = "Kind == EEnemyDropKind::Potion && Effect == EPotionEffect::Speed", EditConditionHides))
    float SpeedMultiplier = 1.5f;

    // Syringe only.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drop",
        meta = (ClampMin = "0.0", EditCondition = "Kind == EEnemyDropKind::Potion && Effect == EPotionEffect::Speed", EditConditionHides))
    float EffectDuration = 15.0f;

    // Saline flask only.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drop",
        meta = (ClampMin = "0.0", EditCondition = "Kind == EEnemyDropKind::Potion && Effect == EPotionEffect::Mana", EditConditionHides))
    float ManaAmount = 40.0f;

    // 0 skips this row. A value of 5 is five times as common as a value of 1.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drop",
        meta = (ClampMin = "0.0"))
    float DropWeight = 1.0f;
};

UENUM(BlueprintType)
enum class EEnemyRarity : uint8
{
    Normal,
    Magic,
    Rare
};

UENUM(BlueprintType)
enum class EEnemyDifficulty : uint8
{
    Level1 UMETA(DisplayName = "Difficulty 1"),
    Level2 UMETA(DisplayName = "Difficulty 2"),
    Level3 UMETA(DisplayName = "Difficulty 3"),
    Level4 UMETA(DisplayName = "Difficulty 4"),
    Level5 UMETA(DisplayName = "Difficulty 5")
};

UENUM(BlueprintType)
enum class EEnemyScaledStat : uint8
{
    MaxHealth,
    Armor,
    DamageAmount,
    AttackSpeed,
    CriticalChance,
    CriticalDamage,
    MovementSpeed,
    EvadeChance
};

USTRUCT(BlueprintType)
struct FEnemyStatMultiplier
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scaling")
    EEnemyScaledStat Attribute = EEnemyScaledStat::MaxHealth;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scaling", meta = (ClampMin = "0.0"))
    float Multiplier = 1.0f;
};

USTRUCT(BlueprintType)
struct FEnemyScalingProfile
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scaling")
    TArray<FEnemyStatMultiplier> StatMultipliers;
};

// The controller does the thinking, this class does the actions
UCLASS()
class RAVENWATCH_API ABaseEnemy : public ACharacter, public IGenericTeamAgentInterface
{
    GENERATED_BODY()

public:

    // Sets default values for this character's properties
    ABaseEnemy();

    // Which side this enemy is on. Bullets ignore anything on the shooter's team.
    virtual FGenericTeamId GetGenericTeamId() const override;
    virtual void SetGenericTeamId(const FGenericTeamId& NewTeamId) override;

    // Shoot one bullet if cooldown and ammo allow
    UFUNCTION(BlueprintCallable, Category = Enemy)
    void Attack();

    // Asks the rifle to reload. The animation notify refills the clip.
    UFUNCTION(BlueprintCallable, Category = Enemy)
    void Reload();

    // True after health hits zero
    UFUNCTION(BlueprintPure, Category = Enemy)
    bool IsDead() const { return bDead; }

    // Turn wandering on or off for this enemy
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Wander")
    bool bCanWander = true;

    // How far from its current spot the enemy will pick a point, in centimeters
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Wander",
        meta = (ClampMin = "0.0"))
    float WanderRadius = 1500.0f;

    // Centimeters per second while wandering. Chasing uses Walk Speed.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Wander",
        meta = (ClampMin = "0.0"))
    float WanderSpeed = 180.0f;

    // Switches between wander speed and chase speed
    void SetWandering(bool bWandering);

    // Shortest pause between walks, in seconds
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Wander",
        meta = (ClampMin = "0.0"))
    float WanderWaitMin = 1.0f;

    // Longest pause between walks, in seconds
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Wander",
        meta = (ClampMin = "0.0"))
    float WanderWaitMax = 3.0f;

protected:

    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

    // Removes the rifle and stops timers when we leave the level
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // Health Component
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Health)
    UHealthComponent* HealthComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes")
    UAttributeSystem* AttributeSystem;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes")
    UAttributeEffectComponent* AttributeEffectComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
    UCombatComponent* CombatComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
    UWidgetComponent* HealthBarWidget;

    // Team number. Same team can't hurt each other. 255 means no team. Spawners overwrite this with their own.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Team, meta = (ClampMax = "254"))
    uint8 TeamId;

    // Rifle Blueprint
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon)
    TSubclassOf<ABaseRifle> RifleClass;

    // Socket on the mesh the rifle attaches to
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon)
    FName WeaponSocketName;

    // The rifle
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Weapon)
    ABaseRifle* Rifle;

    // Seconds between shots
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon,
        meta = (ClampMin = "0.01"))
    float AttackCooldown;

    // Hit animation asset, plays in the ActionSlot
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Animation)
    UAnimSequence* HitAsset;

    // Death animation asset, plays on the full body
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Animation)
    UAnimSequence* DeathAsset;

    // Slot name for upper body blend, must match the ABP
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Animation)
    FName ActionSlotName;

    // Seconds the body stays after death
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Enemy,
        meta = (ClampMin = "0.1"))
    float CorpseLifeSpan;

    // Centimeters per second while chasing. Written onto CharacterMovement.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Enemy,
        meta = (ClampMin = "0.0"))
    float WalkSpeed;

    // Pickup spawned on death. Set this to BP_EquipmentPickup.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Enemy)
    TSubclassOf<AEquipmentPickup> DropClass;

    // Pickup spawned for a potion row. Set this to BP_PotionPickup.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Enemy)
    TSubclassOf<APotionPickup> PotionClass;

    // 1 drops every kill. 0 drops nothing.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Enemy,
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float DropChance;

    // Meshes this enemy can drop. Each row has its own Drop Weight.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Enemy)
    TArray<FGearDropEntry> DropTable;

    // Added to the enemy location when the pickup spawns.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Enemy)
    FVector DropOffset;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Difficulty")
    EEnemyDifficulty EnemyDifficulty = EEnemyDifficulty::Level1;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Enemy|Rarity")
    EEnemyRarity EnemyRarity = EEnemyRarity::Normal;

    //Manual rarity overrides for testing
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Rarity")
    bool bOverrideRarity = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Rarity")
    EEnemyRarity ForcedRarity = EEnemyRarity::Normal;

    //Enemy rarity default weightings
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Rarity", meta = (Config = "0.0"))
    float NormalWeight = 85.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Rarity", meta = (Config = "0.0"))
    float MagicWeight = 12.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Rarity", meta = (Config = "0.0"))
    float RareWeight = 3.0f;

    //Rarity scaling profiles
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Rarity|Scaling")
    FEnemyScalingProfile NormalScaling;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Rarity|Scaling")
    FEnemyScalingProfile MagicScaling;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Rarity|Scaling")
    FEnemyScalingProfile RareScaling;

    //Difficulty scaling profiles
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Difficulty|Scaling")
    FEnemyScalingProfile Difficulty1Scaling;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Difficulty|Scaling")
    FEnemyScalingProfile Difficulty2Scaling;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Difficulty|Scaling")
    FEnemyScalingProfile Difficulty3Scaling;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Difficulty|Scaling")
    FEnemyScalingProfile Difficulty4Scaling;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Difficulty|Scaling")
    FEnemyScalingProfile Difficulty5Scaling;

    // Bound to HealthComponent OnHurt
    UFUNCTION()
    void HandleHurt(float Ratio);

    // Bound to HealthComponent OnDeath
    UFUNCTION()
    void HandleDeath(float Ratio);

    // Bound to Rifle OnReloadStart
    UFUNCTION()
    void HandleReloadStarted();

    // Bound to CharacterAnimation OnReloadNow
    UFUNCTION()
    void HandleReloadNow();

    // Bound to CharacterAnimation OnReloadEnded
    UFUNCTION()
    void HandleReloadEnded();

    UFUNCTION()
    void HandleAttributeChanged(UAttributeSetBase* AttributeSet, FName AttributeName, float OldValue, float NewValue);

    UFUNCTION()
    void HandleHeal(float Ratio);

    UFUNCTION(BlueprintImplementableEvent, Category = "UI")
    void UpdateEnemyHealthBar(float HealthRatio);

    void ApplyMovementSpeed();

private:

    // So we never shoot while dead or reloading
    bool bDead;

    // Game time of the last shot, for the cooldown
    float LastAttackTime;

    bool bIsWandering = false;

    // Spawns DropClass when this enemy dies
    void DropLoot();

    // Weighted pick from DropTable. Null when every weight is 0.
    const FGearDropEntry* ChooseDrop() const;

    // Our C++ anim instance, can be null
    UCharacterAnimation* GetCharacterAnimation() const;

    void RollEnemyRarity();
    void InitializeEnemyScaling();

    void ApplyScalingProfile(const FEnemyScalingProfile& Profile);

    static FName GetScaledAttributeName(EEnemyScaledStat Stat);
};
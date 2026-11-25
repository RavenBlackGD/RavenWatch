// BasePlayer.h - our third person player

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "InteractableObjectInterface.h"
#include "AttributeSystemInterface.h"
#include "PotionPickup.h"
#include "GenericTeamAgentInterface.h"
#include "BasePlayer.generated.h"

class UInputMappingContext;
class UInputAction;
class UWBP_PauseMenu;
class UEquipmentComponent;
class UAttributeSystem;
class UAttributeEffectComponent;
class UAttributeSetBase;
class UCombatComponent;
class UInventoryComponent;
class UToolControllerComponent;
class UUserWidget;

USTRUCT(BlueprintType)
struct FInteractInfo
{
	GENERATED_USTRUCT_BODY()

	FInteractInfo()
		: CurrentInteractable(nullptr)
		, LastInteractionCheckTime(0.0f)
	{
	};

	UPROPERTY(VisibleAnywhere)
	AActor* CurrentInteractable;

	UPROPERTY(VisibleAnywhere)
	float LastInteractionCheckTime;
};

UCLASS()
class RAVENWATCH_API ABasePlayer : public ACharacter, public IAttributeSystemInterface, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:

	// Sets default values for this character's properties
	ABasePlayer();

	// Which side the player is on. Bullets ignore anything on the shooter's team.
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamId) override;

protected:

	// Team number. Survivors share it so your bullets pass through them. 255 means no team.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Team, meta = (ClampMax = "254"))
	uint8 TeamId;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;

	// Enhanced Input Assets
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* ActionMove;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* ActionLook;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* ActionJump;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* ActionSprint;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* ActionFire;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* ActionReload;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* ActionDamageTest;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* ActionPause;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* ActionInteract;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* ActionTool1Ability1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* ActionTool1Ability2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* ActionTool2Ability1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* ActionTool2Ability2;

	// I - open or close the inventory
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* ActionInventory;

	// Spring arm holds the camera behind the player
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	class USpringArmComponent* SpringArm;

	// The camera the player sees through
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	class UCameraComponent* Camera;

	// Health Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Health)
	class UHealthComponent* HealthComponent;

	// Collects AEquipmentPickup overlaps
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Equipment)
	class UEquipmentComponent* EquipmentComponent;

	// Inventory Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UInventoryComponent> InventoryComponent;

	// Rifle Blueprint
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon)
	TSubclassOf<class ABaseRifle> RifleClass;

	// Tool Blueprint (temporary, will be transfered to ToolControllerComponent)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Tool)
	TSubclassOf<class APlayerToolBase> ToolClass;

	// Socket on the mesh the rifle attaches to
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon)
	FName WeaponSocketName;

	// The rifle
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Weapon)
	class ABaseRifle* Rifle;

	// HUD widget
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = HUD)
	TSubclassOf<class UPlayerHUD> HUDClass;

	// The HUD
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = HUD)
	class UPlayerHUD* HUDWidget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes")
	TObjectPtr<UAttributeSystem> AttributeSystem;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes")
	TObjectPtr<UAttributeEffectComponent> AttributeEffectComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UCombatComponent> CombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ToolController")
	TObjectPtr<UToolControllerComponent> ToolController;


	// Damage the DamageTest input deals through ApplyDamage.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Testing",
		meta = (ClampMin = "0.0"))
	float TestDamageAmount;

	// Centimeters per second while walking. Written onto CharacterMovement.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Movement,
		meta = (ClampMin = "0.0"))
	float WalkSpeed;

	// Centimeters per second while Sprint is held.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Movement,
		meta = (ClampMin = "0.0"))
	float SprintSpeed;

	// Game time of the last mana spend. Mana itself lives on UPlayerAttributeSet.
	float LastManaUseTime;

	// True while Left Shift is held.
	bool bSprinting;

	// True while a syringe timer is running.
	bool bSpeedBoosted;

	// Multiplies walk or sprint. The syringe row supplies 1.5.
	float SpeedBoostMultiplier;

	UPROPERTY()
	FTimerHandle SpeedBoostTimer;

	// Interaction Members
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Interaction")
	float InteractionCheckInterval; // How often to check for interactables, edit default values in the editor

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Interaction")
	float InteractionCheckDistance; // How far to check for interactables, edit default values in the editor

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = HUD)
	TSubclassOf<UWBP_PauseMenu> PauseMenuClass;

	// Pause Menu instance
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = HUD)
	UWBP_PauseMenu* PauseMenuWidget;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	TSubclassOf<UUserWidget> InventoryWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> InventoryWidget;

	UPROPERTY()
	FTimerHandle InteractionCheckTimerHandle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	FInteractInfo InteractionInfo;

	UPROPERTY(VisibleAnywhere, Category = "Interaction")
	TScriptInterface<IInteractableObjectInterface> TargetInteractable;

	void ApplyMoveSpeed();

	UFUNCTION()
	void HandleAttributeChanged(UAttributeSetBase* ChangedSet, FName AttributeName, float OldValue, float NewValue);

	bool ApplySpeedBoost(float Multiplier, float Duration);

	// Timer target. Same SetTimer form as Interact.
	UFUNCTION()
	void ClearSpeedBoost();

	// Reads one value from UPlayerAttributeSet. 0 when it is missing.
	float GetPlayerAttribute(FName AttributeName) const;

	UFUNCTION(BlueprintPure, Category = "Mana")
	float GetMaxMana() const;

	UFUNCTION(BlueprintPure, Category = "Mana")
	float GetCurrentMana() const;

	// Adds to CurrentMana on the attribute set. False when already full.
	bool AddMana(float Amount);

	// Refills mana once ManaRegenDelay has passed since the last spend
	void RegenMana(float DeltaTime);

	// Pushes current / max mana to the HUD bar
	void UpdateManaHUD();

	// Fire
	void InputActionFire();

	// Reload
	void InputActionReload();

	// K - hurts us so we can test health
	void InputActionDamageTest();

	// Escape - pause the game
	void InputPauseGame();

	// I - Open inventory
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void InputToggleInventory();

	// 1 - Tool 1 Ability 1
	void InputTool1Ability1();
	void InputTool1Ability1Released();

	// 2 - Tool 1 Ability 2
	void InputTool1Ability2();
	void InputTool1Ability2Released();

	// 3 - Tool 2 Ability 1
	void InputTool2Ability1();
	void InputTool2Ability1Released();

	// 4 - Tool 2 Ability 2
	void InputTool2Ability2();
	void InputTool2Ability2Released();

	// Bound to HealthComponent OnHurt
	UFUNCTION()
	void HandleHurt(float Ratio);

	// Bound to HealthComponent OnHeal
	UFUNCTION()
	void HandleHeal(float Ratio);

	// Bound to HealthComponent OnDeath
	UFUNCTION()
	void HandleDeath(float Ratio);

	// Bound to Rifle OnAmmoChanged
	UFUNCTION()
	void HandleAmmoChanged(int32 Current, int32 Max);

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
	void CheckForInteractables();

	UFUNCTION()
	void FoundInteractable(AActor* Interactable);

	UFUNCTION()
	void NoInteractableFound();

	UFUNCTION()
	void BeginInteract();

	UFUNCTION()
	void EndInteract();

	UFUNCTION()
	void Interact();

public:
	// Binds our inputs
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// Aim from the rifle toward the crosshair destination
	virtual FRotator GetBaseAimRotation() const override;

	// Enhanced Input Callback Functions
	void Move(const FInputActionValue& Value);

	void Look(const FInputActionValue& Value);

	void StartSprint(const FInputActionValue& Value);

	void StopSprint(const FInputActionValue& Value);

	void Fire(const FInputActionValue& Value);

	void Reload(const FInputActionValue& Value);

	void DamageTest(const FInputActionValue& Value);

	void PauseGame(const FInputActionValue& Value);

	void StartInteract(const FInputActionValue& Value);

	void StopInteract(const FInputActionValue& Value);

	void ToggleInventory(const FInputActionValue& Value);

	// Health, speed, or mana. False leaves the pickup on the ground.
	bool ApplyPotion(EPotionEffect Effect, float Amount, float Duration);

	bool IsInteracting() const { return GetWorldTimerManager().IsTimerActive(InteractionCheckTimerHandle); }

	void SetHUDVisible(bool bVisible);

	// ---------------------------------------------//
	// IAttributeSystemInterface implementation
	// ---------------------------------------------//

	UAttributeSystem* GetAttributeSystem() const override { return AttributeSystem; }

	UAttributeEffectComponent* GetAttributeEffectComponent() const override { return AttributeEffectComponent; }

};
// BasePlayer.cpp - our third person player

#include "BasePlayer.h"
#include "CharacterAnimation.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/InputComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "HealthComponent.h"
#include "BaseRifle.h"
#include "PlayerHUD.h"
#include "Equipment/EquipmentComponent.h"
#include "DrawDebugHelpers.h"
#include "WBP_PauseMenu.h"
#include "AttributeSystem.h"
#include "AttributeEffectComponent.h"
#include "AttributeSetBase.h"
#include "CombatAttributeSet.h"
#include "PlayerAttributeSet.h"
#include "CombatComponent.h"
#include "CodeGameInstance.h"
#include "InventoryComponent.h"
#include "Blueprint/UserWidget.h"
#include "Tools/ToolControllerComponent.h"

// Might remove these includes for tool saving to the component later
#include "Tools/PlayerToolBase.h"

// Sets default values
ABasePlayer::ABasePlayer()
{
	PrimaryActorTick.bCanEverTick = true;
	TeamId = 0;

	//Attach ability system component and attribute set
	AttributeSystem = CreateDefaultSubobject<UAttributeSystem>(TEXT("AttributeSystem"));
	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
	TestDamageAmount = 10.0f;
	WalkSpeed = 600.0f;
	SprintSpeed = 900.0f;
	LastManaUseTime = 0.0f;
	bSprinting = false;
	bSpeedBoosted = false;
	SpeedBoostMultiplier = 1.0f;

	WeaponSocketName = TEXT("WeaponSocket");
	RifleClass = nullptr;
	Rifle = nullptr;
	HUDClass = nullptr;
	HUDWidget = nullptr;

	// Move the mesh down to the capsule bottom and face it forward
	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

	// Character faces where the camera looks so strafe anims play.
	bUseControllerRotationYaw = true;
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->bOrientRotationToMovement = false;
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("BasePlayer: CharacterMovement was null."));
	}

	// Create the spring arm and attach it to the root (the capsule)
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	if (SpringArm)
	{
		SpringArm->SetupAttachment(GetRootComponent());
		SpringArm->TargetArmLength = 300.0f;
		SpringArm->SetRelativeLocation(FVector(0.0f, 80.0f, 90.0f));
		SpringArm->bUsePawnControlRotation = true;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("BasePlayer: SpringArm CreateDefaultSubobject failed."));
	}

	// Create the camera and attach it to the spring arm
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	if (Camera && SpringArm)
	{
		Camera->SetupAttachment(SpringArm);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("BasePlayer: Camera was not created or SpringArm was null."));
	}

	// Health lives on a component
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	if (HealthComponent == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("BasePlayer: HealthComponent CreateDefaultSubobject failed."));
	}

	// Pickup overlaps look for this component
	EquipmentComponent = CreateDefaultSubobject<UEquipmentComponent>(TEXT("EquipmentComponent"));
	if (EquipmentComponent == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("BasePlayer: EquipmentComponent CreateDefaultSubobject failed."));
	}

	ToolController = CreateDefaultSubobject<UToolControllerComponent>(TEXT("ToolController"));
	if (ToolController == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("BasePlayer: ToolController CreateDefaultSubobject failed."));
	}

	AttributeEffectComponent = CreateDefaultSubobject<UAttributeEffectComponent>(TEXT("AttributeEffectComponent"));
	if (AttributeEffectComponent == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("BasePlayer: AttributeEffectComponent CreateDefaultSubobject failed."));
	}
}

FGenericTeamId ABasePlayer::GetGenericTeamId() const
{
	return FGenericTeamId(TeamId);
}

void ABasePlayer::SetGenericTeamId(const FGenericTeamId& NewTeamId)
{
	TeamId = NewTeamId.GetId();
}

// Called when the game starts or when spawned
void ABasePlayer::BeginPlay()
{
	Super::BeginPlay();

	// Add Enhanced Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}

	if (AttributeSystem)
	{
		AttributeSystem->OnAttributeChanged.AddDynamic(this, &ABasePlayer::HandleAttributeChanged);
	}

	bool bSessionRestored = false;

	if (UCodeGameInstance* GameInstance = GetGameInstance<UCodeGameInstance>())
	{
		bSessionRestored = GameInstance->ApplyPlayerSession(this);
	}

	float InitialMaxHealth = 100.0f;

	if (AttributeSystem && AttributeSystem->GetAttributeValue(UCombatAttributeSet::StaticClass(), TEXT("MaxHealth"), InitialMaxHealth))
	{
		if (HealthComponent)
		{
			HealthComponent->InitializeHealth(InitialMaxHealth);
		}
	}

	ApplyMoveSpeed();

	// Spawn the rifle and put it in the hand socket
	if (RifleClass)
	{
		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.Instigator = this;

		Rifle = GetWorld()->SpawnActor<ABaseRifle>(RifleClass, GetActorTransform(), Params);
		if (Rifle)
		{
			Rifle->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponSocketName);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("BasePlayer: Rifle failed to spawn."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("BasePlayer: RifleClass is none. Set it on the Blueprint."));
	}

	if (Rifle)
	{
		Rifle->OnReloadStart.AddDynamic(this, &ABasePlayer::HandleReloadStarted);
	}

	if (UCharacterAnimation* Anim = Cast<UCharacterAnimation>(GetMesh()->GetAnimInstance()))
	{
		Anim->OnReloadNow.AddDynamic(this, &ABasePlayer::HandleReloadNow);
		Anim->OnReloadEnded.AddDynamic(this, &ABasePlayer::HandleReloadEnded);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("BasePlayer: Anim instance was null. Reload notifies were not bound."));
	}

	// Create the HUD
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (HUDClass && PlayerController)
	{
		HUDWidget = CreateWidget<UPlayerHUD>(PlayerController, HUDClass);
		if (HUDWidget)
		{
			HUDWidget->AddToViewport();
			if (HealthComponent)
			{
				HUDWidget->SetHealth(HealthComponent->GetHealthRatio());
			}

			UpdateManaHUD();

			if (Rifle)
			{
				HUDWidget->SetAmmo(Rifle->GetCurrentAmmo(), Rifle->GetMaxAmmo());
				Rifle->OnAmmoChanged.AddDynamic(this, &ABasePlayer::HandleAmmoChanged);
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("BasePlayer: HUD widget failed to create."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("BasePlayer: HUDClass is none or no PlayerController."));
	}

	// Bind health events
	if (HealthComponent)
	{
		HealthComponent->OnHurt.AddDynamic(this, &ABasePlayer::HandleHurt);
		HealthComponent->OnHeal.AddDynamic(this, &ABasePlayer::HandleHeal);
		HealthComponent->OnDeath.AddDynamic(this, &ABasePlayer::HandleDeath);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("BasePlayer: HealthComponent was null. Health binds skipped."));
	}

	// Spawn the tool and equip it. Implementation will change when we have multiple tools and a tool equip system.
	if (ToolController)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = this;

		if (ToolClass)
		{
			APlayerToolBase* Tool = GetWorld()->SpawnActor<APlayerToolBase>(ToolClass, GetActorTransform(), SpawnParams);
			if (Tool)
			{
				Tool->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponSocketName);

				Tool->InitializeTool();
				ToolController->EquipToolOne(Tool);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("BasePlayer: Tool failed to spawn."));
			}
		}
	}

	InteractionInfo.LastInteractionCheckTime = 0.0f;
}

// Aim from the rifle toward the HUD crosshair destination
FRotator ABasePlayer::GetBaseAimRotation() const
{
	if (HUDWidget == nullptr)
	{
		return Super::GetBaseAimRotation();
	}

	const FVector Destination = HUDWidget->GetDestination();
	if (Destination.IsNearlyZero())
	{
		return Super::GetBaseAimRotation();
	}

	const FVector Source = Rifle
		? Rifle->GetActorLocation()
		: GetActorLocation();

	const FVector AimDirection = Destination - Source;
	if (AimDirection.IsNearlyZero())
	{
		return Super::GetBaseAimRotation();
	}

	return AimDirection.Rotation();
}

void ABasePlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	RegenMana(DeltaTime);

	if (GetWorld()->GetTimeSeconds() - InteractionInfo.LastInteractionCheckTime > InteractionCheckInterval)
	{
		CheckForInteractables();
	}

	// Fill the hold bar while E is held on a timed interactable
	if (HUDWidget)
	{
		if (IsInteracting())
		{
			const float Duration = GetWorldTimerManager().GetTimerRate(InteractionCheckTimerHandle);
			const float Elapsed = GetWorldTimerManager().GetTimerElapsed(InteractionCheckTimerHandle);
			HUDWidget->SetInteractProgress(Duration > 0.0f ? Elapsed / Duration : 0.0f);
		}
		else
		{
			HUDWidget->ClearInteractProgress();
		}
	}
}

// Binds actions via Enhanced Input
void ABasePlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (ActionMove)
		{
			EnhancedInputComponent->BindAction(ActionMove, ETriggerEvent::Triggered, this, &ABasePlayer::Move);
		}
		if (ActionLook)
		{
			EnhancedInputComponent->BindAction(ActionLook, ETriggerEvent::Triggered, this, &ABasePlayer::Look);
		}
		if (ActionJump)
		{
			EnhancedInputComponent->BindAction(ActionJump, ETriggerEvent::Started, this, &ABasePlayer::Jump);
			EnhancedInputComponent->BindAction(ActionJump, ETriggerEvent::Completed, this, &ABasePlayer::StopJumping);
		}
		if (ActionSprint)
		{
			EnhancedInputComponent->BindAction(ActionSprint, ETriggerEvent::Started, this, &ABasePlayer::StartSprint);
			EnhancedInputComponent->BindAction(ActionSprint, ETriggerEvent::Completed, this, &ABasePlayer::StopSprint);
		}
		if (ActionFire)
		{
			EnhancedInputComponent->BindAction(ActionFire, ETriggerEvent::Started, this, &ABasePlayer::Fire);
		}
		if (ActionReload)
		{
			EnhancedInputComponent->BindAction(ActionReload, ETriggerEvent::Started, this, &ABasePlayer::Reload);
		}
		if (ActionDamageTest)
		{
			EnhancedInputComponent->BindAction(ActionDamageTest, ETriggerEvent::Started, this, &ABasePlayer::DamageTest);
		}
		if (ActionPause)
		{
			EnhancedInputComponent->BindAction(ActionPause, ETriggerEvent::Started, this, &ABasePlayer::PauseGame);
		}
		if (ActionInteract)
		{
			EnhancedInputComponent->BindAction(ActionInteract, ETriggerEvent::Started, this, &ABasePlayer::StartInteract);
			EnhancedInputComponent->BindAction(ActionInteract, ETriggerEvent::Completed, this, &ABasePlayer::StopInteract);
		}
		if (ActionInventory)
		{
			EnhancedInputComponent->BindAction(ActionInventory, ETriggerEvent::Started, this, &ABasePlayer::InputToggleInventory);
		}
		if (ActionTool1Ability1)
		{
			EnhancedInputComponent->BindAction(ActionTool1Ability1, ETriggerEvent::Started, this, &ABasePlayer::InputTool1Ability1);
			EnhancedInputComponent->BindAction(ActionTool1Ability1, ETriggerEvent::Completed, this, &ABasePlayer::InputTool1Ability1Released);
		}
		if (ActionTool1Ability2)
		{
			EnhancedInputComponent->BindAction(ActionTool1Ability2, ETriggerEvent::Started, this, &ABasePlayer::InputTool1Ability2);
			EnhancedInputComponent->BindAction(ActionTool1Ability2, ETriggerEvent::Completed, this, &ABasePlayer::InputTool1Ability2Released);
		}
		if (ActionTool2Ability1)
		{
			EnhancedInputComponent->BindAction(ActionTool2Ability1, ETriggerEvent::Started, this, &ABasePlayer::InputTool2Ability1);
			EnhancedInputComponent->BindAction(ActionTool2Ability1, ETriggerEvent::Completed, this, &ABasePlayer::InputTool2Ability1Released);
		}
		if (ActionTool2Ability2)
		{
			EnhancedInputComponent->BindAction(ActionTool2Ability2, ETriggerEvent::Started, this, &ABasePlayer::InputTool2Ability2);
			EnhancedInputComponent->BindAction(ActionTool2Ability2, ETriggerEvent::Completed, this, &ABasePlayer::InputTool2Ability2Released);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("BasePlayer: Failed to cast PlayerInputComponent to UEnhancedInputComponent."));
	}
}

// --- Enhanced Input Callbacks ---

void ABasePlayer::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		if (MovementVector.Y != 0.0f)
		{
			const FRotator Rotation = Controller->GetControlRotation();
			const FRotator YawRotation(0, Rotation.Yaw, 0);
			const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
			AddMovementInput(Direction, MovementVector.Y);
		}

		if (MovementVector.X != 0.0f)
		{
			const FRotator Rotation = Controller->GetControlRotation();
			const FRotator YawRotation(0, Rotation.Yaw, 0);
			const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
			AddMovementInput(Direction, MovementVector.X);
		}
	}
}

void ABasePlayer::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void ABasePlayer::StartSprint(const FInputActionValue& Value)
{
	bSprinting = true;
	ApplyMoveSpeed();
}

void ABasePlayer::StopSprint(const FInputActionValue& Value)
{
	bSprinting = false;
	ApplyMoveSpeed();
}

void ABasePlayer::Fire(const FInputActionValue& Value)
{
	InputActionFire();
}

void ABasePlayer::Reload(const FInputActionValue& Value)
{
	InputActionReload();
}

void ABasePlayer::DamageTest(const FInputActionValue& Value)
{
	InputActionDamageTest();
}

void ABasePlayer::PauseGame(const FInputActionValue& Value)
{
	InputPauseGame();
}

void ABasePlayer::StartInteract(const FInputActionValue& Value)
{
	BeginInteract();
}

void ABasePlayer::StopInteract(const FInputActionValue& Value)
{
	EndInteract();
}

// --- Movement & Speed Logic ---

void ABasePlayer::ApplyMoveSpeed()
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement)
	{
		UE_LOG(LogTemp, Warning, TEXT("BasePlayer: ApplyMoveSpeed found no CharacterMovement."));
		return;
	}

	const float BaseSpeed = bSprinting ? SprintSpeed : WalkSpeed;
	const float PotionMultiplier = bSpeedBoosted ? SpeedBoostMultiplier : 1.0f;

	float MovementStat = 100.0f;

	if (AttributeSystem)
	{
		AttributeSystem->GetAttributeValue(UCombatAttributeSet::StaticClass(), TEXT("MovementSpeed"), MovementStat);
	}

	const float StatMultiplier = FMath::Max(0.0f, MovementStat) / 100.0f;

	Movement->MaxWalkSpeed = BaseSpeed * StatMultiplier * PotionMultiplier;
}

void ABasePlayer::HandleAttributeChanged(UAttributeSetBase* ChangedSet, FName AttributeName, float OldValue, float NewValue)
{
	if (ChangedSet && ChangedSet->IsA<UCombatAttributeSet>())
	{
		if (AttributeName == TEXT("MovementSpeed"))
		{
			ApplyMoveSpeed();
		}

		if (AttributeName == TEXT("MaxHealth") && HealthComponent)
		{
			HealthComponent->SetMaxHealth(NewValue);
		}
	}

	if (ChangedSet && ChangedSet->IsA<UPlayerAttributeSet>())
	{
		if (AttributeName == GET_MEMBER_NAME_CHECKED(UPlayerAttributeSet, CurrentMana))
		{
			// A drop means mana was spent, so regen waits again
			if (NewValue < OldValue)
			{
				LastManaUseTime = GetWorld()->GetTimeSeconds();
			}

			UpdateManaHUD();
		}
		else if (AttributeName == GET_MEMBER_NAME_CHECKED(UPlayerAttributeSet, MaxMana))
		{
			// Losing max mana (gear taken off) can leave current above the new max
			if (AttributeSystem && GetCurrentMana() > NewValue)
			{
				AttributeSystem->SetBaseAttributeValue(UPlayerAttributeSet::StaticClass(),
					GET_MEMBER_NAME_CHECKED(UPlayerAttributeSet, CurrentMana), FMath::Max(0.0f, NewValue));
			}

			UpdateManaHUD();
		}
	}
}

bool ABasePlayer::ApplySpeedBoost(float Multiplier, float Duration)
{
	if (Multiplier <= 0.0f || Duration <= 0.0f)
	{
		return false;
	}

	bSpeedBoosted = true;
	SpeedBoostMultiplier = Multiplier;
	ApplyMoveSpeed();

	GetWorldTimerManager().SetTimer(SpeedBoostTimer, this, &ABasePlayer::ClearSpeedBoost, Duration, false);
	UE_LOG(LogTemp, Log, TEXT("BasePlayer: Speed boost %f for %f seconds."), Multiplier, Duration);
	return true;
}

void ABasePlayer::ClearSpeedBoost()
{
	bSpeedBoosted = false;
	SpeedBoostMultiplier = 1.0f;
	ApplyMoveSpeed();
	UE_LOG(LogTemp, Log, TEXT("BasePlayer: Speed boost ended."));
}

float ABasePlayer::GetPlayerAttribute(FName AttributeName) const
{
	float Value = 0.0f;

	if (AttributeSystem)
	{
		AttributeSystem->GetAttributeValue(UPlayerAttributeSet::StaticClass(), AttributeName, Value);
	}

	return FMath::Max(0.0f, Value);
}

float ABasePlayer::GetMaxMana() const
{
	return GetPlayerAttribute(GET_MEMBER_NAME_CHECKED(UPlayerAttributeSet, MaxMana));
}

float ABasePlayer::GetCurrentMana() const
{
	return GetPlayerAttribute(GET_MEMBER_NAME_CHECKED(UPlayerAttributeSet, CurrentMana));
}

bool ABasePlayer::AddMana(float Amount)
{
	if (!AttributeSystem || Amount <= 0.0f || GetCurrentMana() >= GetMaxMana())
	{
		return false;
	}

	// The attribute set clamps to MaxMana, and HandleAttributeChanged updates the HUD
	return AttributeSystem->ExecuteInstantModifier(UPlayerAttributeSet::StaticClass(),
		GET_MEMBER_NAME_CHECKED(UPlayerAttributeSet, CurrentMana), EAttributeModifierOperation::Add, Amount);
}

void ABasePlayer::RegenMana(float DeltaTime)
{
	const float Delay = GetPlayerAttribute(GET_MEMBER_NAME_CHECKED(UPlayerAttributeSet, ManaRegenDelay));
	if (GetWorld()->GetTimeSeconds() - LastManaUseTime < Delay)
	{
		return;
	}

	// No regen once dead
	if (HealthComponent && HealthComponent->GetHealthRatio() <= 0.0f)
	{
		return;
	}

	const float Rate = GetPlayerAttribute(GET_MEMBER_NAME_CHECKED(UPlayerAttributeSet, ManaRegenRate));
	if (Rate > 0.0f)
	{
		AddMana(Rate * DeltaTime);
	}
}

void ABasePlayer::UpdateManaHUD()
{
	if (HUDWidget)
	{
		const float Max = GetMaxMana();
		HUDWidget->SetMana(Max > 0.0f ? GetCurrentMana() / Max : 0.0f);
	}
}

bool ABasePlayer::ApplyPotion(EPotionEffect Effect, float Amount, float Duration)
{
	if (Effect == EPotionEffect::Health)
	{
		UHealthComponent* Health = FindComponentByClass<UHealthComponent>();
		if (!Health || Health->GetHealthRatio() >= 1.0f || Amount <= 0.0f)
		{
			return false;
		}

		UGameplayStatics::ApplyDamage(this, -Amount, nullptr, this, UDamageType::StaticClass());
		return true;
	}

	if (Effect == EPotionEffect::Speed)
	{
		return ApplySpeedBoost(Amount, Duration);
	}

	if (Effect == EPotionEffect::Mana)
	{
		return AddMana(Amount);
	}

	return false;
}

void ABasePlayer::SetHUDVisible(bool bVisible)
{
	if (HUDWidget)
	{
		HUDWidget->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}

// --- Combat, Health, & Item Interactions ---

void ABasePlayer::HandleHurt(float Ratio)
{
	if (HUDWidget)
	{
		HUDWidget->SetHealth(Ratio);
	}
}

void ABasePlayer::HandleHeal(float Ratio)
{
	if (HUDWidget)
	{
		HUDWidget->SetHealth(Ratio);
	}
}

void ABasePlayer::HandleDeath(float Ratio)
{
	if (HUDWidget)
	{
		HUDWidget->SetHealth(0.0f);
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (PlayerController)
	{
		DisableInput(PlayerController);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("BasePlayer: No PlayerController on death."));
	}
}

void ABasePlayer::InputActionFire()
{
	if (InventoryWidget && InventoryWidget->IsInViewport())
	{
		return;
	}

	if (!Rifle)
	{
		UE_LOG(LogTemp, Warning, TEXT("BasePlayer: Fire pressed but Rifle was null."));
		return;
	}

	const int32 AmmoBefore = Rifle->GetCurrentAmmo();
	Rifle->Attack();

	if (Rifle->GetCurrentAmmo() < AmmoBefore)
	{
		if (UCharacterAnimation* Anim = Cast<UCharacterAnimation>(GetMesh()->GetAnimInstance()))
		{
			Anim->FireAnimation();
		}
	}
}

void ABasePlayer::InputActionReload()
{
	if (Rifle)
	{
		Rifle->RequestReload();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("BasePlayer: Reload pressed but Rifle was null."));
	}
}

void ABasePlayer::HandleReloadStarted()
{
	UCharacterAnimation* Anim = Cast<UCharacterAnimation>(GetMesh()->GetAnimInstance());
	if (Anim && Anim->ReloadAnimation())
	{
		if (Rifle)
		{
			Rifle->PlayReloadSound();
		}
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("BasePlayer: Reload animation did not play. Rifle lock cleared."));
	if (Rifle)
	{
		Rifle->EndReload();
	}
}

void ABasePlayer::HandleReloadNow()
{
	if (Rifle)
	{
		Rifle->ReloadAmmo();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("BasePlayer: ReloadNow fired but Rifle was null."));
	}
}

void ABasePlayer::HandleReloadEnded()
{
	if (Rifle)
	{
		Rifle->EndReload();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("BasePlayer: ReloadEnded fired but Rifle was null."));
	}
}

void ABasePlayer::InputActionDamageTest()
{
	UGameplayStatics::ApplyDamage(this, TestDamageAmount, GetController(), this, UDamageType::StaticClass());
}

void ABasePlayer::HandleAmmoChanged(int32 Current, int32 Max)
{
	if (HUDWidget)
	{
		HUDWidget->SetAmmo(Current, Max);
	}
}

void ABasePlayer::CheckForInteractables()
{
	InteractionInfo.LastInteractionCheckTime = GetWorld()->GetTimeSeconds();

	FVector TraceStart = Camera->GetComponentLocation();
	FVector TraceEnd = TraceStart + (Camera->GetForwardVector() * InteractionCheckDistance);

	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(this);

	FHitResult TraceHit;

	if (GetWorld()->LineTraceSingleByChannel(TraceHit, TraceStart, TraceEnd, ECC_Visibility, TraceParams))
	{
		AActor* HitActor = TraceHit.GetActor();
		if (HitActor && HitActor->GetClass()->ImplementsInterface(UInteractableObjectInterface::StaticClass()))
		{
			if (HitActor != InteractionInfo.CurrentInteractable)
			{
				FoundInteractable(HitActor);
				return;
			}

			if (HitActor == InteractionInfo.CurrentInteractable)
			{
				return;
			}
		}
	}

	NoInteractableFound();
}

void ABasePlayer::FoundInteractable(AActor* Interactable)
{
	if (IsInteracting())
	{
		EndInteract();
	}

	if (InteractionInfo.CurrentInteractable)
	{
		TargetInteractable = InteractionInfo.CurrentInteractable;
		TargetInteractable->EndFocus();
	}

	InteractionInfo.CurrentInteractable = Interactable;
	TargetInteractable = Interactable;
	TargetInteractable->BeginFocus();

	if (HUDWidget)
	{
		HUDWidget->SetInteractPrompt(FText::Format(
			NSLOCTEXT("HUD", "InteractPrompt", "[E] {0}"),
			TargetInteractable->InteractionData.InteractionName));
	}
}

void ABasePlayer::NoInteractableFound()
{
	if (HUDWidget)
	{
		HUDWidget->ClearInteractPrompt();
	}

	if (IsInteracting())
	{
		GetWorldTimerManager().ClearTimer(InteractionCheckTimerHandle);
	}

	if (InteractionInfo.CurrentInteractable)
	{
		if (IsValid(TargetInteractable.GetObject()))
		{
			TargetInteractable->EndFocus();
		}
		InteractionInfo.CurrentInteractable = nullptr;
		TargetInteractable = nullptr;
	}
}

void ABasePlayer::BeginInteract()
{
	CheckForInteractables();

	if (InteractionInfo.CurrentInteractable)
	{
		if (IsValid(TargetInteractable.GetObject()))
		{
			TargetInteractable->BeginInteract();

			if (FMath::IsNearlyZero(TargetInteractable->InteractionData.InteractionDuration, 0.1f))
			{
				Interact();
			}
			else
			{
				GetWorldTimerManager().SetTimer(InteractionCheckTimerHandle, this, &ABasePlayer::Interact, TargetInteractable->InteractionData.InteractionDuration, false);
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("BasePlayer: BeginInteract called but TargetInteractable was invalid."));
		}
	}
}

void ABasePlayer::EndInteract()
{
	GetWorldTimerManager().ClearTimer(InteractionCheckTimerHandle);

	if (IsValid(TargetInteractable.GetObject()))
	{
		TargetInteractable->EndInteract();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("BasePlayer: EndInteract called but TargetInteractable was invalid."));
	}
}

void ABasePlayer::Interact()
{
	GetWorldTimerManager().ClearTimer(InteractionCheckTimerHandle);
	if (IsValid(TargetInteractable.GetObject()))
	{
		TargetInteractable->Interact();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("BasePlayer: Interact called but TargetInteractable was invalid."));
	}
}

void ABasePlayer::InputPauseGame()
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (PlayerController == nullptr)
	{
		return;
	}

	if (PlayerController->IsPaused())
	{
		return;
	}

	if (PauseMenuClass && !PauseMenuWidget)
	{
		PauseMenuWidget = CreateWidget<UWBP_PauseMenu>(PlayerController, PauseMenuClass);
	}

	if (PauseMenuWidget)
	{
		PlayerController->SetPause(true);

		PauseMenuWidget->AddToViewport(100);

		PlayerController->bShowMouseCursor = true;

		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(PauseMenuWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PlayerController->SetInputMode(InputMode);
	}
}

void ABasePlayer::InputToggleInventory()
{
	APlayerController* PC = Cast<APlayerController>(GetController());

	if (!PC)
	{
		return;
	}

	if (!InventoryWidget)
	{
		if (!InventoryWidgetClass)
		{
			return;
		}

		InventoryWidget = CreateWidget<UUserWidget>(PC, InventoryWidgetClass);

		if (!InventoryWidget)
		{
			return;
		}
	}

	if (InventoryWidget->IsInViewport())
	{
		InventoryWidget->RemoveFromParent();

		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeGameOnly());
	}
	else
	{
		InventoryWidget->AddToViewport(50);

		PC->bShowMouseCursor = true;

		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(InventoryWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

		PC->SetInputMode(InputMode);
	}
}

void ABasePlayer::InputTool1Ability1()
{
	if (ToolController)
	{
		ToolController->UseToolOneAbility(1);
	}
}

void ABasePlayer::InputTool1Ability1Released()
{
	UE_LOG(LogTemp, Log, TEXT("BasePlayer: InputTool1Ability1Released called. Not currently implemented."));
}

void ABasePlayer::InputTool1Ability2()
{
	if (ToolController)
	{
		ToolController->UseToolOneAbility(2);
	}
}

void ABasePlayer::InputTool1Ability2Released()
{
	UE_LOG(LogTemp, Log, TEXT("BasePlayer: InputTool1Ability2Released called. Not currently implemented."));
}

void ABasePlayer::InputTool2Ability1()
{
	//if (ToolController)
	//{
	//	ToolController->UseToolTwoAbility(1);
	//}

	UE_LOG(LogTemp, Log, TEXT("BasePlayer: InputTool2Ability1 called. Not currently implemented."));
}

void ABasePlayer::InputTool2Ability1Released()
{
	UE_LOG(LogTemp, Log, TEXT("BasePlayer: InputTool2Ability1Released called. Not currently implemented."));
}

void ABasePlayer::InputTool2Ability2()
{
	//if (ToolController)
	//{
	//	ToolController->UseToolTwoAbility(2);
	//}

	UE_LOG(LogTemp, Log, TEXT("BasePlayer: InputTool2Ability2 called. Not currently implemented."));
}

void ABasePlayer::InputTool2Ability2Released()
{
	UE_LOG(LogTemp, Log, TEXT("BasePlayer: InputTool2Ability2Released called. Not currently implemented."));
}
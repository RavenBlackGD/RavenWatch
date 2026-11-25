// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractableObjectInterface.h"
#include "Shelter.generated.h"

class UHealthComponent;
class USceneComponent;
class UBoxComponent;
class UTextRenderComponent;
class UStaticMeshComponent;
class AShelter;

// One build stage: the level pieces that appear together, and the shelter's max health once they are up
USTRUCT(BlueprintType)
struct FShelterStage
{
    GENERATED_BODY()

    // Actor Tag on every level piece in this stage
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Shelter)
    FName PieceTag;

    // Shelter max health once this stage is built
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Shelter, meta = (ClampMin = "1.0"))
    float MaxHealth = 100.0f;
};

// Fires when a stage is built. StageIndex starts at 0.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FShelterStageBuilt, AShelter*, Shelter, int32, StageIndex);

// Fires once when health reaches zero
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FShelterDestroyed, AShelter*, Shelter);

UCLASS()
class RAVENWATCH_API AShelter : public AActor, public IInteractableObjectInterface
{
    GENERATED_BODY()

public:
    // Sets default values for this actor's properties
    AShelter();

    // The game mode advances waves from this
    UPROPERTY(BlueprintAssignable, Category = Shelter)
    FShelterStageBuilt OnStageBuilt;

    // The game mode shows the lose screen from this
    UPROPERTY(BlueprintAssignable, Category = Shelter)
    FShelterDestroyed OnShelterDestroyed;

    // Shows the next stage and raises max health. False when every stage is already built.
    UFUNCTION(BlueprintCallable, Category = Shelter)
    bool BuildNextStage();

    UFUNCTION(BlueprintPure, Category = Shelter)
    int32 GetBuiltStages() const { return BuiltStages; }

    UFUNCTION(BlueprintPure, Category = Shelter)
    int32 GetTotalStages() const { return Stages.Num(); }

    UFUNCTION(BlueprintPure, Category = Shelter)
    bool IsComplete() const { return BuiltStages >= Stages.Num(); }

    UFUNCTION(BlueprintPure, Category = Shelter)
    bool IsDestroyed() const { return bDestroyed; }

    // Current health divided by current max health
    UFUNCTION(BlueprintPure, Category = Shelter)
    float GetHealthRatio() const;

    // True for the shelter itself and for any of its built pieces. Enemies use this for line of sight.
    UFUNCTION(BlueprintPure, Category = Shelter)
    bool IsShelterPart(const AActor* Actor) const;

    // Centre of the built piece closest to From. Enemies aim here so their shots hit something that passes damage on.
    UFUNCTION(BlueprintPure, Category = Shelter)
    FVector GetAimPoint(const FVector& From) const;

    // Looking at an unstarted site shows the outline of the finished shelter
    virtual void BeginFocus() override;

    virtual void EndFocus() override;

    virtual void BeginInteract() override;

    virtual void EndInteract() override;

    // Hold finished: build the first stage, which starts the waves
    virtual void Interact() override;

    // Only runs while the start prompt is showing: turns the text to the camera and bobs the arrow
    virtual void Tick(float DeltaSeconds) override;


protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Shelter)
    USceneComponent* SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Health)
    UHealthComponent* HealthComponent;

    // Build stages in order. Each one names the Actor Tag on its level pieces.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Shelter)
    TArray<FShelterStage> Stages;

    // How many stages are standing when the mission starts
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Shelter, meta = (ClampMin = "0"))
    int32 StartingStages;

    // Health gained per second while below the current max, so health rises as the building goes up
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Shelter, meta = (ClampMin = "0.0"))
    float HealthGrowthPerSecond;

    // Seconds between health growth steps
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Shelter, meta = (ClampMin = "0.05"))
    float HealthGrowthInterval;

    // False ignores damage from the player's own shots
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Shelter)
    bool bPlayerCanDamage;

    // What the player's interact trace hits before building starts. Blocks only the Visibility channel.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Start)
    UBoxComponent* InteractVolume;

    // Seconds the player holds E to start building
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Start, meta = (ClampMin = "0.0"))
    float StartHoldTime;

    // Shown after [E] on the HUD
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Start)
    FText StartPrompt;

    // Floating message over the site until building starts. Size and colour are on the component.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Start)
    UTextRenderComponent* PromptText;

    // Bobbing marker under the message, pointing at the site. Pick its mesh on the Blueprint.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Start)
    UStaticMeshComponent* PromptArrow;

    // The floating message. Use a line break to split it.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Start, meta = (MultiLine = "true"))
    FText PromptMessage;

    // Height of the message above the shelter actor
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Start)
    float PromptHeight;

    // How far below the message the arrow sits
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Start)
    float PromptArrowGap;

    // How far the arrow bobs up and down
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Start, meta = (ClampMin = "0.0"))
    float PromptBobHeight;

    // Bobs per second
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Start, meta = (ClampMin = "0.0"))
    float PromptBobSpeed;


private:
    // Level pieces found for each stage, in the same order as Stages
    TArray<TArray<TWeakObjectPtr<AActor>>> StagePieces;

    // How many stages are up
    UPROPERTY(VisibleInstanceOnly, Category = Shelter)
    int32 BuiltStages;

    // Max health of the highest built stage
    float CurrentMaxHealth;

    // True once health reaches zero
    bool bDestroyed;

    // Raises health toward CurrentMaxHealth
    FTimerHandle GrowthTimerHandle;

    // Shows or hides one stage, with its collision
    void SetStageVisible(int32 StageIndex, bool bVisible);

    // Draws every unbuilt piece as an outline only, or puts them back to hidden
    void SetPreviewOutline(bool bShow);

    // True while the outline preview is showing
    bool bPreviewShown;

    // Shows or hides the floating prompt, and turns Tick on only while it shows
    void SetStartPromptVisible(bool bVisible);

    // Running time for the arrow bob
    float PromptTime;

    // One step of health growth. Stops itself at full health.
    void GrowHealth();

    // Bound to OnTakeAnyDamage on every piece, so hits on the walls hurt the shelter
    UFUNCTION()
    void HandlePieceDamaged(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

    // Bound to HealthComponent-OnDeath
    UFUNCTION()
    void HandleDeath(float Ratio);
};
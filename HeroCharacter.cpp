#include "Characters/HeroCharacter.h"
#include "Characters/HeroInputComponent.h"
#include "Characters/HeroCameraComponent.h"
#include "Characters/HeroLocomotionComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/InventoryComponent.h"
#include "DataAssets/WeaponDataAsset.h"
#include "AbilitySystemComponent.h"
#include "Abilities/AttributeSets/CharacterAttributeSet.h"
#include "WoWCloneGameplayTags.h"
#include "UI/InventoryWidget.h"
#include "UI/PlayerVitalsWidget.h"
#include "Blueprint/UserWidget.h"

AHeroCharacter::AHeroCharacter()
{
	// Enable tick for camera auto-alignment logic
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	// The character should not rotate with the camera
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Reset bit-packed booleans (AAA Standards)
	bIsRightClickPressed = 0;
	bIsLeftClickPressed = 0;
	bIsInCombat = 0;

	// Default initialization prevents warnings; proper values set in component
	GetCharacterMovement()->bOrientRotationToMovement = false;

	// Input Component Setup
	HeroInputComp = CreateDefaultSubobject<UHeroInputComponent>(TEXT("HeroInputComponent"));

	// Camera Component Setup
	HeroCameraComp = CreateDefaultSubobject<UHeroCameraComponent>(TEXT("HeroCameraComponent"));

	// Locomotion Component Setup
	HeroLocomotionComp = CreateDefaultSubobject<UHeroLocomotionComponent>(TEXT("HeroLocomotionComponent"));

	// Inventory Setup
	InventoryComp = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
}

void AHeroCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	InitializeComponents();
	InitializeCombatState();
	InitializePlayerInputState();
	InitializeHUD();
}

void AHeroCharacter::InitializePlayerInputState()
{
	if (APlayerController* PC = GetPlayerController())
	{
		PC->bShowMouseCursor = true;
		
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(InputMode);
	}
}

void AHeroCharacter::InitializeComponents()
{
	if (HeroLocomotionComp)
	{
		HeroLocomotionComp->InitializeLocomotion(this);
	}

	if (HeroCameraComp)
	{
		HeroCameraComp->InitializeCamera(this);
	}
}

void AHeroCharacter::InitializeHUD()
{
	if (PlayerVitalsWidgetClass)
	{
		if (APlayerController* PC = GetPlayerController())
		{
			PlayerVitalsWidgetInstance = CreateWidget<UPlayerVitalsWidget>(PC, PlayerVitalsWidgetClass);
			if (PlayerVitalsWidgetInstance)
			{
				PlayerVitalsWidgetInstance->AddToViewport();
				PlayerVitalsWidgetInstance->InitializeVitals(GetAbilitySystemComponent());
			}
		}
	}
}

void AHeroCharacter::InitializeCombatState()
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->AddLooseGameplayTag(WoWCloneTags::State_Uncombat);
	}
}

void AHeroCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (HeroLocomotionComp)
	{
		HeroLocomotionComp->UpdateLocomotionState(DeltaTime);
	}
}

float AHeroCharacter::GetBackwardMovementSpeedMultiplier() const
{
	return HeroLocomotionComp ? HeroLocomotionComp->GetBackwardSpeedMultiplier() : 1.0f;
}

bool AHeroCharacter::IsInCombat() const
{
	return bIsInCombat;
}

void AHeroCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	InitializeInputModule(PlayerInputComponent);
}

void AHeroCharacter::InitializeInputModule(UInputComponent* PlayerInputComponent)
{
	// AAA: Enhanced Input system routing
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (HeroInputComp)
		{
			HeroInputComp->InitializeInput(EnhancedInputComponent, this);
		}
	}
}

void AHeroCharacter::Move(const FInputActionValue& Value)
{
	if (!CanMove() || !HeroLocomotionComp) return;

	const FVector2D ProcessedInput = HeroLocomotionComp->GetNormalizedAndScaledMovementInput(Value.Get<FVector2D>());

	AddMovementInput(GetMovementDirection(EAxis::X), ProcessedInput.X);
	AddMovementInput(GetMovementDirection(EAxis::Y), ProcessedInput.Y);
}

bool AHeroCharacter::CanMove() const
{
	return Controller != nullptr;
}

FVector AHeroCharacter::GetMovementDirection(EAxis::Type Axis) const
{
	const FRotator YawRotation(0, GetActorRotation().Yaw, 0);
	return FRotationMatrix(YawRotation).GetUnitAxis(Axis);
}

void AHeroCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr && !LookAxisVector.IsZero())
	{
		HandleLookInput(LookAxisVector);
	}
}

void AHeroCharacter::HandleLookInput(const FVector2D& LookAxisVector)
{
	AddControllerYawInput(LookAxisVector.X);
	AddControllerPitchInput(LookAxisVector.Y);
}

void AHeroCharacter::RightClickStarted()
{
	bIsRightClickPressed = 1;

	// Allow the character to face exactly where the camera is looking
	SetCharacterYawDecoupled(true);
}

void AHeroCharacter::RightClickCompleted()
{
	bIsRightClickPressed = 0;

	// Decouple character rotation from the camera
	SetCharacterYawDecoupled(false);
}

void AHeroCharacter::SetCharacterYawDecoupled(bool bIsDecoupled)
{
	ConfigureRotationSettings(bIsDecoupled);
	
	if (HeroCameraComp)
	{
		HeroCameraComp->SetCameraDecoupled(bIsDecoupled);
	}
}

void AHeroCharacter::ConfigureRotationSettings(bool bIsDecoupled)
{
	bUseControllerRotationYaw = bIsDecoupled;

	// AAA Standard: Ensure movement rotation is strictly handled by the design pattern
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bOrientRotationToMovement = false; 
	}
}

void AHeroCharacter::LeftClickStarted()
{
	bIsLeftClickPressed = 1;
	SetMouseCursorVisibility(false);
}

void AHeroCharacter::LeftClickCompleted()
{
	bIsLeftClickPressed = 0;
	SetMouseCursorVisibility(true);
}

void AHeroCharacter::SetMouseCursorVisibility(bool bIsVisible)
{
	if (APlayerController* PC = GetPlayerController())
	{
		PC->bShowMouseCursor = bIsVisible;
	}
}

void AHeroCharacter::Zoom(const FInputActionValue& Value)
{
	if (!HeroCameraComp) return;

	const float ZoomValue = Value.Get<float>();
	if (ZoomValue == 0.0f) return;

	HeroCameraComp->ApplyCameraZoom(ZoomValue);
}

void AHeroCharacter::ToggleWeapon()
{
	if (InventoryComp)
	{
		InventoryComp->ToggleDrawHolster();
	}
}

void AHeroCharacter::ToggleWalk()
{
	if (CanToggleWalk())
	{
		HandleWalkSpeedToggle();
	}
}

bool AHeroCharacter::CanToggleWalk() const
{
	return !bIsInCombat;
}

void AHeroCharacter::HandleWalkSpeedToggle()
{
	if (HeroLocomotionComp)
	{
		HeroLocomotionComp->ToggleWalkSpeed();
	}
}

void AHeroCharacter::ToggleCombat()
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ToggleCombatState(ASC);
	}
}

void AHeroCharacter::ToggleCombatState(UAbilitySystemComponent* ASC)
{
	bIsInCombat = !bIsInCombat;
	bIsInCombat ? EnterCombatState(ASC) : ExitCombatState(ASC);
}

void AHeroCharacter::EnterCombatState(UAbilitySystemComponent* ASC)
{
	HandleCombatTagChange(ASC, true);
	ApplyCombatStateMovementOverrides();
	AutoEquipWeaponOnCombatEnter();
}

void AHeroCharacter::ExitCombatState(UAbilitySystemComponent* ASC)
{
	HandleCombatTagChange(ASC, false);
	RevertCombatStateMovementOverrides();
}

void AHeroCharacter::HandleCombatTagChange(UAbilitySystemComponent* ASC, bool bEnteringCombat)
{
	const FGameplayTag& TagToRemove = bEnteringCombat ? WoWCloneTags::State_Uncombat : WoWCloneTags::State_Combat;
	const FGameplayTag& TagToAdd = bEnteringCombat ? WoWCloneTags::State_Combat : WoWCloneTags::State_Uncombat;

	ToggleGameplayTagPair(ASC, TagToRemove, TagToAdd);
}

void AHeroCharacter::ApplyCombatStateMovementOverrides()
{
	if (HeroLocomotionComp)
	{
		HeroLocomotionComp->ApplyCombatStateOverrides();
	}
}

void AHeroCharacter::RevertCombatStateMovementOverrides()
{
	if (HeroLocomotionComp)
	{
		HeroLocomotionComp->RevertCombatStateOverrides();
	}
}

void AHeroCharacter::AutoEquipWeaponOnCombatEnter()
{
	if (InventoryComp && !InventoryComp->HasItemEquippedAtSlot(EEquipmentSlot::MainHand))
	{
		InventoryComp->EquipItemAtIndex(0, EEquipmentSlot::MainHand);
	}
}

void AHeroCharacter::ToggleInventory()
{
	EnsureInventoryWidgetCreated();
	ToggleInventoryVisibility();

	OnInventoryToggled.Broadcast();
}

void AHeroCharacter::EnsureInventoryWidgetCreated()
{
	if (InventoryWidgetInstance || !InventoryWidgetClass) return;

	if (APlayerController* PC = GetPlayerController())
	{
		InventoryWidgetInstance = CreateWidget<UInventoryWidget>(PC, InventoryWidgetClass);
		if (InventoryWidgetInstance)
		{
			InventoryWidgetInstance->InitializeInventoryUI(InventoryComp);
			InventoryWidgetInstance->AddToViewport();
			InventoryWidgetInstance->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void AHeroCharacter::ToggleInventoryVisibility()
{
	if (!InventoryWidgetInstance) return;

	const bool bNextVisibility = (InventoryWidgetInstance->GetVisibility() != ESlateVisibility::Visible);
	InventoryWidgetInstance->SetVisibility(bNextVisibility ? ESlateVisibility::Visible : ESlateVisibility::Hidden);

	SetInputModeForInventory(bNextVisibility);
}

void AHeroCharacter::SetInputModeForInventory(bool bInventoryOpen)
{
	APlayerController* PC = GetPlayerController();
	if (!PC) return;

	// Global mouse visibility rule for this project
	PC->bShowMouseCursor = true;

	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	
	if (bInventoryOpen)
	{
		InputMode.SetWidgetToFocus(InventoryWidgetInstance->TakeWidget());
	}
	
	PC->SetInputMode(InputMode);
}

APlayerController* AHeroCharacter::GetPlayerController() const
{
	return Cast<APlayerController>(GetController());
}

void AHeroCharacter::TestVitals()
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		// Directly deduct 20 HP and 20 Mana for testing UI smoothness
		ASC->ApplyModToAttributeUnsafe(UCharacterAttributeSet::GetHealthAttribute(), EGameplayModOp::Additive, -20.0f);
		ASC->ApplyModToAttributeUnsafe(UCharacterAttributeSet::GetManaAttribute(), EGameplayModOp::Additive, -20.0f);
	}
}

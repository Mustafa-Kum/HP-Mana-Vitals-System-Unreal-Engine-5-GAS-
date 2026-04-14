// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CharacterDataAsset.generated.h"

// AAA Constraint: Use a struct to encapsulate stat logic.
// This ensures high cohesion. If we add new stats later, we just add to the struct,
// rather than bloating the DataAsset with dozens of loose variables.
USTRUCT(BlueprintType)
struct FCharacterDerivedStatScaling
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scaling")
	float BaseHealth = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scaling")
	float AttackDamagePerStrength = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scaling")
	float ArmorPerAgility = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scaling")
	float CastSpeedPercentPerAgility = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scaling", meta = (ClampMin = "0.0"))
	float MaxCastSpeedPercent = 90.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scaling")
	float SpellDamagePerIntellect = 2.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scaling")
	float MaxManaPerIntellect = 15.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scaling")
	float MaxHealthPerStamina = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scaling")
	float BaseHealthRegen = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scaling")
	float HealthRegenPerStrength = 0.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scaling")
	float BaseManaRegen = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scaling")
	float ManaRegenPerIntellect = 0.25f;
};

USTRUCT(BlueprintType)
struct FCharacterStartingStats
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats|Primary")
	float InitialStrength = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats|Primary")
	float InitialAgility = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats|Primary")
	float InitialIntellect = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats|Primary")
	float InitialStamina = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats|Secondary")
	float BaseCriticalStrikeChance = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats|Secondary")
	float BaseMovementSpeed = 600.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats|Derived")
	float BaseArmor = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats|Derived")
	float BaseMagicResistance = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats|Derived")
	float BasePhysicalDamage = 15.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats|Derived")
	float BaseMagicDamage = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats|Vital")
	float BaseMaxMana = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats|Scaling")
	FCharacterDerivedStatScaling DerivedScaling;
};

/**
 * AAA Note: We inherit from UPrimaryDataAsset instead of UDataAsset
 * because PrimaryDataAssets can be loaded asynchronously via the Asset Manager.
 */
UCLASS(BlueprintType, Blueprintable)
class WOWCLONE_API UCharacterDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	// The core starting attributes for this character class
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Class Setup")
	FCharacterStartingStats StartingStats;

	// Startup abilities granted when the character's ASC is initialized.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Class Setup|Abilities")
	TArray<TSubclassOf<class UGameplayAbility>> GrantedAbilities;

	// In the future:
	// UPROPERTY(EditDefaultsOnly) TObjectPtr<UWeaponDataAsset> DefaultWeapon;
};

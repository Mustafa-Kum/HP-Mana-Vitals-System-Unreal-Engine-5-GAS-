// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CharacterDataAsset.generated.h"

// AAA Constraint: Use a struct to encapsulate stat logic.
// This ensures high cohesion. If we add new stats later, we just add to the struct,
// rather than bloating the DataAsset with dozens of loose variables.
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
	float BasePhysicalDamage = 15.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats|Derived")
	float BaseMagicDamage = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats|Vital")
	float BaseMaxMana = 100.0f;
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

	// In the future:
	// UPROPERTY(EditDefaultsOnly) TArray<TSubclassOf<UGameplayAbility>> GrantedAbilities;
	// UPROPERTY(EditDefaultsOnly) TObjectPtr<UWeaponDataAsset> DefaultWeapon;
};

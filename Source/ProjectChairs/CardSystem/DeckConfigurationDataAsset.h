// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CardDataAsset.h"
#include "DeckConfigurationDataAsset.generated.h"

/**
 * Represents a card and how many copies of it are in the deck.
 */
USTRUCT(BlueprintType)
struct FDeckCardEntry
{
	GENERATED_BODY()

	/** The card data asset */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Deck")
	UCardDataAsset* Card = nullptr;

	/** Number of copies of this card in the deck */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Deck", meta = (ClampMin = "1"))
	int32 Count = 1;
};

/**
 * Data asset representing a deck configuration with cards and their counts.
 */
UCLASS(BlueprintType)
class PROJECTCHAIRS_API UDeckConfigurationDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** List of cards and their quantities in this deck */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Deck")
	TArray<FDeckCardEntry> Cards;

	/** Get the total number of cards in this deck configuration */
	UFUNCTION(BlueprintCallable, Category = "Deck")
	int32 GetTotalCardCount() const;
};

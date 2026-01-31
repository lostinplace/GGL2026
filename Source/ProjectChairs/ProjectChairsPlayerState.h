// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "CardSystem/DeckConfigurationDataAsset.h"
#include "CardSystem/CardObject.h"
#include "ProjectChairsPlayerState.generated.h"

/**
 * Custom PlayerState that manages the player's deck and hand of cards.
 */
UCLASS()
class PROJECTCHAIRS_API AProjectChairsPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AProjectChairsPlayerState();

	virtual void BeginPlay() override;

	/** Initialize the deck from a configuration data asset */
	UFUNCTION(BlueprintCallable, Category = "Cards")
	void InitializeDeckFromConfiguration(UDeckConfigurationDataAsset* DeckConfiguration);

	/** Shuffle the deck */
	UFUNCTION(BlueprintCallable, Category = "Cards")
	void ShuffleDeck();

	/** Draw a card from the deck into the hand. Returns the drawn card, or nullptr if deck is empty. */
	UFUNCTION(BlueprintCallable, Category = "Cards")
	UCardObject* DrawCard();

	/** Draw multiple cards from the deck into the hand */
	UFUNCTION(BlueprintCallable, Category = "Cards")
	void DrawCards(int32 Count);

	/** Discard a card from the hand (removes it from play) */
	UFUNCTION(BlueprintCallable, Category = "Cards")
	bool PlayCardFromHand(UCardObject* Card);

	/** Get the current deck */
	UFUNCTION(BlueprintCallable, Category = "Cards")
	const TArray<UCardObject*>& GetDeck() const { return Deck; }

	/** Get the current hand */
	UFUNCTION(BlueprintCallable, Category = "Cards")
	const TArray<UCardObject*>& GetHand() const { return Hand; }

	/** Get the number of cards in the deck */
	UFUNCTION(BlueprintCallable, Category = "Cards")
	int32 GetDeckCount() const { return Deck.Num(); }

	/** Get the number of cards in the hand */
	UFUNCTION(BlueprintCallable, Category = "Cards")
	int32 GetHandCount() const { return Hand.Num(); }

	/** Get the maximum hand size */
	UFUNCTION(BlueprintCallable, Category = "Cards")
	int32 GetMaxHandSize() const { return MaxHandSize; }

	/** Check if the hand is at maximum capacity */
	UFUNCTION(BlueprintCallable, Category = "Cards")
	bool IsHandFull() const { return Hand.Num() >= MaxHandSize; }

	/** Get the discard pile */
	UFUNCTION(BlueprintCallable, Category = "Cards")
	const TArray<UCardObject*>& GetDiscardPile() const { return DiscardPile; }

	/** Shuffle the discard pile back into the deck */
	UFUNCTION(BlueprintCallable, Category = "Cards")
	void ReshuffleDiscardIntoDeck();

protected:
	/** Create a card object from a data asset */
	UCardObject* CreateCardFromDataAsset(UCardDataAsset* CardData);

	/** The default deck configuration to use if none is specified */
	UPROPERTY(EditDefaultsOnly, Category = "Cards")
	UDeckConfigurationDataAsset* DefaultDeckConfiguration;

	/** Maximum number of cards allowed in the hand */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cards")
	int32 MaxHandSize = 7;

	/** The player's current deck (cards not yet drawn) */
	UPROPERTY(BlueprintReadOnly, Category = "Cards")
	TArray<UCardObject*> Deck;

	/** The player's current hand */
	UPROPERTY(BlueprintReadOnly, Category = "Cards")
	TArray<UCardObject*> Hand;

	/** The discard pile */
	UPROPERTY(BlueprintReadOnly, Category = "Cards")
	TArray<UCardObject*> DiscardPile;
};

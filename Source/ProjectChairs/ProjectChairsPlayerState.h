// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "CardSystem/DeckConfigurationDataAsset.h"
#include "CardSystem/CardObject.h"
#include "Logic/ChessData.h"
#include "ProjectChairsPlayerState.generated.h"

class AChessPieceActor;

/**
 * Enum representing the current card interaction mode.
 */
UENUM(BlueprintType)
enum class ECardInteractionMode : uint8
{
	None            UMETA(DisplayName = "None"),           // Normal chess mode
	SelectingTarget UMETA(DisplayName = "Selecting Target") // Card selected, waiting for target
};

/** Delegate broadcast when the selected card or interaction mode changes */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSelectedCardChanged, UCardObject*, SelectedCard, ECardInteractionMode, Mode);

/** Delegate broadcast when the hand changes (cards added or removed) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHandChanged, const TArray<UCardObject*>&, NewHand);

/**
 * Custom PlayerState that manages the player's deck and hand of cards.
 * Also stores the assigned chess color for multiplayer chess games.
 */
UCLASS()
class PROJECTCHAIRS_API AProjectChairsPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AProjectChairsPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Chess Multiplayer
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Chess")
	EPieceColor AssignedChessColor = EPieceColor::White;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Chess")
	bool bHasAssignedColor = false;

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

	// Card Selection System

	/** Select a card from the hand and enter target selection mode */
	UFUNCTION(BlueprintCallable, Category = "Card Selection")
	void SelectCard(UCardObject* Card);

	/** Clear the current card selection and return to normal mode */
	UFUNCTION(BlueprintCallable, Category = "Card Selection")
	void ClearCardSelection();

	/** Get the currently selected card */
	UFUNCTION(BlueprintCallable, Category = "Card Selection")
	UCardObject* GetSelectedCard() const { return SelectedCard; }

	/** Get the current card interaction mode */
	UFUNCTION(BlueprintCallable, Category = "Card Selection")
	ECardInteractionMode GetCardInteractionMode() const { return CardInteractionMode; }

	/** Check if we are currently in card targeting mode */
	UFUNCTION(BlueprintCallable, Category = "Card Selection")
	bool IsSelectingCardTarget() const { return CardInteractionMode == ECardInteractionMode::SelectingTarget; }

	/** Check if the player has already played a card this turn */
	UFUNCTION(BlueprintCallable, Category = "Card Selection")
	bool HasPlayedCardThisTurn() const { return bHasPlayedCardThisTurn; }

	/** Reset the card played flag (call when turn changes) */
	UFUNCTION(BlueprintCallable, Category = "Card Selection")
	void ResetCardPlayedThisTurn() { bHasPlayedCardThisTurn = false; }

	/**
	 * Attempt to apply the selected card to a target chess piece.
	 * Validates the target based on card's target type and player's assigned color.
	 * @return True if the card was successfully applied, false otherwise
	 */
	UFUNCTION(BlueprintCallable, Category = "Card Selection")
	bool TryApplySelectedCardToTarget(AChessPieceActor* TargetPiece);

	/** Broadcast when the selected card or mode changes */
	UPROPERTY(BlueprintAssignable, Category = "Card Selection")
	FOnSelectedCardChanged OnSelectedCardChanged;

	/** Broadcast when the hand changes (cards added or removed) */
	UPROPERTY(BlueprintAssignable, Category = "Cards")
	FOnHandChanged OnHandChanged;

protected:
	/** Create a card object from a data asset */
	UCardObject* CreateCardFromDataAsset(UCardDataAsset* CardData);

	/** The default deck configuration to use if none is specified */
	UPROPERTY(EditDefaultsOnly, Category = "Cards")
	UDeckConfigurationDataAsset* DefaultDeckConfiguration;

	/** Maximum number of cards allowed in the hand */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cards")
	int32 MaxHandSize = 5;

	/** The player's current deck (cards not yet drawn) */
	UPROPERTY(BlueprintReadOnly, Category = "Cards")
	TArray<UCardObject*> Deck;

	/** The player's current hand */
	UPROPERTY(BlueprintReadOnly, Category = "Cards")
	TArray<UCardObject*> Hand;

	/** The discard pile */
	UPROPERTY(BlueprintReadOnly, Category = "Cards")
	TArray<UCardObject*> DiscardPile;

	// Card Selection State

	/** The currently selected card (if any) */
	UPROPERTY(BlueprintReadOnly, Category = "Card Selection")
	UCardObject* SelectedCard;

	/** The current card interaction mode */
	UPROPERTY(BlueprintReadOnly, Category = "Card Selection")
	ECardInteractionMode CardInteractionMode;

	/** Whether the player has played a card this turn */
	UPROPERTY(BlueprintReadOnly, Category = "Card Selection")
	bool bHasPlayedCardThisTurn;
};

// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectChairsPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "ProjectChairsChessPieceActor.h"
#include "CardSystem/Effects/ChessPieceEffectComponent.h"

AProjectChairsPlayerState::AProjectChairsPlayerState()
	: DefaultDeckConfiguration(nullptr)
	, SelectedCard(nullptr)
	, CardInteractionMode(ECardInteractionMode::None)
{
}

void AProjectChairsPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AProjectChairsPlayerState, AssignedChessColor);
	DOREPLIFETIME(AProjectChairsPlayerState, bHasAssignedColor);
}

void AProjectChairsPlayerState::BeginPlay()
{
	Super::BeginPlay();

	if (DefaultDeckConfiguration)
	{
		InitializeDeckFromConfiguration(DefaultDeckConfiguration);
	}
}

void AProjectChairsPlayerState::InitializeDeckFromConfiguration(UDeckConfigurationDataAsset* DeckConfiguration)
{
	if (!DeckConfiguration)
	{
		return;
	}

	// Clear existing cards
	Deck.Empty();
	Hand.Empty();
	DiscardPile.Empty();

	// Create card objects for each entry in the configuration
	for (const FDeckCardEntry& Entry : DeckConfiguration->Cards)
	{
		if (Entry.Card)
		{
			for (int32 i = 0; i < Entry.Count; ++i)
			{
				UCardObject* NewCard = CreateCardFromDataAsset(Entry.Card);
				if (NewCard)
				{
					Deck.Add(NewCard);
				}
			}
		}
	}
}

void AProjectChairsPlayerState::ShuffleDeck()
{
	if (Deck.Num() <= 1)
	{
		return;
	}

	// Fisher-Yates shuffle
	for (int32 i = Deck.Num() - 1; i > 0; --i)
	{
		int32 j = FMath::RandRange(0, i);
		Deck.Swap(i, j);
	}
}

UCardObject* AProjectChairsPlayerState::DrawCard()
{
	if (Deck.Num() == 0)
	{
		return nullptr;
	}

	// Draw from the top of the deck (end of array)
	UCardObject* DrawnCard = Deck.Pop();
	Hand.Add(DrawnCard);
	return DrawnCard;
}

void AProjectChairsPlayerState::DrawCards(int32 Count)
{
	for (int32 i = 0; i < Count; ++i)
	{
		if (!DrawCard())
		{
			// Deck is empty
			break;
		}
	}
}

bool AProjectChairsPlayerState::PlayCardFromHand(UCardObject* Card)
{
	if (!Card)
	{
		return false;
	}

	int32 Index = Hand.Find(Card);
	if (Index != INDEX_NONE)
	{
		Hand.RemoveAt(Index);
		DiscardPile.Add(Card);
		return true;
	}

	return false;
}

void AProjectChairsPlayerState::ReshuffleDiscardIntoDeck()
{
	Deck.Append(DiscardPile);
	DiscardPile.Empty();
	ShuffleDeck();
}

UCardObject* AProjectChairsPlayerState::CreateCardFromDataAsset(UCardDataAsset* CardData)
{
	if (!CardData)
	{
		return nullptr;
	}

	UCardObject* NewCard = NewObject<UCardObject>(this);
	NewCard->InitializeFromDataAsset(CardData);
	return NewCard;
}

void AProjectChairsPlayerState::SelectCard(UCardObject* Card)
{
	// Validate that the card is in our hand
	if (!Card || !Hand.Contains(Card))
	{
		return;
	}

	// If selecting the same card, deselect it
	if (SelectedCard == Card)
	{
		ClearCardSelection();
		return;
	}

	// Update selection state
	SelectedCard = Card;
	CardInteractionMode = ECardInteractionMode::SelectingTarget;

	// Broadcast the change
	OnSelectedCardChanged.Broadcast(SelectedCard, CardInteractionMode);
}

void AProjectChairsPlayerState::ClearCardSelection()
{
	SelectedCard = nullptr;
	CardInteractionMode = ECardInteractionMode::None;

	// Broadcast the change
	OnSelectedCardChanged.Broadcast(nullptr, ECardInteractionMode::None);
}

bool AProjectChairsPlayerState::TryApplySelectedCardToTarget(AProjectChairsChessPieceActor* TargetPiece)
{
	// Validate we have a selected card
	if (!SelectedCard || !Hand.Contains(SelectedCard))
	{
		ClearCardSelection();
		return false;
	}

	// Validate target piece
	if (!TargetPiece)
	{
		return false;
	}

	// Get card data to check target type and effect
	UCardDataAsset* CardData = SelectedCard->GetCardData();
	if (!CardData)
	{
		return false;
	}

	// Validate target based on ETargetType
	ETargetType TargetType = CardData->TargetType;
	EPieceColor PieceColor = TargetPiece->Color;

	switch (TargetType)
	{
	case ETargetType::Self:
		// Can only target own pieces
		if (PieceColor != AssignedChessColor)
		{
			return false;
		}
		break;

	case ETargetType::Enemy:
		// Can only target opponent's pieces
		if (PieceColor == AssignedChessColor)
		{
			return false;
		}
		break;

	case ETargetType::Any:
		// Any piece is valid
		break;
	}

	// Get the chess piece effect class
	TSubclassOf<UChessPieceEffect> EffectClass = CardData->ChessPieceEffectClass;
	if (!EffectClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("Card '%s' has no ChessPieceEffectClass set"), *CardData->DisplayName.ToString());
		return false;
	}

	// Apply the effect via the piece's effect component
	UChessPieceEffectComponent* EffectComponent = TargetPiece->EffectComponent;
	if (!EffectComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("Target piece has no EffectComponent"));
		return false;
	}

	// Apply the effect
	UChessPieceEffect* AppliedEffect = EffectComponent->ApplyEffect(EffectClass);
	if (!AppliedEffect)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to apply effect from card '%s'"), *CardData->DisplayName.ToString());
		return false;
	}

	// Consume the card (move to discard pile)
	PlayCardFromHand(SelectedCard);

	// Clear selection
	ClearCardSelection();

	return true;
}

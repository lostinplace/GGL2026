// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectChairsPlayerState.h"

AProjectChairsPlayerState::AProjectChairsPlayerState()
	: DefaultDeckConfiguration(nullptr)
{
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

bool AProjectChairsPlayerState::DiscardCardFromHand(UCardObject* Card)
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

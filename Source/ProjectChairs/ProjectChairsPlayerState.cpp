// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectChairsPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Presentation/ChessPieceActor.h"
#include "Presentation/ChessBoardActor.h"
#include "CardSystem/Effects/ChessPieceEffectComponent.h"
#include "Kismet/GameplayStatics.h"

AProjectChairsPlayerState::AProjectChairsPlayerState()
	: DefaultDeckConfiguration(nullptr)
	, SelectedCard(nullptr)
	, CardInteractionMode(ECardInteractionMode::None)
	, bHasPlayedCardThisTurn(false)
{
}

void AProjectChairsPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AProjectChairsPlayerState, AssignedChessColor);
	DOREPLIFETIME(AProjectChairsPlayerState, bHasAssignedColor);
	DOREPLIFETIME(AProjectChairsPlayerState, ReplicatedHandData);
}

void AProjectChairsPlayerState::OnRep_HandData()
{
	UE_LOG(LogTemp, Log, TEXT("[Cards] OnRep_HandData called - ReplicatedHandData has %d cards"), ReplicatedHandData.Num());

	// Rebuild local UCardObject instances from replicated data
	RebuildHandFromReplicatedData();

	UE_LOG(LogTemp, Log, TEXT("[Cards] Hand rebuilt with %d cards, broadcasting OnHandChanged"), Hand.Num());

	// Broadcast hand change to update UI
	OnHandChanged.Broadcast(Hand);
}

void AProjectChairsPlayerState::RebuildHandFromReplicatedData()
{
	// Clear existing hand
	Hand.Empty();

	// Recreate UCardObject instances from the replicated CardDataAsset references
	for (UCardDataAsset* CardData : ReplicatedHandData)
	{
		if (CardData)
		{
			UCardObject* NewCard = CreateCardFromDataAsset(CardData);
			if (NewCard)
			{
				Hand.Add(NewCard);
			}
		}
	}
}

void AProjectChairsPlayerState::Server_ConsumeCard_Implementation(int32 CardIndex)
{
	if (Hand.IsValidIndex(CardIndex) && ReplicatedHandData.IsValidIndex(CardIndex))
	{
		UCardObject* Card = Hand[CardIndex];
		Hand.RemoveAt(CardIndex);
		ReplicatedHandData.RemoveAt(CardIndex);
		DiscardPile.Add(Card);

		// Mark that we've played a card this turn
		bHasPlayedCardThisTurn = true;

		// Broadcast hand change for server-side UI (OnRep won't fire on server)
		OnHandChanged.Broadcast(Hand);

		UE_LOG(LogTemp, Log, TEXT("[CardSelection] Server consumed card at index %d"), CardIndex);
	}
}

void AProjectChairsPlayerState::BeginPlay()
{
	Super::BeginPlay();

	// Only initialize deck on the server - Hand will replicate to clients
	if (HasAuthority() && DefaultDeckConfiguration)
	{
		InitializeDeckFromConfiguration(DefaultDeckConfiguration);
		ShuffleDeck();

		// Draw initial hand
		DrawCards(InitialDrawCount);
		UE_LOG(LogTemp, Log, TEXT("[Cards] Drew initial hand of %d cards"), InitialDrawCount);
	}

	// Try to bind to turn change event (may need to retry if board isn't ready)
	TryBindToTurnChange();
}

void AProjectChairsPlayerState::TryBindToTurnChange()
{
	if (bBoundToTurnChange)
	{
		return;
	}

	// Find the board actor
	TArray<AActor*> BoardActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AChessBoardActor::StaticClass(), BoardActors);

	if (BoardActors.Num() > 0)
	{
		AChessBoardActor* Board = Cast<AChessBoardActor>(BoardActors[0]);
		if (Board && Board->GameModel)
		{
			Board->GameModel->OnTurnChanged.AddDynamic(this, &AProjectChairsPlayerState::OnChessTurnChanged);
			bBoundToTurnChange = true;
			UE_LOG(LogTemp, Log, TEXT("[Cards] Bound to OnTurnChanged event"));
			return;
		}
	}

	// Board not ready yet, try again after a short delay
	GetWorld()->GetTimerManager().SetTimer(
		BindTurnChangeTimerHandle,
		this,
		&AProjectChairsPlayerState::TryBindToTurnChange,
		0.5f,
		false
	);
}

void AProjectChairsPlayerState::OnChessTurnChanged(EPieceColor NewSideToMove)
{
	UE_LOG(LogTemp, Log, TEXT("[Cards] Turn changed to %d, my color is %d"), (int32)NewSideToMove, (int32)AssignedChessColor);

	// Reset the card played flag for this player
	if (NewSideToMove == AssignedChessColor)
	{
		bHasPlayedCardThisTurn = false;

		// Draw a card at the start of our turn (server only, will replicate)
		if (HasAuthority())
		{
			DrawCards(TurnDrawCount);
			UE_LOG(LogTemp, Log, TEXT("[Cards] Drew %d card(s) at start of turn"), TurnDrawCount);
		}
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
	// If we're not the server, request the draw through RPC
	if (!HasAuthority())
	{
		Server_DrawCard();
		return nullptr; // Card will be added via replication
	}

	// Check hand limit first
	if (Hand.Num() >= MaxHandSize)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Cards] Cannot draw: Hand is full (%d/%d)"), Hand.Num(), MaxHandSize);
		return nullptr;
	}

	if (Deck.Num() == 0)
	{
		return nullptr;
	}

	// Draw from the top of the deck (end of array)
	UCardObject* DrawnCard = Deck.Pop();
	Hand.Add(DrawnCard);

	// Update replicated data (server-side only)
	if (DrawnCard && DrawnCard->GetCardData())
	{
		ReplicatedHandData.Add(DrawnCard->GetCardData());
	}

	// Broadcast hand change (for server-side listeners)
	OnHandChanged.Broadcast(Hand);

	return DrawnCard;
}

void AProjectChairsPlayerState::Server_DrawCard_Implementation()
{
	// Server-side draw - this will replicate the Hand to the owning client
	DrawCard();
}

void AProjectChairsPlayerState::DrawCards(int32 Count)
{
	for (int32 i = 0; i < Count; ++i)
	{
		if (Hand.Num() >= MaxHandSize)
		{
			// Hand is full
			UE_LOG(LogTemp, Log, TEXT("[Cards] Stopped drawing: Hand is full (%d/%d)"), Hand.Num(), MaxHandSize);
			break;
		}

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
		if (ReplicatedHandData.IsValidIndex(Index))
		{
			ReplicatedHandData.RemoveAt(Index);
		}
		DiscardPile.Add(Card);

		// Broadcast hand change
		OnHandChanged.Broadcast(Hand);

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
	// Check if already played a card this turn
	if (bHasPlayedCardThisTurn)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CardSelection] Already played a card this turn"));
		return;
	}

	// Check if it's the player's turn
	TArray<AActor*> BoardActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AChessBoardActor::StaticClass(), BoardActors);
	if (BoardActors.Num() > 0)
	{
		AChessBoardActor* Board = Cast<AChessBoardActor>(BoardActors[0]);
		if (Board && Board->GameModel && Board->GameModel->BoardState)
		{
			EPieceColor CurrentTurn = Board->GameModel->BoardState->SideToMove;
			if (CurrentTurn != AssignedChessColor)
			{
				UE_LOG(LogTemp, Warning, TEXT("[CardSelection] Cannot play card: Not your turn (Current: %d, Assigned: %d)"),
					(int32)CurrentTurn, (int32)AssignedChessColor);
				return;
			}
		}
	}

	// Validate that the card is valid
	if (!Card || !Card->GetCardData())
	{
		UE_LOG(LogTemp, Warning, TEXT("[CardSelection] SelectCard failed: Card is null or has no data"));
		return;
	}

	// Check if card data is in hand (by CardDataAsset, not pointer)
	// This handles the case where Hand was rebuilt with new UCardObject instances
	bool bFoundInHand = false;
	for (UCardObject* HandCard : Hand)
	{
		if (HandCard && HandCard->GetCardData() == Card->GetCardData())
		{
			bFoundInHand = true;
			break;
		}
	}

	if (!bFoundInHand)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CardSelection] SelectCard failed: Card not in hand"));
		return;
	}

	// If selecting the same card (by data), deselect it
	if (SelectedCard && SelectedCard->GetCardData() == Card->GetCardData())
	{
		UE_LOG(LogTemp, Log, TEXT("[CardSelection] Same card selected, deselecting"));
		ClearCardSelection();
		return;
	}

	// Update selection state
	SelectedCard = Card;
	CardInteractionMode = ECardInteractionMode::SelectingTarget;

	UE_LOG(LogTemp, Log, TEXT("[CardSelection] Card selected: %s, Mode: SelectingTarget"), *Card->GetDisplayName().ToString());

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

bool AProjectChairsPlayerState::TryApplySelectedCardToTarget(AChessPieceActor* TargetPiece)
{
	UE_LOG(LogTemp, Log, TEXT("[CardSelection] TryApplySelectedCardToTarget called"));

	// Validate we have a selected card
	if (!SelectedCard)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CardSelection] No selected card"));
		ClearCardSelection();
		return false;
	}

	// Check if it's the player's turn (safety check)
	TArray<AActor*> BoardActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AChessBoardActor::StaticClass(), BoardActors);
	if (BoardActors.Num() > 0)
	{
		AChessBoardActor* Board = Cast<AChessBoardActor>(BoardActors[0]);
		if (Board && Board->GameModel && Board->GameModel->BoardState)
		{
			EPieceColor CurrentTurn = Board->GameModel->BoardState->SideToMove;
			if (CurrentTurn != AssignedChessColor)
			{
				UE_LOG(LogTemp, Warning, TEXT("[CardSelection] Cannot apply card: Not your turn"));
				ClearCardSelection();
				return false;
			}
		}
	}

	// Check if card data is in hand (by CardDataAsset, not pointer)
	// This handles the case where Hand was rebuilt with new UCardObject instances
	UCardDataAsset* SelectedCardData = SelectedCard->GetCardData();
	bool bFoundInHand = false;
	for (UCardObject* Card : Hand)
	{
		if (Card && Card->GetCardData() == SelectedCardData)
		{
			bFoundInHand = true;
			break;
		}
	}

	if (!bFoundInHand)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CardSelection] Selected card not found in hand"));
		ClearCardSelection();
		return false;
	}

	// Validate target piece
	if (!TargetPiece)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CardSelection] Target piece is null"));
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("[CardSelection] Target piece: %s"), *TargetPiece->GetName());

	// Get card data to check target type and effect
	UCardDataAsset* CardData = SelectedCard->GetCardData();
	if (!CardData)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CardSelection] Card has no CardData"));
		return false;
	}

	// Validate target based on ETargetType
	ETargetType TargetType = CardData->TargetType;
	EPieceColor PieceColor = TargetPiece->Color;

	UE_LOG(LogTemp, Log, TEXT("[CardSelection] TargetType: %d, PieceColor: %d, PlayerColor: %d"),
		(int32)TargetType, (int32)PieceColor, (int32)AssignedChessColor);

	switch (TargetType)
	{
	case ETargetType::Self:
		// Can only target own pieces
		if (PieceColor != AssignedChessColor)
		{
			UE_LOG(LogTemp, Warning, TEXT("[CardSelection] Invalid target: Self card but piece is not player's color"));
			return false;
		}
		break;

	case ETargetType::Enemy:
		// Can only target opponent's pieces
		if (PieceColor == AssignedChessColor)
		{
			UE_LOG(LogTemp, Warning, TEXT("[CardSelection] Invalid target: Enemy card but piece is player's color"));
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
		UE_LOG(LogTemp, Warning, TEXT("[CardSelection] Card '%s' has no ChessPieceEffectClass set"), *CardData->DisplayName.ToString());
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("[CardSelection] EffectClass: %s"), *EffectClass->GetName());

	// Find the effect component on the piece
	UChessPieceEffectComponent* EffectComponent = TargetPiece->FindComponentByClass<UChessPieceEffectComponent>();
	if (!EffectComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CardSelection] Target piece has no EffectComponent"));
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("[CardSelection] Calling ApplyEffect..."));

	// Apply the effect
	UChessPieceEffect* AppliedEffect = EffectComponent->ApplyEffect(EffectClass);
	if (!AppliedEffect)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CardSelection] Failed to apply effect from card '%s'"), *CardData->DisplayName.ToString());
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("[CardSelection] Effect applied successfully!"));

	// Find the card index by matching CardDataAsset (not pointer comparison)
	// This handles the case where Hand was rebuilt with new UCardObject instances
	int32 CardIndex = INDEX_NONE;
	UCardDataAsset* SelectedCardDataTmp = SelectedCard->GetCardData();
	for (int32 i = 0; i < Hand.Num(); ++i)
	{
		if (Hand[i] && Hand[i]->GetCardData() == SelectedCardDataTmp)
		{
			CardIndex = i;
			break;
		}
	}

	// Consume the card via Server RPC (handles replication)
	if (CardIndex != INDEX_NONE)
	{
		UE_LOG(LogTemp, Log, TEXT("[CardSelection] Consuming card at index %d, HasAuthority=%s"),
			CardIndex, HasAuthority() ? TEXT("true") : TEXT("false"));
		Server_ConsumeCard(CardIndex);

		// Optimistic local update for immediate UI feedback on client
		// The server will also update and replicate, but this gives instant feedback
		if (!HasAuthority())
		{
			UE_LOG(LogTemp, Log, TEXT("[CardSelection] Client: Applying optimistic update"));
			if (Hand.IsValidIndex(CardIndex))
			{
				Hand.RemoveAt(CardIndex);
				bHasPlayedCardThisTurn = true;
				UE_LOG(LogTemp, Log, TEXT("[CardSelection] Client: Hand now has %d cards, broadcasting OnHandChanged"), Hand.Num());
				OnHandChanged.Broadcast(Hand);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[CardSelection] Client: CardIndex %d not valid for Hand size %d"), CardIndex, Hand.Num());
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[CardSelection] Could not find card index for consumption!"));
	}

	// Clear selection
	ClearCardSelection();

	return true;
}

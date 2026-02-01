// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectChairsChessPlayerController.h"
#include "ProjectChairsPlayerState.h"
#include "CardSystem/CardObject.h"
#include "CardSystem/CardDataAsset.h"
#include "CardSystem/CardTargetHighlightComponent.h"
#include "Presentation/ChessBoardActor.h"
#include "Presentation/ChessPieceActor.h"
#include "Logic/ChessBoardState.h"
#include "Presentation/SelectableChessPieceComponent.h"
#include "EnhancedInputComponent.h"

AProjectChairsChessPlayerController::AProjectChairsChessPlayerController()
	: LastKnownSideToMove(EPieceColor::White)
	, bHasInitializedTurnTracking(false)
{
}

void AProjectChairsChessPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Bind to PlayerState's card selection delegate
	// Use a timer to ensure PlayerState is valid (may not be immediately available)
	FTimerHandle BindTimerHandle;
	GetWorld()->GetTimerManager().SetTimer(BindTimerHandle, [this]()
	{
		if (AProjectChairsPlayerState* PS = GetProjectChairsPlayerState())
		{
			PS->OnSelectedCardChanged.AddDynamic(this, &AProjectChairsChessPlayerController::OnSelectedCardChanged);
		}
	}, 0.1f, false);
}

void AProjectChairsChessPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Rebind the click action to our handler instead
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (ClickAction)
		{
			// Clear the parent's binding and add our own
			EnhancedInputComponent->ClearBindingsForObject(this);
			EnhancedInputComponent->BindAction(ClickAction, ETriggerEvent::Started, this, &AProjectChairsChessPlayerController::OnProjectChairsMouseClick);
		}
	}
}

void AProjectChairsChessPlayerController::OnProjectChairsMouseClick()
{
	UE_LOG(LogTemp, Log, TEXT("[CardController] OnProjectChairsMouseClick called"));

	// Check for turn changes and reset card played status if needed
	CheckAndResetCardPlayedOnTurnChange();

	// First, check if we're in card targeting mode
	AProjectChairsPlayerState* PS = GetProjectChairsPlayerState();

	if (!PS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CardController] PlayerState is null!"));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[CardController] IsSelectingCardTarget: %s"), PS->IsSelectingCardTarget() ? TEXT("true") : TEXT("false"));
	}

	if (PS && PS->IsSelectingCardTarget())
	{
		UE_LOG(LogTemp, Log, TEXT("[CardController] In card targeting mode"));

		// Get the hit location
		FHitResult Hit;
		GetHitResultUnderCursor(ECC_Visibility, true, Hit);

		if (Hit.bBlockingHit)
		{
			UE_LOG(LogTemp, Log, TEXT("[CardController] Hit: %s"), *GetNameSafe(Hit.GetActor()));

			// Try to handle as card targeting
			if (TryHandleCardTargeting(Hit.Location))
			{
				UE_LOG(LogTemp, Log, TEXT("[CardController] Card targeting succeeded"));
				// Card targeting was successful, don't process as chess move
				return;
			}

			// Check if we clicked on empty space (not a piece)
			AChessPieceActor* HitPiece = FindPieceAtLocation(Hit.Location);
			if (!HitPiece)
			{
				UE_LOG(LogTemp, Log, TEXT("[CardController] Clicked empty space, clearing selection"));
				// Clicked on empty space, clear card selection
				PS->ClearCardSelection();
				return;
			}
			// If we hit a piece but targeting failed (invalid target), keep selection active
			// and let the user try again
			UE_LOG(LogTemp, Log, TEXT("[CardController] Hit piece but targeting failed (invalid target)"));
			return;
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("[CardController] No hit, clearing selection"));
			// Clicked outside the board, clear selection
			PS->ClearCardSelection();
			return;
		}
	}

	// Not in card targeting mode, use standard chess logic
	// Recreate the parent's OnMouseClick behavior

	// Turn validation for multiplayer
	if (!CanInteract())
	{
		UE_LOG(LogTemp, Warning, TEXT("[ChessController] Not your turn! Waiting for opponent."));
		return;
	}

	FVector HitLoc;
	AChessBoardActor* HitBoard = FindBoardUnderCursor(HitLoc);

	if (HitBoard)
	{
		// Convert World Hit to Coord
		FBoardCoord Coord = HitBoard->WorldToCoord(HitLoc);

		UE_LOG(LogTemp, Warning, TEXT("[ChessController] Board Found: %s. HitLoc: %s. Coord: %d, %d"), *HitBoard->GetName(), *HitLoc.ToString(), Coord.File, Coord.Rank);

		// If valid, interact
		if (Coord.IsValid())
		{
			// Check logic
			if (Coord.File >= 0 && Coord.File <= 7 && Coord.Rank >= 0 && Coord.Rank <= 7)
			{
				HitBoard->HandleSquareClicked(Coord);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[ChessController] Coord out of bounds: %d, %d"), Coord.File, Coord.Rank);
				HitBoard->HandleSquareClicked(FBoardCoord(-1, -1));
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[ChessController] Invalid Coord calculated from HitLoc."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[ChessController] No Board Found under cursor."));

		// Debug what we DID hit
		FHitResult Hit;
		if (GetHitResultUnderCursor(ECC_Visibility, false, Hit))
		{
			UE_LOG(LogTemp, Warning, TEXT("[ChessController] Hit Actor: %s"), *GetNameSafe(Hit.GetActor()));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[ChessController] No Hit Result at all."));
		}

		if (CurrentBoard)
		{
			CurrentBoard->HandleSquareClicked(FBoardCoord(-1, -1));
		}
	}
}

bool AProjectChairsChessPlayerController::TryHandleCardTargeting(const FVector& HitLocation)
{
	UE_LOG(LogTemp, Log, TEXT("[CardController] TryHandleCardTargeting called"));

	AProjectChairsPlayerState* PS = GetProjectChairsPlayerState();
	if (!PS || !PS->IsSelectingCardTarget())
	{
		UE_LOG(LogTemp, Warning, TEXT("[CardController] Not in card targeting mode"));
		return false;
	}

	// Find the piece at the hit location
	AChessPieceActor* TargetPiece = FindPieceAtLocation(HitLocation);
	if (!TargetPiece)
	{
		UE_LOG(LogTemp, Log, TEXT("[CardController] No ChessPieceActor found at location"));
		// No piece at this location
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("[CardController] Found target piece: %s"), *TargetPiece->GetName());

	// Attempt to apply the card to this piece
	bool bResult = PS->TryApplySelectedCardToTarget(TargetPiece);
	UE_LOG(LogTemp, Log, TEXT("[CardController] TryApplySelectedCardToTarget returned: %s"), bResult ? TEXT("true") : TEXT("false"));
	return bResult;
}

AChessPieceActor* AProjectChairsChessPlayerController::FindPieceAtLocation(const FVector& HitLocation)
{
	// Use coordinate-based lookup instead of relying on hitting the piece actor directly
	// This matches how normal piece selection works
	if (!CurrentBoard || !CurrentBoard->GameModel || !CurrentBoard->GameModel->BoardState)
	{
		UE_LOG(LogTemp, Log, TEXT("[CardController] FindPieceAtLocation: No valid board"));
		return nullptr;
	}

	// Convert world location to board coordinate
	FBoardCoord Coord = CurrentBoard->WorldToCoord(HitLocation);

	if (!Coord.IsValid())
	{
		UE_LOG(LogTemp, Log, TEXT("[CardController] FindPieceAtLocation: Invalid coordinate"));
		return nullptr;
	}

	UE_LOG(LogTemp, Log, TEXT("[CardController] FindPieceAtLocation: Coord %d, %d"), Coord.File, Coord.Rank);

	// Get the piece ID at this coordinate
	int32 PieceId = CurrentBoard->GameModel->BoardState->GetPieceIdAt(Coord);

	if (PieceId == -1)
	{
		UE_LOG(LogTemp, Log, TEXT("[CardController] FindPieceAtLocation: No piece at coordinate"));
		return nullptr;
	}

	// Look up the piece actor
	if (AChessPieceActor** PieceActorPtr = CurrentBoard->PieceActors.Find(PieceId))
	{
		UE_LOG(LogTemp, Log, TEXT("[CardController] FindPieceAtLocation: Found piece %s"), *(*PieceActorPtr)->GetName());
		return *PieceActorPtr;
	}

	UE_LOG(LogTemp, Log, TEXT("[CardController] FindPieceAtLocation: Piece ID %d not found in PieceActors"), PieceId);
	return nullptr;
}

AProjectChairsPlayerState* AProjectChairsChessPlayerController::GetProjectChairsPlayerState() const
{
	return Cast<AProjectChairsPlayerState>(PlayerState);
}

void AProjectChairsChessPlayerController::OnSelectedCardChanged(UCardObject* SelectedCard, ECardInteractionMode Mode)
{
	if (Mode == ECardInteractionMode::SelectingTarget && SelectedCard)
	{
		UpdateCardTargetHighlights(SelectedCard);
	}
	else
	{
		ClearCardTargetHighlights();
	}
}

void AProjectChairsChessPlayerController::UpdateCardTargetHighlights(UCardObject* SelectedCard)
{
	if (!CurrentBoard || !SelectedCard)
	{
		return;
	}

	AProjectChairsPlayerState* PS = GetProjectChairsPlayerState();
	if (!PS)
	{
		return;
	}

	UCardDataAsset* CardData = SelectedCard->GetCardData();
	if (!CardData)
	{
		return;
	}

	ETargetType TargetType = CardData->TargetType;
	EPieceColor PlayerColor = PS->AssignedChessColor;

	// Iterate all pieces on the board and highlight valid targets
	for (auto& Pair : CurrentBoard->PieceActors)
	{
		AChessPieceActor* PieceActor = Pair.Value;
		if (!PieceActor)
		{
			continue;
		}

		// Find the highlight component on this piece
		UCardTargetHighlightComponent* HighlightComp = UCardTargetHighlightComponent::FindHighlightComponent(PieceActor);
		if (!HighlightComp)
		{
			continue;
		}

		bool bIsValidTarget = false;

		switch (TargetType)
		{
		case ETargetType::Self:
			bIsValidTarget = (PieceActor->Color == PlayerColor);
			break;

		case ETargetType::Enemy:
			bIsValidTarget = (PieceActor->Color != PlayerColor);
			break;

		case ETargetType::Any:
			bIsValidTarget = true;
			break;
		}

		HighlightComp->SetHighlighted(bIsValidTarget);
	}
}

void AProjectChairsChessPlayerController::ClearCardTargetHighlights()
{
	if (!CurrentBoard)
	{
		return;
	}

	// Clear highlights on all pieces
	for (auto& Pair : CurrentBoard->PieceActors)
	{
		if (AChessPieceActor* PieceActor = Pair.Value)
		{
			if (UCardTargetHighlightComponent* HighlightComp = UCardTargetHighlightComponent::FindHighlightComponent(PieceActor))
			{
				HighlightComp->SetHighlighted(false);
			}
		}
	}
}

void AProjectChairsChessPlayerController::CheckAndResetCardPlayedOnTurnChange()
{
	if (!CurrentBoard || !CurrentBoard->GameModel || !CurrentBoard->GameModel->BoardState)
	{
		return;
	}

	EPieceColor CurrentSideToMove = CurrentBoard->GameModel->BoardState->SideToMove;

	// Initialize on first check
	if (!bHasInitializedTurnTracking)
	{
		LastKnownSideToMove = CurrentSideToMove;
		bHasInitializedTurnTracking = true;
		return;
	}

	// Check if the turn has changed
	if (CurrentSideToMove != LastKnownSideToMove)
	{
		LastKnownSideToMove = CurrentSideToMove;

		// Reset card played status for the player whose turn it now is
		if (AProjectChairsPlayerState* PS = GetProjectChairsPlayerState())
		{
			// Only reset if it's now this player's turn
			if (PS->AssignedChessColor == CurrentSideToMove)
			{
				PS->ResetCardPlayedThisTurn();
				UE_LOG(LogTemp, Log, TEXT("[CardController] Turn changed to %s, reset card played status"),
					CurrentSideToMove == EPieceColor::White ? TEXT("White") : TEXT("Black"));
			}
		}
	}
}

// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectChairsChessPlayerController.h"
#include "ProjectChairsPlayerState.h"
#include "ProjectChairsChessPieceActor.h"
#include "Presentation/ChessBoardActor.h"
#include "Presentation/SelectableChessPieceComponent.h"
#include "EnhancedInputComponent.h"

AProjectChairsChessPlayerController::AProjectChairsChessPlayerController()
{
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
	// First, check if we're in card targeting mode
	AProjectChairsPlayerState* PS = GetProjectChairsPlayerState();
	if (PS && PS->IsSelectingCardTarget())
	{
		// Get the hit location
		FHitResult Hit;
		GetHitResultUnderCursor(ECC_Visibility, false, Hit);

		if (Hit.bBlockingHit)
		{
			// Try to handle as card targeting
			if (TryHandleCardTargeting(Hit.Location))
			{
				// Card targeting was successful, don't process as chess move
				return;
			}

			// Check if we clicked on empty space (not a piece)
			AProjectChairsChessPieceActor* HitPiece = FindPieceAtLocation(Hit.Location);
			if (!HitPiece)
			{
				// Clicked on empty space, clear card selection
				PS->ClearCardSelection();
				return;
			}
			// If we hit a piece but targeting failed (invalid target), keep selection active
			// and let the user try again
			return;
		}
		else
		{
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
	AProjectChairsPlayerState* PS = GetProjectChairsPlayerState();
	if (!PS || !PS->IsSelectingCardTarget())
	{
		return false;
	}

	// Find the piece at the hit location
	AProjectChairsChessPieceActor* TargetPiece = FindPieceAtLocation(HitLocation);
	if (!TargetPiece)
	{
		// No piece at this location
		return false;
	}

	// Attempt to apply the card to this piece
	return PS->TryApplySelectedCardToTarget(TargetPiece);
}

AProjectChairsChessPieceActor* AProjectChairsChessPlayerController::FindPieceAtLocation(const FVector& HitLocation)
{
	// Use a raycast to find the piece directly
	FHitResult Hit;
	GetHitResultUnderCursor(ECC_Visibility, false, Hit);

	if (!Hit.bBlockingHit || !Hit.GetActor())
	{
		return nullptr;
	}

	// Check if we hit a selectable chess piece
	if (USelectableChessPieceComponent* Selectable = USelectableChessPieceComponent::FindSelectableComponent(Hit.GetActor()))
	{
		// The actor has a selectable component, try to cast to our piece type
		return Cast<AProjectChairsChessPieceActor>(Hit.GetActor());
	}

	// Also try direct cast in case the selectable component check fails
	return Cast<AProjectChairsChessPieceActor>(Hit.GetActor());
}

AProjectChairsPlayerState* AProjectChairsChessPlayerController::GetProjectChairsPlayerState() const
{
	return Cast<AProjectChairsPlayerState>(PlayerState);
}

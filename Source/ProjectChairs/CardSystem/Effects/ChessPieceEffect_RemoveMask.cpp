// Copyright Epic Games, Inc. All Rights Reserved.

#include "ChessPieceEffect_RemoveMask.h"
#include "Presentation/ChessPieceActor.h"
#include "Presentation/ChessBoardActor.h"
#include "Logic/ChessData.h"

UChessPieceEffect_RemoveMask::UChessPieceEffect_RemoveMask()
{
	// Instant effect - applies immediately and does not go on the stack
	BaseDuration = 0;
}

void UChessPieceEffect_RemoveMask::OnApply_Implementation(AChessPieceActor* TargetPiece)
{
	Super::OnApply_Implementation(TargetPiece);

	if (!TargetPiece)
	{
		return;
	}

	// Get the board actor to call the replicated request
	AChessBoardActor* BoardActor = Cast<AChessBoardActor>(TargetPiece->GetAttachParentActor());
	if (BoardActor)
	{
		// Use Request function which routes through server for replication
		BoardActor->RequestSetPieceMask(TargetPiece->PieceId, EPieceType::None);
	}
	else
	{
		// Fallback for non-networked scenarios
		TargetPiece->SetMask(EPieceType::None);
	}
}

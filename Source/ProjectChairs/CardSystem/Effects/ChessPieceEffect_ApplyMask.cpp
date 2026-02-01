// Copyright Epic Games, Inc. All Rights Reserved.

#include "ChessPieceEffect_ApplyMask.h"
#include "Presentation/ChessPieceActor.h"
#include "Presentation/ChessBoardActor.h"

UChessPieceEffect_ApplyMask::UChessPieceEffect_ApplyMask()
	: MaskPieceType(EPieceType::Knight)
{
	// Instant effect - applies immediately and does not go on the stack
	BaseDuration = 0;
}

void UChessPieceEffect_ApplyMask::OnApply_Implementation(AChessPieceActor* TargetPiece)
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
		BoardActor->RequestSetPieceMask(TargetPiece->PieceId, MaskPieceType);
	}
	else
	{
		// Fallback for non-networked scenarios
		TargetPiece->SetMask(MaskPieceType);
	}
}

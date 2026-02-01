// Copyright Epic Games, Inc. All Rights Reserved.

#include "ChessPieceEffect_ApplyMask.h"
#include "Presentation/ChessPieceActor.h"

UChessPieceEffect_ApplyMask::UChessPieceEffect_ApplyMask()
	: MaskPieceType(EPieceType::Knight)
{
	// Instant effect - applies immediately and does not go on the stack
	BaseDuration = 0;
}

void UChessPieceEffect_ApplyMask::OnApply_Implementation(AChessPieceActor* TargetPiece)
{
	Super::OnApply_Implementation(TargetPiece);

	if (TargetPiece)
	{
		// Apply the mask to the piece
		// This updates the game model which will broadcast to update visuals
		TargetPiece->SetMask(MaskPieceType);
	}
}

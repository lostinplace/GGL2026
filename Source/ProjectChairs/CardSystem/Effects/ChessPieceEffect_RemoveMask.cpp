// Copyright Epic Games, Inc. All Rights Reserved.

#include "ChessPieceEffect_RemoveMask.h"
#include "Presentation/ChessPieceActor.h"
#include "Logic/ChessData.h"

UChessPieceEffect_RemoveMask::UChessPieceEffect_RemoveMask()
{
	// Instant effect - applies immediately and does not go on the stack
	BaseDuration = 0;
}

void UChessPieceEffect_RemoveMask::OnApply_Implementation(AChessPieceActor* TargetPiece)
{
	Super::OnApply_Implementation(TargetPiece);

	if (TargetPiece)
	{
		// Remove the mask by setting it to None
		TargetPiece->SetMask(EPieceType::None);
	}
}

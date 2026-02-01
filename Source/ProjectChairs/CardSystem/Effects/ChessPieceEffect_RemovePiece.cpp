// Copyright Epic Games, Inc. All Rights Reserved.

#include "ChessPieceEffect_RemovePiece.h"
#include "Presentation/ChessPieceActor.h"
#include "Presentation/ChessBoardActor.h"

UChessPieceEffect_RemovePiece::UChessPieceEffect_RemovePiece()
{
	// Instant effect - applies immediately and does not go on the stack
	BaseDuration = 0;
}

void UChessPieceEffect_RemovePiece::OnApply_Implementation(AChessPieceActor* TargetPiece)
{
	Super::OnApply_Implementation(TargetPiece);

	if (!TargetPiece)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RemovePiece] Target piece is null"));
		return;
	}

	int32 PieceId = TargetPiece->PieceId;
	UE_LOG(LogTemp, Log, TEXT("[RemovePiece] Removing piece %d from board"), PieceId);

	// Get the board actor (parent of the piece)
	AChessBoardActor* BoardActor = Cast<AChessBoardActor>(TargetPiece->GetAttachParentActor());
	if (!BoardActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RemovePiece] Could not find parent ChessBoardActor"));
		return;
	}

	// Use Request function which routes through server for replication
	BoardActor->RequestRemovePiece(PieceId);

	UE_LOG(LogTemp, Log, TEXT("[RemovePiece] Piece %d removal broadcast sent"), PieceId);
}

#include "Presentation/ChessPieceActor.h"
#include "Logic/ChessMoveRule.h"
#include "Logic/ChessBoardState.h"

AChessPieceActor::AChessPieceActor()
{
	PrimaryActorTick.bCanEverTick = false;
	
	// Create a root component so it can be placed
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AChessPieceActor::Init(int32 InPieceId, EPieceType InType, EPieceColor InColor)
{
	PieceId = InPieceId;
	Type = InType;
	Color = InColor;

	// Spawn Rule if class is set
	if (MoveRuleClass)
	{
		FActorSpawnParameters Params;
		Params.Owner = this;
		MoveRuleInstance = GetWorld()->SpawnActor<AChessMoveRule>(MoveRuleClass, Params);
		if (MoveRuleInstance)
		{
			MoveRuleInstance->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
		}
	}

	OnInitialized(PieceId, Type, Color);
}

void AChessPieceActor::UpdateLegalMoves(const UChessBoardState* Board)
{
	CachedMoves.Empty();
	if (MoveRuleInstance && Board)
	{
		// Find my coord
		// Helper from Board? Or pass it in? 
		// For efficiency, caller should probably pass coords, but here we just have Board.
		// Need to scan.
		FBoardCoord MyCoord;
		bool bFound = false;
		for (int32 i = 0; i < 64; ++i)
		{
			if (Board->Squares[i] == PieceId)
			{
				MyCoord = FBoardCoord::FromIndex(i);
				bFound = true;
				break;
			}
		}

		if (bFound)
		{
			FPieceInstance MyInstance(PieceId, Type, Color);
			// We might need to get the actual instance from Board to check 'HasMoved' status
			if (const FPieceInstance* RealInstance = Board->GetPiece(PieceId))
			{
				MyInstance = *RealInstance;
			}
			
			MoveRuleInstance->GenerateMoves(Board, MyCoord, MyInstance, CachedMoves);
		}
	}
}

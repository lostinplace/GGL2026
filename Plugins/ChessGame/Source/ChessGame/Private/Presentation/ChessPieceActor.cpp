#include "Presentation/ChessPieceActor.h"
#include "Presentation/ChessPieceStyleSet.h" // Added Include
#include "Logic/ChessMoveRule.h"
#include "Logic/ChessBoardState.h"
#include "Presentation/SelectableChessPieceComponent.h"
#include "Presentation/ChessBoardActor.h"

AChessPieceActor::AChessPieceActor()
{
	PrimaryActorTick.bCanEverTick = false;
	
	// Create a root component so it can be placed
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	SelectionComponent = CreateDefaultSubobject<USelectableChessPieceComponent>(TEXT("SelectionComponent"));
}

void AChessPieceActor::UpdateVisuals_Implementation(const class UChessPieceStyleSet* StyleSet, EPieceType VisualType)
{
	// Default implementation does nothing. Blueprint can overwrite.
	// UE_LOG(LogTemp, Verbose, TEXT("AChessPieceActor::UpdateVisuals_Implementation Called"));
}

void AChessPieceActor::SetMask(EPieceType NewMask)
{
	if (AActor* BoardActor = GetAttachParentActor())
	{
		if (AChessBoardActor* ChessBoard = Cast<AChessBoardActor>(BoardActor))
		{
			if (ChessBoard->GameModel)
			{
				ChessBoard->GameModel->SetPieceMask(PieceId, NewMask);
			}
		}
	}
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
	// ... existing logic ...
	CachedMoves.Empty();
	if (MoveRuleInstance && Board)
	{
		// ... existing logic ...
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
			if (const FPieceInstance* RealInstance = Board->GetPiece(PieceId))
			{
				MyInstance = *RealInstance;
			}
			
			MoveRuleInstance->GenerateMoves(Board, MyCoord, MyInstance, CachedMoves);
		}
	}
}

void AChessPieceActor::OnSelectionChanged_Implementation(bool bSelected)
{
	// Forward to component
	if (SelectionComponent)
	{
		SelectionComponent->SetSelected(bSelected);
	}

	if (bSelected)
	{
		// Visual pop up
		SetActorRelativeScale3D(FVector(1.2f));
		// Debug message
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, FString::Printf(TEXT("Selected Piece: %d"), PieceId));
	}
	else
	{
		// Reset
		SetActorRelativeScale3D(FVector(1.0f));
	}
}

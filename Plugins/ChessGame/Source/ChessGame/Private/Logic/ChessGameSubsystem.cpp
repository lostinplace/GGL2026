#include "Logic/ChessGameSubsystem.h"

void UChessGameSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	CurrentInteractionState = EChessInteractionState::Idle;
	CurrentSelectedCoord = FBoardCoord(-1, -1);
}

void UChessGameSubsystem::Deinitialize()
{
	Super::Deinitialize();
	ActiveGameModel = nullptr;
}

void UChessGameSubsystem::RegisterGame(UChessGameModel* InGameModel)
{
	ActiveGameModel = InGameModel;
}

void UChessGameSubsystem::SetSelectedCoord(FBoardCoord Coord)
{
	if (CurrentSelectedCoord != Coord)
	{
		CurrentSelectedCoord = Coord;
		
		// Update State
		if (CurrentSelectedCoord.IsValid())
		{
			CurrentInteractionState = EChessInteractionState::PieceSelected;
		}
		else
		{
			CurrentInteractionState = EChessInteractionState::Idle;
		}

		OnSelectionUpdated.Broadcast(CurrentSelectedCoord);
		OnInteractionStateChanged.Broadcast(CurrentInteractionState);
	}
}

void UChessGameSubsystem::ClearSelection()
{
	SetSelectedCoord(FBoardCoord(-1, -1));
}

EPieceColor UChessGameSubsystem::GetSideToMove() const
{
	if (ActiveGameModel && ActiveGameModel->BoardState)
	{
		return ActiveGameModel->BoardState->SideToMove;
	}
	return EPieceColor::White;
}

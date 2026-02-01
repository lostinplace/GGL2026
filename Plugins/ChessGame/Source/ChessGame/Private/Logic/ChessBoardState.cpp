#include "Logic/ChessBoardState.h"

UChessBoardState::UChessBoardState()
{
	InitializeEmpty();
}

void UChessBoardState::InitializeEmpty()
{
	Squares.Init(-1, 64);
	Pieces.Empty();
	SideToMove = EPieceColor::White;
	bHasEnPassantTarget = false;
	HalfmoveClock = 0;
	FullmoveNumber = 1;

	bIsGameOver = false;
	bIsDraw = false;
	bInCheck = false;
	Winner = EPieceColor::White;
}

int32 UChessBoardState::GetPieceIdAt(FBoardCoord Coord) const
{
	if (!Coord.IsValid())
	{
		return -1;
	}
	int32 Index = Coord.ToIndex();
	if (Squares.IsValidIndex(Index))
	{
		return Squares[Index];
	}
	return -1;
}

void UChessBoardState::SetPieceIdAt(FBoardCoord Coord, int32 PieceId)
{
	if (!Coord.IsValid())
	{
		return;
	}
	int32 Index = Coord.ToIndex();
	if (Squares.IsValidIndex(Index))
	{
		Squares[Index] = PieceId;
	}
}

void UChessBoardState::AddPiece(int32 PieceId, EPieceType Type, EPieceColor Color, FBoardCoord Coord)
{
	FPieceInstance NewPiece(PieceId, Type, Color);
	Pieces.Add(PieceId, NewPiece);
	SetPieceIdAt(Coord, PieceId);
}

void UChessBoardState::MovePiece(int32 PieceId, FBoardCoord From, FBoardCoord To)
{
	SetPieceIdAt(From, -1);
	SetPieceIdAt(To, PieceId);
	
	if (FPieceInstance* Piece = Pieces.Find(PieceId))
	{
		Piece->bHasMoved = true;
	}
}

void UChessBoardState::RemovePiece(int32 PieceId)
{
	Pieces.Remove(PieceId);
}

const FPieceInstance* UChessBoardState::GetPiece(int32 PieceId) const
{
	return Pieces.Find(PieceId);
}

FChessBoardStateData UChessBoardState::ToStruct() const
{
	FChessBoardStateData Data;
	Data.Squares = Squares;
	Pieces.GenerateValueArray(Data.PiecesArray);
	Data.SideToMove = SideToMove;
	Data.bHasEnPassantTarget = bHasEnPassantTarget;
	Data.EnPassantTarget = EnPassantTarget;
	Data.HalfmoveClock = HalfmoveClock;
	Data.FullmoveNumber = FullmoveNumber;
	
	Data.bIsGameOver = bIsGameOver;
	Data.bIsDraw = bIsDraw;
	Data.Winner = Winner;
	Data.bInCheck = bInCheck;

	return Data;
}

void UChessBoardState::FromStruct(const FChessBoardStateData& Data)
{
	Squares = Data.Squares;
	Pieces.Empty();
	for (const FPieceInstance& Piece : Data.PiecesArray)
	{
		Pieces.Add(Piece.PieceId, Piece);
	}
	SideToMove = Data.SideToMove;
	bHasEnPassantTarget = Data.bHasEnPassantTarget;
	EnPassantTarget = Data.EnPassantTarget;
	HalfmoveClock = Data.HalfmoveClock;
	FullmoveNumber = Data.FullmoveNumber;

	bIsGameOver = Data.bIsGameOver;
	bIsDraw = Data.bIsDraw;
	Winner = Data.Winner;
	bInCheck = Data.bInCheck;
}


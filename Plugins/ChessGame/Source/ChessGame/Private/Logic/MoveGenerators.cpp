#include "Logic/MoveGenerators.h"

void UMoveGeneratorBase::AddMove(TArray<FChessMove>& OutMoves, FBoardCoord From, FBoardCoord To, const FPieceInstance& MovingPiece, int32 CapturedPieceId) const
{
	FChessMove Move;
	Move.From = From;
	Move.To = To;
	Move.MovingPieceId = MovingPiece.PieceId;
	Move.CapturedPieceId = CapturedPieceId;
	Move.SpecialType = ESpecialMoveType::Normal;
	OutMoves.Add(Move);
}

bool UMoveGeneratorBase::IsSameColor(const UChessBoardState* Board, FBoardCoord Coord, EPieceColor Color) const
{
	if (!Coord.IsValid()) return false;
	int32 PieceId = Board->GetPieceIdAt(Coord);
	if (PieceId == -1) return false;
	
	const FPieceInstance* Piece = Board->Pieces.Find(PieceId);
	return Piece && Piece->Color == Color;
}

bool UMoveGeneratorBase::IsEnemy(const UChessBoardState* Board, FBoardCoord Coord, EPieceColor Color) const
{
	if (!Coord.IsValid()) return false;
	int32 PieceId = Board->GetPieceIdAt(Coord);
	if (PieceId == -1) return false;

	const FPieceInstance* Piece = Board->Pieces.Find(PieceId);
	return Piece && Piece->Color != Color;
}

bool UMoveGeneratorBase::IsEmpty(const UChessBoardState* Board, FBoardCoord Coord) const
{
	if (!Coord.IsValid()) return false;
	return Board->GetPieceIdAt(Coord) == -1;
}

void UMoveGenerator_Sliding::GeneratePseudoMoves(const UChessBoardState* Board, FBoardCoord From, const FPieceInstance& Piece, TArray<FChessMove>& OutMoves) const
{
	TArray<FBoardCoord> Directions;
	if (bOrthogonal)
	{
		Directions.Add(FBoardCoord(1, 0));
		Directions.Add(FBoardCoord(-1, 0));
		Directions.Add(FBoardCoord(0, 1));
		Directions.Add(FBoardCoord(0, -1));
	}
	if (bDiagonal)
	{
		Directions.Add(FBoardCoord(1, 1));
		Directions.Add(FBoardCoord(1, -1));
		Directions.Add(FBoardCoord(-1, 1));
		Directions.Add(FBoardCoord(-1, -1));
	}

	for (const FBoardCoord& Dir : Directions)
	{
		for (int32 i = 1; i < 8; ++i)
		{
			FBoardCoord Target(From.File + Dir.File * i, From.Rank + Dir.Rank * i);
			if (!Target.IsValid()) break;

			if (IsEmpty(Board, Target))
			{
				AddMove(OutMoves, From, Target, Piece);
			}
			else
			{
				if (IsEnemy(Board, Target, Piece.Color))
				{
					AddMove(OutMoves, From, Target, Piece, Board->GetPieceIdAt(Target));
				}
				break; // Blocked
			}
		}
	}
}

void UMoveGenerator_Knight::GeneratePseudoMoves(const UChessBoardState* Board, FBoardCoord From, const FPieceInstance& Piece, TArray<FChessMove>& OutMoves) const
{
	TArray<FBoardCoord> Offsets = {
		{1, 2}, {1, -2}, {-1, 2}, {-1, -2},
		{2, 1}, {2, -1}, {-2, 1}, {-2, -1}
	};

	for (const FBoardCoord& Offset : Offsets)
	{
		FBoardCoord Target(From.File + Offset.File, From.Rank + Offset.Rank);
		if (Target.IsValid())
		{
			if (IsEmpty(Board, Target))
			{
				AddMove(OutMoves, From, Target, Piece);
			}
			else if (IsEnemy(Board, Target, Piece.Color))
			{
				AddMove(OutMoves, From, Target, Piece, Board->GetPieceIdAt(Target));
			}
		}
	}
}

void UMoveGenerator_Pawn::GeneratePseudoMoves(const UChessBoardState* Board, FBoardCoord From, const FPieceInstance& Piece, TArray<FChessMove>& OutMoves) const
{
	int32 Direction = (Piece.Color == EPieceColor::White) ? 1 : -1;
	int32 StartRank = (Piece.Color == EPieceColor::White) ? 1 : 6;
	int32 PromoteRank = (Piece.Color == EPieceColor::White) ? 7 : 0;

	// Forward 1
	FBoardCoord Forward1(From.File, From.Rank + Direction);
	if (Forward1.IsValid() && IsEmpty(Board, Forward1))
	{
		if (Forward1.Rank == PromoteRank)
		{
			// Promotion
			TArray<EPieceType> PromoteTypes = { EPieceType::Queen, EPieceType::Rook, EPieceType::Bishop, EPieceType::Knight };
			for (EPieceType PType : PromoteTypes)
			{
				FChessMove Move;
				Move.From = From;
				Move.To = Forward1;
				Move.MovingPieceId = Piece.PieceId;
				Move.SpecialType = ESpecialMoveType::Promotion;
				Move.PromotionType = PType;
				OutMoves.Add(Move);
			}
		}
		else
		{
			AddMove(OutMoves, From, Forward1, Piece);
			
			// Forward 2
			if (From.Rank == StartRank)
			{
				FBoardCoord Forward2(From.File, From.Rank + Direction * 2);
				if (Forward2.IsValid() && IsEmpty(Board, Forward2))
				{
					AddMove(OutMoves, From, Forward2, Piece);
				}
			}
		}
	}

	// Captures
	int32 CaptureFiles[] = { -1, 1 };
	for (int32 FileOffset : CaptureFiles)
	{
		FBoardCoord Target(From.File + FileOffset, From.Rank + Direction);
		if (Target.IsValid())
		{
			// Normal Capture
			if (IsEnemy(Board, Target, Piece.Color))
			{
				if (Target.Rank == PromoteRank)
				{
					// Promotion Capture
					TArray<EPieceType> PromoteTypes = { EPieceType::Queen, EPieceType::Rook, EPieceType::Bishop, EPieceType::Knight };
					for (EPieceType PType : PromoteTypes)
					{
						FChessMove Move;
						Move.From = From;
						Move.To = Target;
						Move.MovingPieceId = Piece.PieceId;
						Move.CapturedPieceId = Board->GetPieceIdAt(Target);
						Move.SpecialType = ESpecialMoveType::Promotion;
						Move.PromotionType = PType;
						OutMoves.Add(Move);
					}
				}
				else
				{
					AddMove(OutMoves, From, Target, Piece, Board->GetPieceIdAt(Target));
				}
			}
			// En Passant
			else if (Board->bHasEnPassantTarget && Board->EnPassantTarget == Target)
			{
				// Capture the pawn which is at (Target.File, From.Rank)
				FBoardCoord CapturedPawnCoord(Target.File, From.Rank);
				if (IsEnemy(Board, CapturedPawnCoord, Piece.Color))
				{
					FChessMove Move;
					Move.From = From;
					Move.To = Target;
					Move.MovingPieceId = Piece.PieceId;
					Move.CapturedPieceId = Board->GetPieceIdAt(CapturedPawnCoord);
					Move.SpecialType = ESpecialMoveType::EnPassant;
					OutMoves.Add(Move);
				}
			}
		}
	}
}

void UMoveGenerator_King::GeneratePseudoMoves(const UChessBoardState* Board, FBoardCoord From, const FPieceInstance& Piece, TArray<FChessMove>& OutMoves) const
{
	// Normal moves
	for (int32 dx = -1; dx <= 1; ++dx)
	{
		for (int32 dy = -1; dy <= 1; ++dy)
		{
			if (dx == 0 && dy == 0) continue;
			FBoardCoord Target(From.File + dx, From.Rank + dy);
			if (Target.IsValid())
			{
				if (IsEmpty(Board, Target))
				{
					AddMove(OutMoves, From, Target, Piece);
				}
				else if (IsEnemy(Board, Target, Piece.Color))
				{
					AddMove(OutMoves, From, Target, Piece, Board->GetPieceIdAt(Target));
				}
			}
		}
	}

	// Castling
	// Note: We only generate pseudo-castling moves here (checking path clear of pieces).
	// Checking if path is attacked is done in legality filter.
	if (!Piece.bHasMoved)
	{
		int32 Rank = From.Rank;
		// Kingside
		// Check Rook at file 7
		FBoardCoord RookCoord(7, Rank);
		int32 RookId = Board->GetPieceIdAt(RookCoord);
		if (RookId != -1)
		{
			const FPieceInstance* Rook = Board->Pieces.Find(RookId);
			if (Rook && Rook->Type == EPieceType::Rook && Rook->Color == Piece.Color && !Rook->bHasMoved)
			{
				if (IsEmpty(Board, FBoardCoord(5, Rank)) && IsEmpty(Board, FBoardCoord(6, Rank)))
				{
					FChessMove Move;
					Move.From = From;
					Move.To = FBoardCoord(6, Rank);
					Move.MovingPieceId = Piece.PieceId;
					Move.SpecialType = ESpecialMoveType::Castling;
					OutMoves.Add(Move);
				}
			}
		}

		// Queenside
		// Check Rook at file 0
		RookCoord = FBoardCoord(0, Rank);
		RookId = Board->GetPieceIdAt(RookCoord);
		if (RookId != -1)
		{
			const FPieceInstance* Rook = Board->Pieces.Find(RookId);
			if (Rook && Rook->Type == EPieceType::Rook && Rook->Color == Piece.Color && !Rook->bHasMoved)
			{
				if (IsEmpty(Board, FBoardCoord(1, Rank)) && IsEmpty(Board, FBoardCoord(2, Rank)) && IsEmpty(Board, FBoardCoord(3, Rank)))
				{
					FChessMove Move;
					Move.From = From;
					Move.To = FBoardCoord(2, Rank);
					Move.MovingPieceId = Piece.PieceId;
					Move.SpecialType = ESpecialMoveType::Castling;
					OutMoves.Add(Move);
				}
			}
		}
	}
}

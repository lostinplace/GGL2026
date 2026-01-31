#include "Logic/ChessRuleSet.h"
#include "Logic/ChessMoveRule.h"

UChessRuleSet::UChessRuleSet()
{
}

void UChessRuleSet::Initialize(UObject* WorldContextObject)
{
	// Default mappings if not set
	if (MoveRuleClasses.Num() == 0)
	{
		MoveRuleClasses.Add(EPieceType::Rook, AChessMoveRule_Sliding::StaticClass());
		MoveRuleClasses.Add(EPieceType::Bishop, AChessMoveRule_Sliding::StaticClass());
		MoveRuleClasses.Add(EPieceType::Queen, AChessMoveRule_Sliding::StaticClass());
		MoveRuleClasses.Add(EPieceType::Knight, AChessMoveRule_Knight::StaticClass());
		MoveRuleClasses.Add(EPieceType::Pawn, AChessMoveRule_Pawn::StaticClass());
		MoveRuleClasses.Add(EPieceType::King, AChessMoveRule_King::StaticClass());
	}
	
	// Spawn instances
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : GetWorld();
	if (!World) return;
	// Wait, RuleSet is UObject. GetWorld might return null or outer's world.
	// If RuleSet is owned by GameModel (UObject), GetWorld() requires implementation or outer chain.
	
	// Assuming RuleSet is created within an Actor's world context (GameModel created in BoardActor).
	
	FActorSpawnParameters Params;
	Params.ObjectFlags |= RF_Transient;

	for (const auto& Pair : MoveRuleClasses)
	{
		if (Pair.Value)
		{
			AChessMoveRule* Rule = World->SpawnActor<AChessMoveRule>(Pair.Value, Params);
			if (Rule)
			{
				// Configure Sliding rules specifically if using the generic class
				if (AChessMoveRule_Sliding* Sliding = Cast<AChessMoveRule_Sliding>(Rule))
				{
					if (Pair.Key == EPieceType::Rook) Sliding->bOrthogonal = true;
					if (Pair.Key == EPieceType::Bishop) Sliding->bDiagonal = true;
					if (Pair.Key == EPieceType::Queen) { Sliding->bOrthogonal = true; Sliding->bDiagonal = true; }
				}
				MoveRules.Add(Pair.Key, Rule);
			}
		}
	}
}

void UChessRuleSet::SetupInitialBoardState(UChessBoardState* BoardState, EChessInitMode InitMode)
{
	BoardState->InitializeEmpty();

	// Helper to add pieces
	int32 NextId = 0;
	auto AddPiece = [&](EPieceType Type, EPieceColor Color, int32 File, int32 Rank) {
		BoardState->AddPiece(NextId++, Type, Color, FBoardCoord(File, Rank));
	};

	if (InitMode == EChessInitMode::Standard)
	{
		// White Pieces (Rank 0 and 1)
		for (int i = 0; i < 8; ++i) AddPiece(EPieceType::Pawn, EPieceColor::White, i, 1);
		AddPiece(EPieceType::Rook, EPieceColor::White, 0, 0);
		AddPiece(EPieceType::Knight, EPieceColor::White, 1, 0);
		AddPiece(EPieceType::Bishop, EPieceColor::White, 2, 0);
		AddPiece(EPieceType::Queen, EPieceColor::White, 3, 0);
		AddPiece(EPieceType::King, EPieceColor::White, 4, 0);
		AddPiece(EPieceType::Bishop, EPieceColor::White, 5, 0);
		AddPiece(EPieceType::Knight, EPieceColor::White, 6, 0);
		AddPiece(EPieceType::Rook, EPieceColor::White, 7, 0);

		// Black Pieces (Rank 7 and 6)
		for (int i = 0; i < 8; ++i) AddPiece(EPieceType::Pawn, EPieceColor::Black, i, 6);
		AddPiece(EPieceType::Rook, EPieceColor::Black, 0, 7);
		AddPiece(EPieceType::Knight, EPieceColor::Black, 1, 7);
		AddPiece(EPieceType::Bishop, EPieceColor::Black, 2, 7);
		AddPiece(EPieceType::Queen, EPieceColor::Black, 3, 7);
		AddPiece(EPieceType::King, EPieceColor::Black, 4, 7);
		AddPiece(EPieceType::Bishop, EPieceColor::Black, 5, 7);
		AddPiece(EPieceType::Knight, EPieceColor::Black, 6, 7);
		AddPiece(EPieceType::Rook, EPieceColor::Black, 7, 7);
	}
	// Test Mode
	else if (InitMode == EChessInitMode::Test_KingsOnly)
	{
		// Just Kings for testing
		AddPiece(EPieceType::King, EPieceColor::White, 4, 0);
		AddPiece(EPieceType::King, EPieceColor::Black, 4, 7);
	}
	
	// Better approach: Modify the condition to allow standard setup, then apply mask.
	else if (InitMode == EChessInitMode::Test_MaskedPawns)
	{
		// 1. Setup Standard Board
		// White
		AddPiece(EPieceType::Rook, EPieceColor::White, 0, 0);
		AddPiece(EPieceType::Knight, EPieceColor::White, 1, 0);
		AddPiece(EPieceType::Bishop, EPieceColor::White, 2, 0);
		AddPiece(EPieceType::Queen, EPieceColor::White, 3, 0);
		AddPiece(EPieceType::King, EPieceColor::White, 4, 0);
		AddPiece(EPieceType::Bishop, EPieceColor::White, 5, 0);
		AddPiece(EPieceType::Knight, EPieceColor::White, 6, 0);
		AddPiece(EPieceType::Rook, EPieceColor::White, 7, 0);
		for (int i = 0; i < 8; i++) AddPiece(EPieceType::Pawn, EPieceColor::White, i, 1);

		// Black
		AddPiece(EPieceType::Rook, EPieceColor::Black, 0, 7);
		AddPiece(EPieceType::Knight, EPieceColor::Black, 1, 7);
		AddPiece(EPieceType::Bishop, EPieceColor::Black, 2, 7);
		AddPiece(EPieceType::Queen, EPieceColor::Black, 3, 7);
		AddPiece(EPieceType::King, EPieceColor::Black, 4, 7);
		AddPiece(EPieceType::Bishop, EPieceColor::Black, 5, 7);
		AddPiece(EPieceType::Knight, EPieceColor::Black, 6, 7);
		AddPiece(EPieceType::Rook, EPieceColor::Black, 7, 7);
		for (int i = 0; i < 8; i++) AddPiece(EPieceType::Pawn, EPieceColor::Black, i, 6);

		// 2. Apply Masks
		for (auto& Pair : BoardState->Pieces)
		{
			// Mask everything as Pawn (except maybe Kings?)
			// User said "all of the pieces". Let's do all.
			// Ideally King mask might be confusing if it looks like a Pawn.
			// But for testing "Mask Logic", it's fine.
			Pair.Value.MaskType = EPieceType::Pawn;
		}
	}
	else if (InitMode == EChessInitMode::Random960)
	{
		// 1. Setup empty back rank slots
		TArray<int32> Slots;
		for(int i=0; i<8; ++i) Slots.Add(i);
		
		int32 Placement[8]; // Stores Piece Type by File Index
		
		// 2. Place Bishops (Opposite Colors)
		int32 Bishop1Pos = FMath::RandRange(0, 3) * 2; // 0, 2, 4, 6
		int32 Bishop2Pos = FMath::RandRange(0, 3) * 2 + 1; // 1, 3, 5, 7
		
		Placement[Bishop1Pos] = (int32)EPieceType::Bishop;
		Placement[Bishop2Pos] = (int32)EPieceType::Bishop;
		
		Slots.Remove(Bishop1Pos);
		Slots.Remove(Bishop2Pos);
		
		// 3. Place Queen
		int32 QueenIndex = FMath::RandRange(0, Slots.Num() - 1);
		int32 QueenPos = Slots[QueenIndex];
		Placement[QueenPos] = (int32)EPieceType::Queen;
		Slots.RemoveAt(QueenIndex);
		
		// 4. Place Knights
		for(int k=0; k<2; ++k)
		{
			int32 KnightIndex = FMath::RandRange(0, Slots.Num() - 1);
			int32 KnightPos = Slots[KnightIndex];
			Placement[KnightPos] = (int32)EPieceType::Knight;
			Slots.RemoveAt(KnightIndex);
		}
		
		// 5. Place Rooks and King (Left Rook, King, Right Rook)
		// Slots now has 3 items. They are already sorted because we removed from original list?
		// No, RemoveAt preserves order of remaining elements? Yes, TArray keeps order.
		// So Slots[0] is Leftmost Empty, Slots[1] is Middle Empty, Slots[2] is Rightmost Empty.
		
		Placement[Slots[0]] = (int32)EPieceType::Rook;
		Placement[Slots[1]] = (int32)EPieceType::King; // Designates "Center" of K-R relation
		Placement[Slots[2]] = (int32)EPieceType::Rook;
		
		// 6. Apply to Board (White and Black Mirrored)
		for(int i=0; i<8; ++i)
		{
			AddPiece((EPieceType)Placement[i], EPieceColor::White, i, 0);
			AddPiece((EPieceType)Placement[i], EPieceColor::Black, i, 7);
		}
		
		// Pawns
		for (int i = 0; i < 8; ++i) 
		{
			AddPiece(EPieceType::Pawn, EPieceColor::White, i, 1);
			AddPiece(EPieceType::Pawn, EPieceColor::Black, i, 6);
		}
	}
	// Empty: do nothing
}

void UChessRuleSet::GeneratePseudoLegalMoves(const UChessBoardState* Board, int32 PieceId, TArray<FChessMove>& OutMoves)
{
	const FPieceInstance* Piece = Board->Pieces.Find(PieceId);
	if (!Piece) return;
	
	// 1. Generate Canonical Moves (Move + Capture)
	if (AChessMoveRule** RulePtr = MoveRules.Find(Piece->Type))
	{
		if (AChessMoveRule* Rule = *RulePtr)
		{
			// Find coord
			FBoardCoord From;
			bool bFound = false;
			for (int32 i = 0; i < 64; ++i)
			{
				if (Board->Squares[i] == PieceId)
				{
					From = FBoardCoord::FromIndex(i);
					bFound = true;
					break;
				}
			}

			if (bFound)
			{
				Rule->GenerateMoves(Board, From, *Piece, OutMoves);

				// 2. Generate Mask Moves (Move ONLY, No Capture)
				if (Piece->MaskType != EPieceType::None && Piece->MaskType != Piece->Type)
				{
					if (AChessMoveRule** MaskRulePtr = MoveRules.Find(Piece->MaskType))
					{
						if (AChessMoveRule* MaskRule = *MaskRulePtr)
						{
							TArray<FChessMove> MaskMoves;
							// Create a "Fake" piece with MaskType for the generator (so Pawns know direction etc)
							FPieceInstance MaskPiece = *Piece;
							MaskPiece.Type = Piece->MaskType;
							
							MaskRule->GenerateMoves(Board, From, MaskPiece, MaskMoves);

							// Filter: Add only NON-CAPTURING moves
							for (const FChessMove& MMove : MaskMoves)
							{
								if (MMove.CapturedPieceId == -1 && MMove.SpecialType != ESpecialMoveType::EnPassant)
								{
									// Avoid duplicates
									bool bExists = false;
									for (const FChessMove& Existing : OutMoves)
									{
										if (Existing.To == MMove.To)
										{
											bExists = true; 
											break;
										}
									}
									if (!bExists)
									{
										// Restore MovingPieceId to real piece ID (GenerateMoves might have used the copy's ID which is same, but let's be safe)
										// The copy had same ID, so it's fine.
										// But ensure SpecialType logic (like Promotion) is consistent.
										// If Mask is Pawn, and it reaches end -> Promotion?
										// User didn't specify. Assuming yes, but they promote to what? Masked Queen? 
										// For MVP, allow the movement.
										OutMoves.Add(MMove);
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

void UChessRuleSet::GenerateLegalMoves(const UChessBoardState* Board, int32 PieceId, TArray<FChessMove>& OutMoves)
{
	TArray<FChessMove> PseudoMoves;
	GeneratePseudoLegalMoves(Board, PieceId, PseudoMoves);

	for (const FChessMove& Move : PseudoMoves)
	{
		if (IsMoveLegal(Board, Move))
		{
			OutMoves.Add(Move);
		}
	}
}

bool UChessRuleSet::IsMoveLegal(const UChessBoardState* Board, const FChessMove& Move)
{
	// 1. Clone board
	UChessBoardState* TempBoard = NewObject<UChessBoardState>();
	TempBoard->Squares = Board->Squares;
	TempBoard->Pieces = Board->Pieces;
	TempBoard->SideToMove = Board->SideToMove;
	TempBoard->bHasEnPassantTarget = Board->bHasEnPassantTarget;
	TempBoard->EnPassantTarget = Board->EnPassantTarget;
	
	// Apply Move to TempBoard
	TempBoard->SetPieceIdAt(Move.From, -1);
	TempBoard->SetPieceIdAt(Move.To, Move.MovingPieceId);
	
	// Handle capture
	if (Move.CapturedPieceId != -1)
	{
		TempBoard->RemovePiece(Move.CapturedPieceId);
	}
	// Note: EnPassant capture removes a different pawn
	if (Move.SpecialType == ESpecialMoveType::EnPassant)
	{
		FBoardCoord CapturedCoord(Move.To.File, Move.From.Rank);
		TempBoard->SetPieceIdAt(CapturedCoord, -1);
		// Assuming proper captured piece ID handling in generator/logic
	}
	
	// Check if King is in Check
	const FPieceInstance* MovingPiece = Board->Pieces.Find(Move.MovingPieceId);
	if (MovingPiece)
	{
		return !IsKingInCheck(TempBoard, MovingPiece->Color);
	}
	
	return false;
}

FBoardCoord UChessRuleSet::FindKing(const UChessBoardState* Board, EPieceColor Color) const
{
	for (const auto& Pair : Board->Pieces)
	{
		if (Pair.Value.Type == EPieceType::King && Pair.Value.Color == Color)
		{
			// Need location
			for (int32 i = 0; i < 64; ++i)
			{
				if (Board->Squares[i] == Pair.Key)
				{
					return FBoardCoord::FromIndex(i);
				}
			}
		}
	}
	return FBoardCoord(); // Invalid
}

bool UChessRuleSet::IsKingInCheck(const UChessBoardState* Board, EPieceColor Color)
{
	FBoardCoord KingPos = FindKing(Board, Color);
	if (!KingPos.IsValid()) return false; 

	EPieceColor EnemyColor = (Color == EPieceColor::White) ? EPieceColor::Black : EPieceColor::White;

	for (const auto& Pair : Board->Pieces)
	{
		if (Pair.Value.Color == EnemyColor)
		{
			if (AChessMoveRule** RulePtr = MoveRules.Find(Pair.Value.Type))
			{
				AChessMoveRule* Rule = *RulePtr;
				if (!Rule) continue; 

				// Get location of enemy
				FBoardCoord EnemyPos;
				bool bFound = false;
				for (int32 i = 0; i < 64; ++i)
				{
					if (Board->Squares[i] == Pair.Value.PieceId)
					{
						EnemyPos = FBoardCoord::FromIndex(i);
						bFound = true;
						break;
					}
				}

				if (bFound)
				{
					TArray<FChessMove> Moves;
					Rule->GenerateMoves(Board, EnemyPos, Pair.Value, Moves);
					for (const FChessMove& Move : Moves)
					{
						if (Move.To == KingPos)
						{
							return true;
						}
					}
				}
			}
		}
	}

	return false;
}

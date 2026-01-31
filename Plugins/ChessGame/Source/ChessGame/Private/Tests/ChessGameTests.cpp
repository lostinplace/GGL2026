#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Logic/ChessData.h"
#include "Logic/ChessBoardState.h"
#include "Logic/ChessRuleSet.h"
#include "Logic/ChessGameModel.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChessGameCoordinateTest, "ChessGame.Logic.Coordinates", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChessGameCoordinateTest::RunTest(const FString& Parameters)
{
	FBoardCoord A1(0, 0);
	TestEqual(TEXT("A1 Index"), A1.ToIndex(), 0);
	TestEqual(TEXT("A1 From Index"), FBoardCoord::FromIndex(0), A1);

	FBoardCoord H8(7, 7);
	TestEqual(TEXT("H8 Index"), H8.ToIndex(), 63);
	TestEqual(TEXT("H8 From Index"), FBoardCoord::FromIndex(63), H8);

	TestTrue(TEXT("A1 Valid"), A1.IsValid());
	TestFalse(TEXT("Invalid Coord"), FBoardCoord(-1, 0).IsValid());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChessGameInitialSetupTest, "ChessGame.Logic.InitialSetup", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChessGameInitialSetupTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	WorldContext.SetCurrentWorld(World);

	UChessRuleSet* RuleSet = NewObject<UChessRuleSet>();
	RuleSet->Initialize(World);

	UChessBoardState* Board = NewObject<UChessBoardState>();
	RuleSet->SetupInitialBoardState(Board);

	// Check White Pieces
	TestEqual(TEXT("White Rook at A1"), Board->GetPieceIdAt(FBoardCoord(0, 0)) != -1, true);
	const FPieceInstance* Piece = Board->GetPiece(Board->GetPieceIdAt(FBoardCoord(0, 0)));
	if (Piece)
	{
		TestEqual(TEXT("Is White"), Piece->Color, EPieceColor::White);
		TestEqual(TEXT("Is Rook"), Piece->Type, EPieceType::Rook);
	}

	// Check Black King at E8 (4, 7)
	Piece = Board->GetPiece(Board->GetPieceIdAt(FBoardCoord(4, 7)));
	if (Piece)
	{
		TestEqual(TEXT("Is Black"), Piece->Color, EPieceColor::Black);
		TestEqual(TEXT("Is King"), Piece->Type, EPieceType::King);
	}

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChessGamePawnMoveTest, "ChessGame.Logic.PawnMoves", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChessGamePawnMoveTest::RunTest(const FString& Parameters)
{
	// Create Temp World for Actors
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	WorldContext.SetCurrentWorld(World);

	UChessRuleSet* RuleSet = NewObject<UChessRuleSet>();
	RuleSet->Initialize(World);
	UChessBoardState* Board = NewObject<UChessBoardState>();
	Board->InitializeEmpty();

	// Add White Pawn at E2 (4, 1)
	Board->AddPiece(0, EPieceType::Pawn, EPieceColor::White, FBoardCoord(4, 1));
	// Add Black Pawn at D3 (3, 2) (Capture target)
	Board->AddPiece(1, EPieceType::Pawn, EPieceColor::Black, FBoardCoord(3, 2));

	TArray<FChessMove> Moves;
	RuleSet->GenerateLegalMoves(Board, 0, Moves);

	// Expected: Forward to E3, Double Forward to E4, Capture D3
	bool bFoundE3 = false;
	bool bFoundE4 = false;
	bool bFoundD3 = false;

	for (const FChessMove& Move : Moves)
	{
		if (Move.To == FBoardCoord(4, 2)) bFoundE3 = true;
		if (Move.To == FBoardCoord(4, 3)) bFoundE4 = true;
		if (Move.To == FBoardCoord(3, 2)) bFoundD3 = true;
	}

	TestTrue(TEXT("Found E3"), bFoundE3);
	TestTrue(TEXT("Found E4"), bFoundE4);
	TestTrue(TEXT("Found Capture D3"), bFoundD3);

	// Clean up
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);

	return true;
}

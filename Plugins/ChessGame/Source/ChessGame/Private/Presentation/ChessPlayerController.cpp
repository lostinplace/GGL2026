#include "Presentation/ChessPlayerController.h"

#include "Presentation/ChessPieceActor.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "EngineUtils.h"

AChessPlayerController::AChessPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void AChessPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Auto-find board if single player / single board
	if (!CurrentBoard)
	{
		// Find first board actor
		for (TActorIterator<AChessBoardActor> It(GetWorld()); It; ++It)
		{
			CurrentBoard = *It;
			break;
		}
	}
}

void AChessPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	InputComponent->BindAction("LeftClick", IE_Pressed, this, &AChessPlayerController::OnMouseClick);
	// We can also bind "Touch" for mobile
}

void AChessPlayerController::OnMouseClick()
{
	FVector HitLoc;
	AChessBoardActor* HitBoard = FindBoardUnderCursor(HitLoc);
	
	if (HitBoard)
	{
		// Convert World Hit to Coord
		FBoardCoord Coord = HitBoard->WorldToCoord(HitLoc);
		
		// If valid, interact
		if (Coord.IsValid()) // WorldToCoord might return invalid if we check bounds, but currently it just divides
		{
			// Optional: Check bounds manually if WorldToCoord doesn't
			if (Coord.File >= 0 && Coord.File <= 7 && Coord.Rank >= 0 && Coord.Rank <= 7)
			{
				HitBoard->HandleSquareClicked(Coord);
			}
			else
			{
				// Clicked outside grid but on board actor components?
				// Deselect?
				HitBoard->HandleSquareClicked(FBoardCoord(-1, -1));
			}
		}
	}
	else
	{
		// Clicked nothing / void
		// If we define "Deselect on void click" logic:
		if (CurrentBoard)
		{
			CurrentBoard->HandleSquareClicked(FBoardCoord(-1, -1));
		}
	}
}

AChessBoardActor* AChessPlayerController::FindBoardUnderCursor(FVector& OutHitLocation)
{
	FHitResult Hit;
	GetHitResultUnderCursor(ECC_Visibility, false, Hit);

	if (Hit.bBlockingHit && Hit.GetActor())
	{
		OutHitLocation = Hit.Location;
		
		// Did we hit the board directly?
		if (AChessBoardActor* Board = Cast<AChessBoardActor>(Hit.GetActor()))
		{
			return Board;
		}
		
		// Did we hit a piece?
		if (AChessPieceActor* Piece = Cast<AChessPieceActor>(Hit.GetActor()))
		{
			// Return its owner (the board) logic? 
			// Piece isn't usually owned by Board in Scene Hierarchy unless attached.
			// Currently we AttachToActor in SpawnPieceActor. So GetOwner should work.
			// Or GetAttachParentActor.
			if (Piece->GetAttachParentActor())
			{
				if (AChessBoardActor* Board = Cast<AChessBoardActor>(Piece->GetAttachParentActor()))
				{
					// If we hit a piece, the "Location" is on the piece surface, which might be high up (Z).
					// We need the board-plane location to determine the square.
					// Ideally, we raycast against the BOARD PLANE, ignoring the piece for coordinates.
					// BUT, we know which piece it is.
					
					// Alternative: Piece->PieceId -> Board->State->GetSquareOfPiece -> Coord
					// Much more accurate than raycasting against a mesh.
					
					// Let's rely on logic for piece hits.
					
					// However, the caller expects a Board + Location to convert.
					// If we return Board, the caller will convert "Piece Surface Location" to Coord.
					// This works IF WorldToCoord handles Z (it ignores Z usually).
					
					// But if we hit the top of a tall piece, X/Y might be correct.
					
					return Board;
				}
			}
		}
		
		// Did we hit the Board Mesh (which might be a child component / attached)?
		// If the user made a separate actor for the table?
		// We only support direct Board Actor hits for now.
	}
	
	return nullptr;
}

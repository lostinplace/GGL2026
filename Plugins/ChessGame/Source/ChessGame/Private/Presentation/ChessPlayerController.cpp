#include "Presentation/ChessPlayerController.h"
#include "Presentation/SelectableChessPieceComponent.h"

#include "Presentation/ChessPieceActor.h"
#include "Logic/ChessGameSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "EngineUtils.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

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

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (ClickAction)
		{
			EnhancedInputComponent->BindAction(ClickAction, ETriggerEvent::Started, this, &AChessPlayerController::OnMouseClick);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ChessPlayerController: InputComponent is not EnhancedInputComponent. Check Project Settings -> Input -> Default Classes."));
	}
}

void AChessPlayerController::OnMouseClick()
{
	// Turn validation for multiplayer
	if (!CanInteract())
	{
		UE_LOG(LogTemp, Warning, TEXT("[ChessController] Not your turn! Waiting for opponent."));
		return;
	}

	FVector HitLoc;
	AChessBoardActor* HitBoard = FindBoardUnderCursor(HitLoc);

	if (HitBoard)
	{
		// Convert World Hit to Coord
		FBoardCoord Coord = HitBoard->WorldToCoord(HitLoc);
		
		UE_LOG(LogTemp, Warning, TEXT("[ChessController] Board Found: %s. HitLoc: %s. Coord: %d, %d"), *HitBoard->GetName(), *HitLoc.ToString(), Coord.File, Coord.Rank);

		// If valid, interact
		if (Coord.IsValid()) 
		{
			// Check logic
			if (Coord.File >= 0 && Coord.File <= 7 && Coord.Rank >= 0 && Coord.Rank <= 7)
			{
				HitBoard->HandleSquareClicked(Coord);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[ChessController] Coord out of bounds: %d, %d"), Coord.File, Coord.Rank);
				HitBoard->HandleSquareClicked(FBoardCoord(-1, -1));
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[ChessController] Invalid Coord calculated from HitLoc."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[ChessController] No Board Found under cursor."));
		
		// Debug what we DID hit
		FHitResult Hit;
		if (GetHitResultUnderCursor(ECC_Visibility, false, Hit))
		{
			UE_LOG(LogTemp, Warning, TEXT("[ChessController] Hit Actor: %s"), *GetNameSafe(Hit.GetActor()));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[ChessController] No Hit Result at all."));
		}

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
		
		// 1. Prioritize "Selectable" components
		// If we hit an actor that has this component, it's a piece we can interact with.
		if (USelectableChessPieceComponent* Selectable = USelectableChessPieceComponent::FindSelectableComponent(Hit.GetActor()))
		{
			// The component is on the actor, but we need the Board.
			// Assumption: The Actor *is* the ChessPieceActor or related.
			if (AChessPieceActor* Piece = Cast<AChessPieceActor>(Hit.GetActor()))
			{
				if (Piece->GetAttachParentActor())
				{
					if (AChessBoardActor* Board = Cast<AChessBoardActor>(Piece->GetAttachParentActor()))
					{
						return Board;
					}
				}
			}
		}

		// 2. Direct Board Hit
		if (AChessBoardActor* Board = Cast<AChessBoardActor>(Hit.GetActor()))
		{
			return Board;
		}
		
		// 3. Fallback: Hit a piece (Actor) directly (legacy support or if Component search fails but cast works)
		if (AChessPieceActor* Piece = Cast<AChessPieceActor>(Hit.GetActor()))
		{
			if (Piece->GetAttachParentActor())
			{
				if (AChessBoardActor* Board = Cast<AChessBoardActor>(Piece->GetAttachParentActor()))
				{
					return Board;
				}
			}
		}
	}
	
	return nullptr;
}

bool AChessPlayerController::CanInteract() const
{
	// If no assigned color (single player mode), always allow interaction
	if (!HasAssignedColor())
	{
		return true;
	}

	// Get the current side to move from the subsystem
	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		return true;
	}

	UChessGameSubsystem* ChessSubsystem = GI->GetSubsystem<UChessGameSubsystem>();
	if (!ChessSubsystem)
	{
		return true;
	}

	// Only allow interaction if it's this player's turn
	EPieceColor SideToMove = ChessSubsystem->GetSideToMove();
	EPieceColor MyColor = GetAssignedColor();

	return SideToMove == MyColor;
}

EPieceColor AChessPlayerController::GetAssignedColor() const
{
	// Check PlayerState for assigned color
	// PlayerState needs to have AssignedChessColor and bHasAssignedColor properties
	if (PlayerState)
	{
		// Use reflection to check if the PlayerState has these properties
		// This avoids a hard dependency on ProjectChairsPlayerState in the plugin
		UFunction* GetColorFunc = PlayerState->GetClass()->FindFunctionByName(TEXT("GetAssignedChessColor"));
		if (GetColorFunc)
		{
			// Has the function, call it
			struct { EPieceColor ReturnValue; } Params;
			PlayerState->ProcessEvent(GetColorFunc, &Params);
			return Params.ReturnValue;
		}

		// Fallback: Try to read the property directly via reflection
		FProperty* ColorProp = PlayerState->GetClass()->FindPropertyByName(TEXT("AssignedChessColor"));
		if (ColorProp)
		{
			EPieceColor* ColorPtr = ColorProp->ContainerPtrToValuePtr<EPieceColor>(PlayerState);
			if (ColorPtr)
			{
				return *ColorPtr;
			}
		}
	}

	return EPieceColor::White;
}

bool AChessPlayerController::HasAssignedColor() const
{
	if (PlayerState)
	{
		// Check via reflection to avoid hard dependency
		FProperty* HasColorProp = PlayerState->GetClass()->FindPropertyByName(TEXT("bHasAssignedColor"));
		if (HasColorProp)
		{
			bool* HasColorPtr = HasColorProp->ContainerPtrToValuePtr<bool>(PlayerState);
			if (HasColorPtr)
			{
				return *HasColorPtr;
			}
		}
	}

	return false;
}

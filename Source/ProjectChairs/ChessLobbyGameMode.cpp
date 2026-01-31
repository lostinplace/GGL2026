// Copyright Epic Games, Inc. All Rights Reserved.

#include "ChessLobbyGameMode.h"
#include "ChessGameState.h"
#include "ProjectChairsPlayerState.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "EngineUtils.h"

AChessLobbyGameMode::AChessLobbyGameMode()
{
	GameStateClass = AChessGameState::StaticClass();
	PlayerStateClass = AProjectChairsPlayerState::StaticClass();
}

void AChessLobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// Assign color to the new player
	AssignPlayerColor(NewPlayer);

	UE_LOG(LogTemp, Log, TEXT("[ChessLobbyGameMode] Player logged in: %s"), *GetNameSafe(NewPlayer));
}

void AChessLobbyGameMode::Logout(AController* Exiting)
{
	AChessGameState* ChessState = GetChessGameState();
	if (ChessState)
	{
		APlayerController* PC = Cast<APlayerController>(Exiting);
		if (PC && PC->PlayerState)
		{
			// Clear the player from their slot and release spawn reservation
			if (ChessState->WhitePlayer == PC->PlayerState)
			{
				ChessState->WhitePlayer = nullptr;
				bWhiteSpawnReserved = false;
				UE_LOG(LogTemp, Log, TEXT("[ChessLobbyGameMode] White player disconnected"));
			}
			else if (ChessState->BlackPlayer == PC->PlayerState)
			{
				ChessState->BlackPlayer = nullptr;
				bBlackSpawnReserved = false;
				UE_LOG(LogTemp, Log, TEXT("[ChessLobbyGameMode] Black player disconnected"));
			}
		}
	}

	Super::Logout(Exiting);
}

AActor* AChessLobbyGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	// Reserve a spawn slot immediately to handle simultaneous joins
	FName DesiredTag;
	if (!bWhiteSpawnReserved)
	{
		bWhiteSpawnReserved = true;
		DesiredTag = FName("White");
	}
	else if (!bBlackSpawnReserved)
	{
		bBlackSpawnReserved = true;
		DesiredTag = FName("Black");
	}
	else
	{
		// Both slots taken - spectator or reject
		UE_LOG(LogTemp, Warning, TEXT("[ChessLobbyGameMode] Both spawn slots reserved, using default spawn"));
		return Super::ChoosePlayerStart_Implementation(Player);
	}

	// Find a PlayerStart with the matching tag
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		APlayerStart* PlayerStart = *It;
		if (PlayerStart->PlayerStartTag == DesiredTag)
		{
			UE_LOG(LogTemp, Log, TEXT("[ChessLobbyGameMode] Spawning player at %s PlayerStart"), *DesiredTag.ToString());
			return PlayerStart;
		}
	}

	// Fallback to default behavior if no tagged spawn found
	UE_LOG(LogTemp, Warning, TEXT("[ChessLobbyGameMode] No PlayerStart with tag '%s' found, using default"), *DesiredTag.ToString());
	return Super::ChoosePlayerStart_Implementation(Player);
}

void AChessLobbyGameMode::AssignPlayerColor(APlayerController* PlayerController)
{
	if (!PlayerController || !PlayerController->PlayerState)
	{
		return;
	}

	AChessGameState* ChessState = GetChessGameState();
	if (!ChessState)
	{
		return;
	}

	AProjectChairsPlayerState* PS = Cast<AProjectChairsPlayerState>(PlayerController->PlayerState);
	if (!PS)
	{
		return;
	}

	// Assign White to first player, Black to second
	if (ChessState->WhitePlayer == nullptr)
	{
		ChessState->WhitePlayer = PS;
		PS->AssignedChessColor = EPieceColor::White;
		PS->bHasAssignedColor = true;
		UE_LOG(LogTemp, Log, TEXT("[ChessLobbyGameMode] Assigned WHITE to player: %s"), *PS->GetPlayerName());
	}
	else if (ChessState->BlackPlayer == nullptr)
	{
		ChessState->BlackPlayer = PS;
		PS->AssignedChessColor = EPieceColor::Black;
		PS->bHasAssignedColor = true;
		UE_LOG(LogTemp, Log, TEXT("[ChessLobbyGameMode] Assigned BLACK to player: %s"), *PS->GetPlayerName());
	}
	else
	{
		// Game is full - could kick the player or make them spectator
		UE_LOG(LogTemp, Warning, TEXT("[ChessLobbyGameMode] Game is full, player %s cannot join"), *PS->GetPlayerName());
	}
}

AChessGameState* AChessLobbyGameMode::GetChessGameState() const
{
	return Cast<AChessGameState>(GameState);
}

bool AChessLobbyGameMode::CanStartGame() const
{
	AChessGameState* ChessState = GetChessGameState();
	return ChessState && ChessState->AreAllPlayersReady() && !ChessState->bGameStarted;
}

void AChessLobbyGameMode::StartChessGame()
{
	AChessGameState* ChessState = GetChessGameState();
	if (ChessState && CanStartGame())
	{
		ChessState->bGameStarted = true;
		UE_LOG(LogTemp, Log, TEXT("[ChessLobbyGameMode] Chess game started!"));
	}
}

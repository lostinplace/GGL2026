// Copyright Epic Games, Inc. All Rights Reserved.

#include "ChessMainMenuGameMode.h"
#include "UI/ChessLobbyWidget.h"

AChessMainMenuGameMode::AChessMainMenuGameMode()
{
	// Use default pawn and controller for menu
	DefaultPawnClass = nullptr;
}

void AChessMainMenuGameMode::BeginPlay()
{
	Super::BeginPlay();

	// Automatically show the lobby widget
	LobbyWidget = UChessLobbyWidget::ShowLobby(this);
}

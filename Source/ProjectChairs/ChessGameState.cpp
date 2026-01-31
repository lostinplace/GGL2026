// Copyright Epic Games, Inc. All Rights Reserved.

#include "ChessGameState.h"
#include "ProjectChairsPlayerState.h"
#include "Net/UnrealNetwork.h"

AChessGameState::AChessGameState()
	: WhitePlayer(nullptr)
	, BlackPlayer(nullptr)
	, bGameStarted(false)
{
}

void AChessGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AChessGameState, WhitePlayer);
	DOREPLIFETIME(AChessGameState, BlackPlayer);
	DOREPLIFETIME(AChessGameState, bGameStarted);
}

EPieceColor AChessGameState::GetPlayerColor(APlayerState* PlayerState) const
{
	if (PlayerState == WhitePlayer)
	{
		return EPieceColor::White;
	}
	return EPieceColor::Black;
}

bool AChessGameState::HasPlayerColor(APlayerState* PlayerState) const
{
	return PlayerState == WhitePlayer || PlayerState == BlackPlayer;
}

bool AChessGameState::AreAllPlayersReady() const
{
	return WhitePlayer != nullptr && BlackPlayer != nullptr;
}

APlayerState* AChessGameState::GetPlayerForColor(EPieceColor Color) const
{
	if (Color == EPieceColor::White)
	{
		return WhitePlayer;
	}
	return BlackPlayer;
}

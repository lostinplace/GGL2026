// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Presentation/ChessPieceActor.h"
#include "ProjectChairsChessPieceActor.generated.h"

/**
 * Project Chairs specific chess piece actor with game-specific functionality.
 */
UCLASS(Blueprintable)
class PROJECTCHAIRS_API AProjectChairsChessPieceActor : public AChessPieceActor
{
	GENERATED_BODY()

public:
	AProjectChairsChessPieceActor();

protected:
	virtual void BeginPlay() override;
};

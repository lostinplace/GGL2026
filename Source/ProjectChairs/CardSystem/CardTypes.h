// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CardTypes.generated.h"

UENUM(BlueprintType)
enum class EMoveSet : uint8
{
	None	UMETA(DisplayName = "None"),
	Pawn	UMETA(DisplayName = "Pawn"),
	Queen	UMETA(DisplayName = "Queen"),
	Rook	UMETA(DisplayName = "Rook"),
	Bishop	UMETA(DisplayName = "Bishop"),
	Knight	UMETA(DisplayName = "Knight"),
	King	UMETA(DisplayName = "King")
};

UENUM(BlueprintType)
enum class ETargetType : uint8
{
	Self	UMETA(DisplayName = "Self"),
	Enemy	UMETA(DisplayName = "Enemy"),
	Any		UMETA(DisplayName = "Any")
};

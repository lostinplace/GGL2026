// Copyright Epic Games, Inc. All Rights Reserved.

#include "ChessPieceEffect_ChangeMoveset.h"
#include "ChessPieceEffectComponent.h"
#include "Presentation/ChessPieceActor.h"
#include "Logic/ChessMoveRule.h"

UChessPieceEffect_ChangeMoveset::UChessPieceEffect_ChangeMoveset()
	: TargetMoveSet(EMoveSet::Queen)
	, bDiagonal(false)
	, bOrthogonal(false)
{
	BaseDuration = 2;
}

void UChessPieceEffect_ChangeMoveset::OnApply_Implementation(AChessPieceActor* TargetPiece)
{
	Super::OnApply_Implementation(TargetPiece);

	// Set flags based on target moveset
	switch (TargetMoveSet)
	{
	case EMoveSet::Queen:
		bDiagonal = true;
		bOrthogonal = true;
		break;
	case EMoveSet::Rook:
		bDiagonal = false;
		bOrthogonal = true;
		break;
	case EMoveSet::Bishop:
		bDiagonal = true;
		bOrthogonal = false;
		break;
	default:
		bDiagonal = false;
		bOrthogonal = false;
		break;
	}
	// Note: The move rule instance will be configured by RefreshMoveRule()
	// which is called by ApplyEffect after this effect is added to the stack.
	// The sliding rule flags are configured there based on this effect's settings.
}

TSubclassOf<AChessMoveRule> UChessPieceEffect_ChangeMoveset::GetMoveRuleOverride_Implementation() const
{
	// For Queen, Rook, Bishop use the sliding rule
	switch (TargetMoveSet)
	{
	case EMoveSet::Queen:
	case EMoveSet::Rook:
	case EMoveSet::Bishop:
		return AChessMoveRule_Sliding::StaticClass();

	case EMoveSet::Knight:
		return AChessMoveRule_Knight::StaticClass();

	case EMoveSet::King:
		return AChessMoveRule_King::StaticClass();

	case EMoveSet::Pawn:
		return AChessMoveRule_Pawn::StaticClass();

	case EMoveSet::None:
	default:
		return nullptr;
	}
}

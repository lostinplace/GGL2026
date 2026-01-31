// Copyright Epic Games, Inc. All Rights Reserved.

#include "ChessPieceEffect.h"
#include "ChessPieceEffectComponent.h"

UChessPieceEffect::UChessPieceEffect()
	: BaseDuration(0)
	, RemainingDuration(0)
	, OwningComponent(nullptr)
{
}

bool UChessPieceEffect::IsInstant() const
{
	return BaseDuration <= 0;
}

void UChessPieceEffect::OnApply_Implementation(AChessPieceActor* TargetPiece)
{
	// Base implementation does nothing - override in subclasses
}

void UChessPieceEffect::OnRemove_Implementation(AChessPieceActor* TargetPiece)
{
	// Base implementation does nothing - override in subclasses
}

TSubclassOf<AChessMoveRule> UChessPieceEffect::GetMoveRuleOverride_Implementation() const
{
	// Base implementation returns nullptr (no override)
	return nullptr;
}

void UChessPieceEffect::Initialize(UChessPieceEffectComponent* InOwningComponent)
{
	OwningComponent = InOwningComponent;
	RemainingDuration = BaseDuration;
}

// Copyright Epic Games, Inc. All Rights Reserved.

#include "ChessPieceEffectComponent.h"
#include "ChessPieceEffect.h"
#include "ChessPieceEffect_ChangeMoveset.h"
#include "Presentation/ChessPieceActor.h"
#include "Logic/ChessMoveRule.h"

UChessPieceEffectComponent::UChessPieceEffectComponent()
	: OriginalMoveRuleClass(nullptr)
	, CachedOwningPiece(nullptr)
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UChessPieceEffectComponent::BeginPlay()
{
	Super::BeginPlay();

	// Cache the owning piece
	CachedOwningPiece = Cast<AChessPieceActor>(GetOwner());

	// Store the original move rule class
	if (CachedOwningPiece)
	{
		OriginalMoveRuleClass = CachedOwningPiece->MoveRuleClass;
	}
}

AChessPieceActor* UChessPieceEffectComponent::GetOwningPiece() const
{
	return CachedOwningPiece;
}

UChessPieceEffect* UChessPieceEffectComponent::ApplyEffect(TSubclassOf<UChessPieceEffect> EffectClass)
{
	if (!EffectClass)
	{
		return nullptr;
	}

	// Create the effect object
	UChessPieceEffect* NewEffect = NewObject<UChessPieceEffect>(this, EffectClass);
	if (!NewEffect)
	{
		return nullptr;
	}

	// Initialize the effect
	NewEffect->Initialize(this);

	// Call OnApply
	AChessPieceActor* OwningPiece = GetOwningPiece();
	NewEffect->OnApply(OwningPiece);

	// Only add to stack if not instant
	if (!NewEffect->IsInstant())
	{
		EffectStack.Add(NewEffect);
		RefreshMoveRule();
	}

	return NewEffect;
}

void UChessPieceEffectComponent::RemoveTopEffect()
{
	if (EffectStack.Num() > 0)
	{
		RemoveEffectAtIndex(EffectStack.Num() - 1);
	}
}

void UChessPieceEffectComponent::RemoveEffect(UChessPieceEffect* Effect)
{
	if (!Effect)
	{
		return;
	}

	int32 Index = EffectStack.Find(Effect);
	if (Index != INDEX_NONE)
	{
		RemoveEffectAtIndex(Index);
	}
}

void UChessPieceEffectComponent::RemoveEffectAtIndex(int32 Index)
{
	if (!EffectStack.IsValidIndex(Index))
	{
		return;
	}

	UChessPieceEffect* Effect = EffectStack[Index];

	// Call OnRemove before removing from stack
	if (Effect)
	{
		Effect->OnRemove(GetOwningPiece());
	}

	EffectStack.RemoveAt(Index);
	RefreshMoveRule();
}

TSubclassOf<AChessMoveRule> UChessPieceEffectComponent::GetEffectiveMoveRuleClass() const
{
	// Traverse stack top-to-bottom, return first non-null override
	for (int32 i = EffectStack.Num() - 1; i >= 0; --i)
	{
		if (EffectStack[i])
		{
			TSubclassOf<AChessMoveRule> Override = EffectStack[i]->GetMoveRuleOverride();
			if (Override)
			{
				return Override;
			}
		}
	}

	// No override found, return original
	return OriginalMoveRuleClass;
}

void UChessPieceEffectComponent::OnTurnChanged(EPieceColor NewTurnColor)
{
	AChessPieceActor* OwningPiece = GetOwningPiece();
	if (!OwningPiece)
	{
		return;
	}

	// Duration decrements when turn changes AWAY from piece's color (piece's turn ended)
	// So if it's now the opponent's turn, decrement our effects
	if (NewTurnColor != OwningPiece->Color)
	{
		// Process in reverse order so we can safely remove expired effects
		for (int32 i = EffectStack.Num() - 1; i >= 0; --i)
		{
			if (EffectStack[i])
			{
				EffectStack[i]->RemainingDuration--;

				if (EffectStack[i]->RemainingDuration <= 0)
				{
					RemoveEffectAtIndex(i);
				}
			}
		}
	}
}

void UChessPieceEffectComponent::RefreshMoveRule()
{
	AChessPieceActor* OwningPiece = GetOwningPiece();
	if (!OwningPiece)
	{
		return;
	}

	TSubclassOf<AChessMoveRule> EffectiveClass = GetEffectiveMoveRuleClass();

	// Check if the class has changed
	TSubclassOf<AChessMoveRule> CurrentClass = OwningPiece->MoveRuleInstance ?
		OwningPiece->MoveRuleInstance->GetClass() : nullptr;

	if (EffectiveClass != CurrentClass)
	{
		// Destroy old instance
		if (OwningPiece->MoveRuleInstance)
		{
			OwningPiece->MoveRuleInstance->Destroy();
			OwningPiece->MoveRuleInstance = nullptr;
		}

		// Spawn new instance if we have a valid class
		if (EffectiveClass)
		{
			UWorld* World = GetWorld();
			if (World)
			{
				FActorSpawnParameters SpawnParams;
				SpawnParams.Owner = OwningPiece;
				OwningPiece->MoveRuleInstance = World->SpawnActor<AChessMoveRule>(EffectiveClass, SpawnParams);
				OwningPiece->MoveRuleClass = EffectiveClass;

				// Configure sliding rule flags if applicable
				AChessMoveRule_Sliding* SlidingRule = Cast<AChessMoveRule_Sliding>(OwningPiece->MoveRuleInstance);
				if (SlidingRule)
				{
					// Find the top ChangeMoveset effect that provides this override
					for (int32 i = EffectStack.Num() - 1; i >= 0; --i)
					{
						UChessPieceEffect_ChangeMoveset* ChangeEffect = Cast<UChessPieceEffect_ChangeMoveset>(EffectStack[i]);
						if (ChangeEffect && ChangeEffect->GetMoveRuleOverride() == EffectiveClass)
						{
							SlidingRule->bDiagonal = ChangeEffect->bDiagonal;
							SlidingRule->bOrthogonal = ChangeEffect->bOrthogonal;
							break;
						}
					}
				}
			}
		}
	}
}

// Copyright Epic Games, Inc. All Rights Reserved.

#include "CardObject.h"

UCardObject::UCardObject()
	: CardData(nullptr)
{
}

void UCardObject::InitializeFromDataAsset(UCardDataAsset* InCardData)
{
	CardData = InCardData;
}

FText UCardObject::GetDisplayName() const
{
	return CardData ? CardData->DisplayName : FText::GetEmpty();
}

EMoveSet UCardObject::GetMoveSet() const
{
	return CardData ? CardData->MoveSet : EMoveSet::None;
}

ETargetType UCardObject::GetTargetType() const
{
	return CardData ? CardData->TargetType : ETargetType::Self;
}

TSubclassOf<UCardEffect> UCardObject::GetEffectClass() const
{
	return CardData ? CardData->EffectClass : nullptr;
}

UTexture2D* UCardObject::GetIcon() const
{
	return CardData ? CardData->Icon.LoadSynchronous() : nullptr;
}

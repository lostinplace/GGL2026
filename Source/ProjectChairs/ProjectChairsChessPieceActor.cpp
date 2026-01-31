// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectChairsChessPieceActor.h"
#include "CardSystem/Effects/ChessPieceEffectComponent.h"

AProjectChairsChessPieceActor::AProjectChairsChessPieceActor()
{
	EffectComponent = CreateDefaultSubobject<UChessPieceEffectComponent>(TEXT("EffectComponent"));
}

void AProjectChairsChessPieceActor::BeginPlay()
{
	Super::BeginPlay();
}

// Copyright Epic Games, Inc. All Rights Reserved.

#include "DeckConfigurationDataAsset.h"

int32 UDeckConfigurationDataAsset::GetTotalCardCount() const
{
	int32 Total = 0;
	for (const FDeckCardEntry& Entry : Cards)
	{
		Total += Entry.Count;
	}
	return Total;
}

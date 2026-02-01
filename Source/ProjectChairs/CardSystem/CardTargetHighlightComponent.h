// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CardTargetHighlightComponent.generated.h"

/**
 * Component that manages card targeting highlight state for an actor.
 * Attach to chess pieces to enable visual feedback when they are valid card targets.
 *
 * Uses Custom Depth rendering for post-process outline effects.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class PROJECTCHAIRS_API UCardTargetHighlightComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCardTargetHighlightComponent();

	/** Set whether this actor is a valid card target */
	UFUNCTION(BlueprintCallable, Category = "Card Targeting")
	void SetHighlighted(bool bNewHighlighted);

	/** Get whether this actor is currently highlighted as a valid card target */
	UFUNCTION(BlueprintCallable, Category = "Card Targeting")
	bool IsHighlighted() const { return bIsHighlighted; }

	/** Called when the highlight state changes (Blueprint event) */
	UFUNCTION(BlueprintImplementableEvent, Category = "Card Targeting")
	void OnHighlightChanged(bool bHighlighted);

	/** Native event for highlight changes (can be overridden in C++) */
	UFUNCTION(BlueprintNativeEvent, Category = "Card Targeting")
	void HandleHighlightChanged(bool bHighlighted);

	/** If true, automatically enables Custom Depth rendering on mesh components when highlighted */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card Targeting|Outline")
	bool bUseCustomDepthOutline;

	/** The stencil value to use when highlighted (for post-process material) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card Targeting|Outline", meta = (ClampMin = "1", ClampMax = "255"))
	int32 HighlightStencilValue;

protected:
	virtual void BeginPlay() override;

	/** Whether this actor is currently highlighted as a valid card target */
	UPROPERTY(BlueprintReadOnly, Category = "Card Targeting")
	bool bIsHighlighted;

	/** Enables or disables custom depth rendering on all mesh components */
	void SetCustomDepthEnabled(bool bEnabled);

	/** Cached mesh components for performance */
	UPROPERTY()
	TArray<UPrimitiveComponent*> CachedMeshComponents;

	/** Cache the mesh components on the owning actor */
	void CacheMeshComponents();

public:
	/** Find the CardTargetHighlightComponent on an actor, if it exists */
	UFUNCTION(BlueprintCallable, Category = "Card Targeting", meta = (DefaultToSelf = "Actor"))
	static UCardTargetHighlightComponent* FindHighlightComponent(AActor* Actor);
};

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SelectableChessPieceComponent.generated.h"

// Event for when this component is selected/deselected
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSelectionStateChanged, bool, bSelected);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CHESSGAME_API USelectableChessPieceComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	USelectableChessPieceComponent();

protected:
	virtual void BeginPlay() override;

public:	
	// API to Select/Deselect
	UFUNCTION(BlueprintCallable, Category = "Selection")
	void SetSelected(bool bNewSelected);

	UPROPERTY(BlueprintAssignable, Category = "Selection")
	FOnSelectionStateChanged OnSelectionStateChanged;

	// Current State
	UPROPERTY(BlueprintReadOnly, Category = "Selection")
	bool bIsSelected = false;

	// Helper to get from Actor
	static USelectableChessPieceComponent* FindSelectableComponent(AActor* Actor);
};

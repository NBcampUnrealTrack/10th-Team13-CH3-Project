#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "VeilBreakGameState.generated.h"

UCLASS()
class VEILBREAK_API AVeilBreakGameState : public AGameState
{
	GENERATED_BODY()
	
public:
	AVeilBreakGameState();

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Score")
	int32 Score;
	UFUNCTION(BlueprintPure, Category = "Score")
	int32 GetScore() const;
	UFUNCTION(BlueprintCallable, Category = "Score")
	void AddScore(int32 Amount);
};
	
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "VeilbreakGameState.generated.h"

UCLASS()
class VEILBREAK_API AVeilbreakGameState : public AGameState
{
	GENERATED_BODY()
	
public:
	AVeilbreakGameState();

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Score")
	int32 Score;
	UFUNCTION(BlueprintPure, Category = "Score")
	int32 GetScore() const;
	UFUNCTION(BlueprintCallable, Category = "Score")
	void AddScore(int32 Amount);
};
	
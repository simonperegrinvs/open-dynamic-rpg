#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "OdrController.generated.h"

UCLASS()
class AOdrController : public APlayerController {
    GENERATED_BODY()

  protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;

  private:
    void Step(int32 Q, int32 R);
    void NorthEast();
    void East();
    void SouthEast();
    void SouthWest();
    void West();
    void NorthWest();
    void Interact();
    void First();
    void Second();
    void Third();
    void Fourth();
    void Fifth();
    void Sixth();
    void CreateHero();
    void Recruit();
    void Authored();
    void Generated();
    void SearchOrCast();
    void BackgroundOrDefend();
    void Attack();
    void CycleTarget();
    void CycleHealingTarget();
    void AreaSpell();
    void RetreatOrRest();
    void Upgrade();
    void InspectOrLevel();
    void Boss();
    void ActivateAll();
    void Save();
    void Load();
};

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "OdrController.generated.h"

UCLASS()
class AOdrController : public APlayerController {
    GENERATED_BODY()

  protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void SetupInputComponent() override;
    virtual void PlayerTick(float DeltaTime) override;
    virtual bool InputKey(const FInputKeyEventArgs& Params) override;

  private:
    friend class SOdrPointerSurface;
    TSharedPtr<class SWidget> PointerSurface;
    FVector2D PointerPosition = FVector2D::ZeroVector;
    bool bHasPointerPosition = false;
    void PointerMoved(FVector2D Position);
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
    void MouseClick();
    void ZoomIn();
    void ZoomOut();
    void Focus();
    void Cancel();
    void BeginPan();
    void EndPan();
    void BeginOrbit();
    void EndOrbit();
    bool bPanning = false;
    bool bOrbiting = false;
    FVector2D PreviousMouse = FVector2D::ZeroVector;
};

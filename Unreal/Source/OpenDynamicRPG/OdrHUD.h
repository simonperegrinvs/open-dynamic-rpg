#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "OdrHUD.generated.h"

UCLASS()
class AOdrHUD : public AHUD {
    GENERATED_BODY()

  public:
    virtual void DrawHUD() override;
};

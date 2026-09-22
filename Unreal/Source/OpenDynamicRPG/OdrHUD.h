#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "OdrHUD.generated.h"

class AOdrGameMode;
class FJsonObject;

UCLASS()
class AOdrHUD : public AHUD {
    GENERATED_BODY()
  public:
    virtual void DrawHUD() override;
    bool HandleClick(const FVector2D& ScreenPosition);
    void UpdateHover(const FVector2D& ScreenPosition) {
        Mouse = ScreenPosition;
    }

  private:
    struct FButton {
        FBox2D Bounds;
        FString Action;
        FString Hint;
        bool bEnabled;
    };
    void Rect(float X, float Y, float W, float H, FLinearColor Color);
    void Text(const FString& Value, float X, float Y, float Size = 17,
              FLinearColor Color = FLinearColor::White);
    float TextWidth(const FString& Value, float Size) const;
    float Wrapped(const FString& Value, float X, float Y, float W, float Size, FLinearColor Color);
    void Panel(float X, float Y, float W, float H);
    void Button(const FString& Action, const FString& Label, const FString& Hint, float X, float Y,
                float W, bool bEnabled = true, bool bSelected = false, float H = 38);
    void DrawWorld(AOdrGameMode* Mode);
    void DrawParty(AOdrGameMode* Mode, const TSharedPtr<FJsonObject>& State);
    void HexOutline(FIntPoint Hex, FLinearColor Color, float Thickness = 2);
    void Execute(const FString& Action, AOdrGameMode* Mode);
    bool OverPanel(FVector2D Position) const;
    TArray<FButton> Buttons;
    TArray<FBox2D> Panels;
    TArray<TPair<FBox2D, FString>> ActorHits;
    FVector2D Mouse = FVector2D(-1, -1);
    float Scale = 1, Width = 1440, Height = 900;
    FIntPoint HoverHex = FIntPoint(MAX_int32, MAX_int32);
    TArray<FIntPoint> HoverPath;
    TSharedPtr<FJsonObject> HoverState;
};

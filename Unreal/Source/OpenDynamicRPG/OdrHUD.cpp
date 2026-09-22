#include "OdrHUD.h"

#include "Engine/Canvas.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "OdrGameMode.h"

void AOdrHUD::DrawHUD() {
    Super::DrawHUD();
    const AOdrGameMode* Mode = GetWorld()->GetAuthGameMode<AOdrGameMode>();
    if (Mode == nullptr) {
        return;
    }
    DrawText(TEXT("OPEN DYNAMIC RPG | PLACEHOLDER BUILD"), FLinearColor::Yellow, 24.0f, 24.0f);
    DrawText(Mode->Status(), FLinearColor::White, 24.0f, 60.0f);
    const FString Playback = Mode->Presentation();
    if (!Playback.IsEmpty()) {
        DrawText(Playback, FLinearColor::Yellow,
                 Canvas->SizeX * 0.5f - 7.5f * Playback.Len(), 130.0f, nullptr, 1.5f);
    }
    if (PlayerOwner != nullptr) {
        for (const FOdrSceneLabel& Label : Mode->Labels()) {
            FVector2D Screen;
            if (PlayerOwner->ProjectWorldLocationToScreen(Label.Position, Screen)) {
                if (Label.Text == TEXT("RETREAT")) {
                    Screen.Y += 18.0f;
                } else if (Label.Text == TEXT("Party")) {
                    Screen.Y -= 18.0f;
                }
                float TextWidth = 0.0f;
                float TextHeight = 0.0f;
                GetTextSize(Label.Text, TextWidth, TextHeight, nullptr, 1.1f);
                DrawText(Label.Text, Label.Color, Screen.X - TextWidth * 0.5f,
                         Screen.Y - TextHeight * 0.5f, nullptr, 1.1f);
            }
        }
    }
    DrawText(Mode->Help(), FLinearColor::Green, 24.0f, Canvas->SizeY - 52.0f);
    DrawText(TEXT("F5 save | F9 load"), FLinearColor::White, 24.0f, Canvas->SizeY - 28.0f);
}

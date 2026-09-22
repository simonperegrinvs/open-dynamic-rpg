#include "OdrController.h"

#include "Camera/CameraActor.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "InputKeyEventArgs.h"
#include "OdrGameMode.h"
#include "OdrHUD.h"
#include "Slate/SceneViewport.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SLeafWidget.h"

namespace {
AOdrGameMode* GameMode(const APlayerController* Controller) {
    return Controller->GetWorld()->GetAuthGameMode<AOdrGameMode>();
}
} // namespace

// Keep the actual Slate pointer-event coordinates. Polling the viewport cursor
// can return its old capture position on Mac when focus changes during a click.
class SOdrPointerSurface : public SLeafWidget {
  public:
    SLATE_BEGIN_ARGS(SOdrPointerSurface) {}
    SLATE_ARGUMENT(AOdrController*, Controller)
    SLATE_END_ARGS()
    void Construct(const FArguments& Arguments) {
        Owner = Arguments._Controller;
    }
    virtual FVector2D ComputeDesiredSize(float) const override {
        return FVector2D(1, 1);
    }
    virtual int32 OnPaint(const FPaintArgs&, const FGeometry&, const FSlateRect&,
                          FSlateWindowElementList&, int32 Layer, const FWidgetStyle&,
                          bool) const override {
        return Layer;
    }
    void Position(const FGeometry& Geometry, const FPointerEvent& Event) const {
        if (!Owner.IsValid())
            return;
        FVector2D Point = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
        int32 W = 0, H = 0;
        Owner->GetViewportSize(W, H);
        const auto Size = Geometry.GetLocalSize();
        if (Size.X > 0 && Size.Y > 0)
            Owner->PointerMoved(FVector2D(Point.X * W / Size.X, Point.Y * H / Size.Y));
    }
    virtual FReply OnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event) override {
        Position(Geometry, Event);
        return FReply::Handled();
    }
    virtual FReply OnMouseButtonDown(const FGeometry& Geometry,
                                     const FPointerEvent& Event) override {
        Position(Geometry, Event);
        if (Owner.IsValid()) {
            Owner->PreviousMouse = Owner->PointerPosition;
            if (Event.GetEffectingButton() == EKeys::MiddleMouseButton)
                Owner->BeginPan();
            if (Event.GetEffectingButton() == EKeys::RightMouseButton)
                Owner->BeginOrbit();
        }
        return FReply::Handled().CaptureMouse(SharedThis(this));
    }
    virtual FReply OnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event) override {
        Position(Geometry, Event);
        if (Owner.IsValid()) {
            if (Event.GetEffectingButton() == EKeys::LeftMouseButton)
                Owner->MouseClick();
            if (Event.GetEffectingButton() == EKeys::MiddleMouseButton)
                Owner->EndPan();
            if (Event.GetEffectingButton() == EKeys::RightMouseButton)
                Owner->EndOrbit();
        }
        return FReply::Handled().ReleaseMouseCapture();
    }
    virtual FReply OnMouseWheel(const FGeometry& Geometry, const FPointerEvent& Event) override {
        Position(Geometry, Event);
        if (Owner.IsValid())
            if (auto* Mode = GameMode(Owner.Get()))
                Mode->AdjustZoom(Event.GetWheelDelta() * -160);
        return FReply::Handled();
    }
    virtual void OnMouseCaptureLost(const FCaptureLostEvent& Event) override {
        if (Owner.IsValid()) {
            Owner->EndPan();
            Owner->EndOrbit();
        }
        SLeafWidget::OnMouseCaptureLost(Event);
    }

  private:
    TWeakObjectPtr<AOdrController> Owner;
};

void AOdrController::BeginPlay() {
    Super::BeginPlay();
    bShowMouseCursor = true;
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;
    FInputModeGameAndUI InputMode;
    InputMode.SetHideCursorDuringCapture(false);
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(InputMode);
    if (GetWorld()->GetGameViewport()) {
        GetWorld()->GetGameViewport()->SetMouseCaptureMode(EMouseCaptureMode::NoCapture);
        GetWorld()->GetGameViewport()->SetMouseLockMode(EMouseLockMode::DoNotLock);
        PointerSurface = SNew(SOdrPointerSurface).Controller(this);
        GetWorld()->GetGameViewport()->AddViewportWidgetContent(PointerSurface.ToSharedRef(), 10);
    }
    if (AOdrGameMode* Mode = GameMode(this)) {
        if (ACameraActor* Camera = Mode->Camera()) {
            SetViewTarget(Camera);
        }
    }
}

void AOdrController::EndPlay(const EEndPlayReason::Type Reason) {
    EndPan();
    EndOrbit();
    if (PointerSurface.IsValid() && GetWorld()->GetGameViewport())
        GetWorld()->GetGameViewport()->RemoveViewportWidgetContent(PointerSurface.ToSharedRef());
    PointerSurface.Reset();
    Super::EndPlay(Reason);
}

void AOdrController::PointerMoved(FVector2D Position) {
    PointerPosition = Position;
    bHasPointerPosition = true;
    if (auto* Hud = Cast<AOdrHUD>(GetHUD()))
        Hud->UpdateHover(Position);
}

void AOdrController::SetupInputComponent() {
    Super::SetupInputComponent();
    InputComponent->BindKey(EKeys::Q, IE_Pressed, this, &AOdrController::NorthWest);
    InputComponent->BindKey(EKeys::W, IE_Pressed, this, &AOdrController::NorthEast);
    InputComponent->BindKey(EKeys::E, IE_Pressed, this, &AOdrController::East);
    InputComponent->BindKey(EKeys::A, IE_Pressed, this, &AOdrController::West);
    InputComponent->BindKey(EKeys::S, IE_Pressed, this, &AOdrController::SouthWest);
    InputComponent->BindKey(EKeys::D, IE_Pressed, this, &AOdrController::SouthEast);
    InputComponent->BindKey(EKeys::X, IE_Pressed, this, &AOdrController::Interact);
    InputComponent->BindKey(EKeys::One, IE_Pressed, this, &AOdrController::First);
    InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &AOdrController::Second);
    InputComponent->BindKey(EKeys::Three, IE_Pressed, this, &AOdrController::Third);
    InputComponent->BindKey(EKeys::Four, IE_Pressed, this, &AOdrController::Fourth);
    InputComponent->BindKey(EKeys::Five, IE_Pressed, this, &AOdrController::Fifth);
    InputComponent->BindKey(EKeys::Six, IE_Pressed, this, &AOdrController::Sixth);
    InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &AOdrController::CreateHero);
    InputComponent->BindKey(EKeys::N, IE_Pressed, this, &AOdrController::Recruit);
    InputComponent->BindKey(EKeys::M, IE_Pressed, this, &AOdrController::Authored);
    InputComponent->BindKey(EKeys::G, IE_Pressed, this, &AOdrController::Generated);
    InputComponent->BindKey(EKeys::C, IE_Pressed, this, &AOdrController::SearchOrCast);
    InputComponent->BindKey(EKeys::B, IE_Pressed, this, &AOdrController::BackgroundOrDefend);
    InputComponent->BindKey(EKeys::F, IE_Pressed, this, &AOdrController::Attack);
    InputComponent->BindKey(EKeys::T, IE_Pressed, this, &AOdrController::CycleTarget);
    InputComponent->BindKey(EKeys::Y, IE_Pressed, this, &AOdrController::CycleHealingTarget);
    InputComponent->BindKey(EKeys::Z, IE_Pressed, this, &AOdrController::AreaSpell);
    InputComponent->BindKey(EKeys::R, IE_Pressed, this, &AOdrController::RetreatOrRest);
    InputComponent->BindKey(EKeys::U, IE_Pressed, this, &AOdrController::Upgrade);
    InputComponent->BindKey(EKeys::I, IE_Pressed, this, &AOdrController::InspectOrLevel);
    InputComponent->BindKey(EKeys::L, IE_Pressed, this, &AOdrController::InspectOrLevel);
    InputComponent->BindKey(EKeys::V, IE_Pressed, this, &AOdrController::Boss);
    InputComponent->BindKey(EKeys::P, IE_Pressed, this, &AOdrController::ActivateAll);
    InputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &AOdrController::BackgroundOrDefend);
    InputComponent->BindKey(EKeys::F5, IE_Pressed, this, &AOdrController::Save);
    InputComponent->BindKey(EKeys::F9, IE_Pressed, this, &AOdrController::Load);
    InputComponent->BindKey(EKeys::MouseScrollUp, IE_Pressed, this, &AOdrController::ZoomIn);
    InputComponent->BindKey(EKeys::MouseScrollDown, IE_Pressed, this, &AOdrController::ZoomOut);
    InputComponent->BindKey(EKeys::Home, IE_Pressed, this, &AOdrController::Focus);
    InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AOdrController::Cancel);
    InputComponent->BindKey(EKeys::MiddleMouseButton, IE_Pressed, this, &AOdrController::BeginPan);
    InputComponent->BindKey(EKeys::MiddleMouseButton, IE_Released, this, &AOdrController::EndPan);
    InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &AOdrController::BeginOrbit);
    InputComponent->BindKey(EKeys::RightMouseButton, IE_Released, this, &AOdrController::EndOrbit);
}

bool AOdrController::InputKey(const FInputKeyEventArgs& Params) {
    const bool bHandled = Super::InputKey(Params);
    if (Params.Key == EKeys::LeftMouseButton && Params.Event == IE_Released) {
        // Resolve against the viewport position from this event, before the next
        // input tick can replace it with a focus/capture cursor position.
        MouseClick();
        return true;
    }
    return bHandled;
}

void AOdrController::PlayerTick(float DeltaTime) {
    Super::PlayerTick(DeltaTime);
    float X = 0.0f, Y = 0.0f;
    const bool bPositionValid = bHasPointerPosition || GetMousePosition(X, Y);
    if (bHasPointerPosition) {
        X = PointerPosition.X;
        Y = PointerPosition.Y;
    }
    if (bPositionValid) {
        if (AOdrHUD* Hud = Cast<AOdrHUD>(GetHUD()))
            Hud->UpdateHover(FVector2D(X, Y));
        auto* Mode = GameMode(this);
        auto* Camera = Mode ? Mode->Camera() : nullptr;
        if (Camera && bPanning) {
            FVector Before, BeforeDirection, After, AfterDirection;
            if (DeprojectScreenPositionToWorld(PreviousMouse.X, PreviousMouse.Y, Before,
                                               BeforeDirection) &&
                DeprojectScreenPositionToWorld(X, Y, After, AfterDirection) &&
                BeforeDirection.Z < -0.01 && AfterDirection.Z < -0.01) {
                Before -= BeforeDirection * Before.Z / BeforeDirection.Z;
                After -= AfterDirection * After.Z / AfterDirection.Z;
                Camera->AddActorWorldOffset(Before - After);
            }
        }
        if (Camera && bOrbiting) {
            const FVector Direction = Camera->GetActorForwardVector();
            const FVector Location = Camera->GetActorLocation();
            if (Direction.Z < -0.01) {
                const FVector Pivot = Location - Direction * Location.Z / Direction.Z;
                const FVector Offset =
                    (Location - Pivot)
                        .RotateAngleAxis((X - PreviousMouse.X) * 0.25f, FVector::UpVector);
                Camera->SetActorLocation(Pivot + Offset);
                Camera->SetActorRotation((-Offset).Rotation());
            }
        }
        PreviousMouse = FVector2D(X, Y);
    }
}
void AOdrController::BeginPan() {
    bPanning = true;
}
void AOdrController::EndPan() {
    bPanning = false;
}
void AOdrController::BeginOrbit() {
    bOrbiting = true;
}
void AOdrController::EndOrbit() {
    bOrbiting = false;
}

void AOdrController::MouseClick() {
    float X = 0.0f, Y = 0.0f;
    if (bHasPointerPosition) {
        X = PointerPosition.X;
        Y = PointerPosition.Y;
    } else if (!GetMousePosition(X, Y))
        return;
    AOdrHUD* Hud = Cast<AOdrHUD>(GetHUD());
    if (Hud != nullptr && Hud->HandleClick(FVector2D(X, Y)))
        return;
    AOdrGameMode* Mode = GameMode(this);
    if (Mode == nullptr || (Mode->Phase() != TEXT("overworld") &&
                            Mode->Phase() != TEXT("dungeon") && Mode->Phase() != TEXT("battle")))
        return;
    FVector WorldOrigin, WorldDirection;
    if (DeprojectScreenPositionToWorld(X, Y, WorldOrigin, WorldDirection) &&
        !FMath::IsNearlyZero(WorldDirection.Z)) {
        const float Distance = -WorldOrigin.Z / WorldDirection.Z;
        if (Distance >= 0.0f)
            Mode->ClickHex(AOdrGameMode::WorldToHex(WorldOrigin + WorldDirection * Distance));
    }
}
void AOdrController::ZoomIn() {
    if (AOdrGameMode* Mode = GameMode(this))
        Mode->AdjustZoom(-160.0f);
}
void AOdrController::ZoomOut() {
    if (AOdrGameMode* Mode = GameMode(this))
        Mode->AdjustZoom(160.0f);
}
void AOdrController::Focus() {
    if (AOdrGameMode* Mode = GameMode(this))
        Mode->FocusCamera();
}
void AOdrController::Cancel() {
    EndPan();
    EndOrbit();
    if (AOdrGameMode* Mode = GameMode(this)) {
        Mode->CancelNavigation();
        Mode->SelectActor(FString());
    }
}

void AOdrController::Step(int32 Q, int32 R) {
    if (AOdrGameMode* Mode = GameMode(this)) {
        const FString Phase = Mode->Phase();
        if (Phase == TEXT("overworld") || Phase == TEXT("dungeon") || Phase == TEXT("battle")) {
            // The host computes the next absolute hex from the current snapshot.
            Mode->Step(Q, R);
        }
    }
}

void AOdrController::NorthEast() {
    Step(1, -1);
}
void AOdrController::East() {
    Step(1, 0);
}
void AOdrController::SouthEast() {
    Step(0, 1);
}
void AOdrController::SouthWest() {
    Step(-1, 1);
}
void AOdrController::West() {
    Step(-1, 0);
}
void AOdrController::NorthWest() {
    Step(0, -1);
}

void AOdrController::Interact() {
    if (AOdrGameMode* Mode = GameMode(this)) {
        if (Mode->Phase() == TEXT("city")) {
            Mode->Command(TEXT("{\"action\":\"leave_city\"}"));
        } else if (Mode->Phase() == TEXT("overworld")) {
            if (!Mode->Command(TEXT("{\"action\":\"enter_mine\"}"))) {
                Mode->Command(TEXT("{\"action\":\"enter_city\"}"));
            }
        } else if (Mode->Phase() == TEXT("dungeon")) {
            Mode->Command(TEXT("{\"action\":\"leave_mine\"}"));
        }
    }
}

void AOdrController::First() {
    if (AOdrGameMode* Mode = GameMode(this)) {
        if (Mode->Phase() == TEXT("creation"))
            Mode->ChooseClass(0);
        else if (Mode->Phase() == TEXT("dungeon"))
            Mode->Command(TEXT("{\"action\":\"search\"}"));
    }
}
void AOdrController::Second() {
    if (AOdrGameMode* Mode = GameMode(this)) {
        if (Mode->Phase() == TEXT("creation"))
            Mode->ChooseClass(1);
        else if (Mode->Phase() == TEXT("dungeon"))
            Mode->Command(TEXT("{\"action\":\"take_hidden_loot\"}"));
    }
}
void AOdrController::Third() {
    if (AOdrGameMode* Mode = GameMode(this)) {
        if (Mode->Phase() == TEXT("creation"))
            Mode->ChooseClass(2);
        else if (Mode->Phase() == TEXT("dungeon"))
            Mode->Command(TEXT("{\"action\":\"begin_main\"}"));
    }
}
void AOdrController::Fourth() {
    if (AOdrGameMode* Mode = GameMode(this)) {
        if (Mode->Phase() == TEXT("creation"))
            Mode->ChooseClass(3);
        else if (Mode->Phase() == TEXT("dungeon"))
            Mode->Command(TEXT("{\"action\":\"begin_secret\"}"));
    }
}
void AOdrController::Fifth() {
    if (AOdrGameMode* Mode = GameMode(this)) {
        if (Mode->Phase() == TEXT("creation"))
            Mode->ChooseClass(4);
        else if (Mode->Phase() == TEXT("dungeon"))
            Mode->Command(TEXT("{\"action\":\"take_ore\"}"));
    }
}
void AOdrController::Sixth() {
    if (AOdrGameMode* Mode = GameMode(this)) {
        if (Mode->Phase() == TEXT("creation"))
            Mode->ChooseClass(5);
        else if (Mode->Phase() == TEXT("dungeon"))
            Mode->ChangeFloor();
    }
}

void AOdrController::CreateHero() {
    if (AOdrGameMode* Mode = GameMode(this)) {
        Mode->CreateSelectedHero();
    }
}
void AOdrController::Recruit() {
    if (AOdrGameMode* Mode = GameMode(this))
        Mode->RecruitNext();
}
void AOdrController::Authored() {
    if (AOdrGameMode* Mode = GameMode(this))
        Mode->Command(TEXT("{\"action\":\"create_run\"}"));
}
void AOdrController::Generated() {
    if (AOdrGameMode* Mode = GameMode(this))
        Mode->CreateGeneratedRun();
}
void AOdrController::SearchOrCast() {
    if (AOdrGameMode* Mode = GameMode(this)) {
        if (Mode->Phase() == TEXT("creation"))
            Mode->CycleAncestry();
        else if (Mode->Phase() == TEXT("battle"))
            Mode->ActOnSelected(true);
    }
}
void AOdrController::BackgroundOrDefend() {
    if (AOdrGameMode* Mode = GameMode(this)) {
        if (Mode->Phase() == TEXT("creation"))
            Mode->CycleBackground();
        else if (Mode->Phase() == TEXT("battle"))
            Mode->Command(TEXT("{\"action\":\"defend\"}"));
    }
}
void AOdrController::Attack() {
    if (AOdrGameMode* Mode = GameMode(this))
        Mode->ActOnSelected(false);
}
void AOdrController::CycleTarget() {
    if (AOdrGameMode* Mode = GameMode(this))
        Mode->CycleTarget();
}
void AOdrController::CycleHealingTarget() {
    if (AOdrGameMode* Mode = GameMode(this))
        Mode->CycleHealingTarget();
}
void AOdrController::AreaSpell() {
    if (AOdrGameMode* Mode = GameMode(this))
        Mode->ActOnNearest(true, true);
}
void AOdrController::RetreatOrRest() {
    if (AOdrGameMode* Mode = GameMode(this)) {
        const FString Phase = Mode->Phase();
        if (Phase == TEXT("battle"))
            Mode->Command(TEXT("{\"action\":\"retreat\"}"));
        else if (Phase == TEXT("city"))
            Mode->Command(TEXT("{\"action\":\"rest\"}"));
        else if (Phase == TEXT("dungeon"))
            Mode->Command(TEXT("{\"action\":\"camp\"}"));
    }
}
void AOdrController::Upgrade() {
    if (AOdrGameMode* Mode = GameMode(this)) {
        if (Mode->Phase() == TEXT("city"))
            Mode->Command(TEXT("{\"action\":\"upgrade\"}"));
    }
}
void AOdrController::InspectOrLevel() {
    if (AOdrGameMode* Mode = GameMode(this)) {
        if (Mode->Phase() == TEXT("overworld"))
            Mode->Command(TEXT("{\"action\":\"inspect_ruins\"}"));
        else if (Mode->Phase() == TEXT("city"))
            Mode->LevelHero();
    }
}
void AOdrController::Boss() {
    if (AOdrGameMode* Mode = GameMode(this)) {
        if (Mode->Phase() == TEXT("city"))
            Mode->CreateBossRun();
    }
}
void AOdrController::ActivateAll() {
    if (AOdrGameMode* Mode = GameMode(this)) {
        if (Mode->Phase() == TEXT("city"))
            Mode->SetAllActive();
    }
}
void AOdrController::Save() {
    if (AOdrGameMode* Mode = GameMode(this))
        Mode->SaveSession();
}
void AOdrController::Load() {
    if (AOdrGameMode* Mode = GameMode(this))
        Mode->LoadSession();
}

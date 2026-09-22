#include "OdrController.h"

#include "Camera/CameraActor.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "OdrGameMode.h"

namespace {
AOdrGameMode* GameMode(const APlayerController* Controller) {
    return Controller->GetWorld()->GetAuthGameMode<AOdrGameMode>();
}
} // namespace

void AOdrController::BeginPlay() {
    Super::BeginPlay();
    if (AOdrGameMode* Mode = GameMode(this)) {
        if (ACameraActor* Camera = Mode->Camera()) {
            SetViewTarget(Camera);
        }
    }
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

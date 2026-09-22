#include "OdrGameMode.h"

#include "OdrController.h"
#include "OdrHUD.h"
#include "odr/session.h"

#include "Animation/AnimInstance.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/MeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace {
TSharedPtr<FJsonObject> ParseObject(const FString& Text) {
    TSharedPtr<FJsonObject> Object;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Text);
    if (!FJsonSerializer::Deserialize(Reader, Object)) {
        return nullptr;
    }
    return Object;
}

TSharedPtr<FJsonObject> ObjectField(const TSharedPtr<FJsonObject>& Object, const FString& Name) {
    if (!Object.IsValid()) {
        return nullptr;
    }
    const TSharedPtr<FJsonObject>* Value = nullptr;
    return Object->TryGetObjectField(Name, Value) && Value != nullptr ? *Value : nullptr;
}

FString StringField(const TSharedPtr<FJsonObject>& Object, const FString& Name) {
    FString Value;
    if (Object.IsValid()) {
        Object->TryGetStringField(Name, Value);
    }
    return Value;
}

int32 NumberField(const TSharedPtr<FJsonObject>& Object, const FString& Name) {
    double Value = 0.0;
    if (Object.IsValid()) {
        Object->TryGetNumberField(Name, Value);
    }
    return static_cast<int32>(Value);
}

FString ArrayString(const TArray<TSharedPtr<FJsonValue>>& Values, int32 Index) {
    return Values.IsValidIndex(Index) && Values[Index].IsValid() ? Values[Index]->AsString()
                                                                 : FString();
}

FIntPoint Position(const TArray<TSharedPtr<FJsonValue>>& Values) {
    if (Values.Num() < 2) {
        return FIntPoint::ZeroValue;
    }
    return {static_cast<int32>(Values[0]->AsNumber()), static_cast<int32>(Values[1]->AsNumber())};
}

FIntPoint PositionField(const TSharedPtr<FJsonObject>& Object, const FString& Name) {
    const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
    return Object.IsValid() && Object->TryGetArrayField(Name, Values) && Values != nullptr
               ? Position(*Values)
               : FIntPoint::ZeroValue;
}

int32 HexDistance(const FIntPoint& A, const FIntPoint& B) {
    const int32 DQ = A.X - B.X;
    const int32 DR = A.Y - B.Y;
    return (FMath::Abs(DQ) + FMath::Abs(DR) + FMath::Abs(DQ + DR)) / 2;
}

FString DataPath(const FString& File) {
    return FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data"), File);
}

FString CommandWithPosition(const FString& Action, int32 Q, int32 R) {
    return FString::Printf(TEXT("{\"action\":\"%s\",\"q\":%d,\"r\":%d}"), *Action, Q, R);
}

const TCHAR* Classes[] = {TEXT("Warrior"), TEXT("Rogue"),  TEXT("Ranger"),
                          TEXT("Mage"),    TEXT("Cleric"), TEXT("Barbarian")};
const TCHAR* Ancestries[] = {TEXT("Human"), TEXT("Elf"), TEXT("Dwarf"), TEXT("Orc")};
const TCHAR* Backgrounds[] = {TEXT("Guard"), TEXT("Scholar"), TEXT("Outlaw")};
const TCHAR* Recruits[] = {TEXT("rowan"), TEXT("sable"), TEXT("tala"), TEXT("iona"),
                           TEXT("bran"),  TEXT("kora"),  TEXT("eden"), TEXT("fenn"),
                           TEXT("mira"),  TEXT("orin"),  TEXT("pax")};
} // namespace

AOdrGameMode::AOdrGameMode() {
    PlayerControllerClass = AOdrController::StaticClass();
    HUDClass = AOdrHUD::StaticClass();
    DefaultPawnClass = nullptr;
}

void AOdrGameMode::BeginPlay() {
    Super::BeginPlay();
    FString Definition;
    if (!FFileHelper::LoadFileToString(Definition, *DataPath(TEXT("mine.json")))) {
        LastMessage = TEXT("Missing staged mine.json. Run tools/stage_content.py.");
        UE_LOG(LogTemp, Error, TEXT("%s"), *LastMessage);
        return;
    }
    Session = odr_create(TCHAR_TO_UTF8(*Definition));
    if (Session == nullptr) {
        LastMessage = TEXT("Adventure definition failed to validate.");
        UE_LOG(LogTemp, Error, TEXT("%s"), *LastMessage);
        return;
    }
    FString VisualText;
    if (FFileHelper::LoadFileToString(VisualText, *DataPath(TEXT("visuals.json")))) {
        Visuals = ParseObject(VisualText);
    }
    SceneCamera =
        GetWorld()->SpawnActor<ACameraActor>(FVector(0.0, 0.0, 2800.0), FRotator(-90.0, 90.0, 0.0));
    if (SceneCamera != nullptr) {
        SceneCamera->GetCameraComponent()->SetProjectionMode(ECameraProjectionMode::Orthographic);
        SceneCamera->GetCameraComponent()->SetConstraintAspectRatio(false);
        SceneCamera->GetCameraComponent()->SetOrthoWidth(3600.0f);
        if (APlayerController* Controller = UGameplayStatics::GetPlayerController(GetWorld(), 0)) {
            Controller->SetViewTarget(SceneCamera);
        }
    }
    if (auto* Key =
            GetWorld()->SpawnActor<ADirectionalLight>(FVector(0, 0, 1500), FRotator(-52, -35, 0))) {
        Key->GetLightComponent()->SetIntensity(5.0f);
        Key->GetLightComponent()->SetLightColor(FLinearColor(1.0f, 0.85f, 0.65f));
        Cast<UDirectionalLightComponent>(Key->GetLightComponent())->SetForwardShadingPriority(1);
    }
    if (auto* Fill =
            GetWorld()->SpawnActor<ADirectionalLight>(FVector(0, 0, 1500), FRotator(-40, 145, 0))) {
        Fill->GetLightComponent()->SetIntensity(2.2f);
        Fill->GetLightComponent()->SetLightColor(FLinearColor(0.52f, 0.72f, 1.0f));
        Fill->GetLightComponent()->SetCastShadows(false);
    }
    if (auto* Post = GetWorld()->SpawnActor<APostProcessVolume>()) {
        Post->bUnbound = true;
        auto& Settings = Post->Settings;
        Settings.bOverride_AutoExposureMethod = true;
        Settings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
        Settings.bOverride_AutoExposureBias = true;
        Settings.AutoExposureBias = -0.6f;
        Settings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
        Settings.AutoExposureApplyPhysicalCameraExposure = false;
        Settings.bOverride_BloomIntensity = true;
        Settings.BloomIntensity = 0.25f;
        Settings.bOverride_VignetteIntensity = true;
        Settings.VignetteIntensity = 0.25f;
        Settings.bOverride_AmbientOcclusionIntensity = true;
        Settings.AmbientOcclusionIntensity = 0.7f;
    }
    Refresh();
    if (FParse::Param(FCommandLine::Get(), TEXT("odrpreviewbattle"))) {
        FString Preview;
        if (FFileHelper::LoadFileToString(Preview, *DataPath(TEXT("blacksmith_mine.jsonl")))) {
            TArray<FString> Lines;
            Preview.ParseIntoArrayLines(Lines, true);
            for (const FString& Line : Lines) {
                if (Line.IsEmpty() || Line.StartsWith(TEXT("#")))
                    continue;
                if (!Command(Line) || Phase() == TEXT("battle"))
                    break;
            }
        }
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("odrsmoke"))) {
        OdrSmoke();
    }
}

void AOdrGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    CancelNavigation();
    odr_destroy(Session);
    Session = nullptr;
    Super::EndPlay(EndPlayReason);
}

TSharedPtr<FJsonObject> AOdrGameMode::Snapshot() const {
    return Session == nullptr ? nullptr : ParseObject(UTF8_TO_TCHAR(odr_snapshot(Session)));
}

FString AOdrGameMode::Phase() const {
    return StringField(Snapshot(), TEXT("phase"));
}

FString AOdrGameMode::SelectedClass() const {
    return Classes[ClassIndex];
}
FString AOdrGameMode::SelectedAncestry() const {
    return Ancestries[AncestryIndex];
}
FString AOdrGameMode::SelectedBackground() const {
    return Backgrounds[BackgroundIndex];
}

void AOdrGameMode::ChooseClass(int32 Index) {
    if (Phase() == TEXT("creation")) {
        ClassIndex = FMath::Clamp(Index, 0, 5);
        Refresh();
    }
}

void AOdrGameMode::CycleAncestry() {
    if (Phase() == TEXT("creation")) {
        AncestryIndex = (AncestryIndex + 1) % 4;
        Refresh();
    }
}

void AOdrGameMode::CycleBackground() {
    if (Phase() == TEXT("creation")) {
        BackgroundIndex = (BackgroundIndex + 1) % 3;
        Refresh();
    }
}

bool AOdrGameMode::Command(const FString& JsonCommand) {
    if (!bFollowingPath) {
        CancelNavigation();
    }
    if (Session == nullptr) {
        return false;
    }
    const bool bSuccess = odr_apply(Session, TCHAR_TO_UTF8(*JsonCommand)) != 0;
    LastMessage = bSuccess ? TEXT("") : UTF8_TO_TCHAR(odr_error(Session));
    if (!bSuccess) {
        UE_LOG(LogTemp, Warning, TEXT("ODR command rejected: %s (%s)"), *JsonCommand, *LastMessage);
    } else {
        SelectedTarget.Empty();
        const auto State = Snapshot();
        const TArray<TSharedPtr<FJsonValue>>* Events = nullptr;
        if (State.IsValid() && State->TryGetArrayField(TEXT("events"), Events) &&
            Events != nullptr && !Events->IsEmpty()) {
            PresentationText = StringField(Events->Last()->AsObject(), TEXT("text"));
            PresentationUntil = GetWorld()->GetTimeSeconds() + 3.0f;
        }
    }
    Refresh();
    return bSuccess;
}

void AOdrGameMode::RecruitNext() {
    const auto State = Snapshot();
    const TArray<TSharedPtr<FJsonValue>>* Party = nullptr;
    if (!State.IsValid() || !State->TryGetArrayField(TEXT("party"), Party)) {
        return;
    }
    for (const TCHAR* Recruit : Recruits) {
        bool bPresent = false;
        for (const auto& Member : *Party) {
            bPresent |= StringField(Member->AsObject(), TEXT("id")) == Recruit;
        }
        if (!bPresent) {
            Command(FString::Printf(TEXT("{\"action\":\"recruit\",\"id\":\"%s\"}"), Recruit));
            return;
        }
    }
    LastMessage = TEXT("All eleven companions have joined.");
}

void AOdrGameMode::CreateSelectedHero() {
    if (Phase() == TEXT("creation")) {
        Command(FString::Printf(TEXT("{\"action\":\"create_hero\",\"name\":\"Ari\","
                                     "\"ancestry\":\"%s\",\"class\":\"%s\",\"background\":\"%s\"}"),
                                *SelectedAncestry(), *SelectedClass(), *SelectedBackground()));
    }
}

void AOdrGameMode::Step(int32 Q, int32 R) {
    const auto State = Snapshot();
    const FString CurrentPhase = StringField(State, TEXT("phase"));
    FIntPoint From;
    FString Action;
    if (CurrentPhase == TEXT("overworld")) {
        From = PositionField(ObjectField(State, TEXT("world")), TEXT("pos"));
        Action = TEXT("travel");
    } else if (CurrentPhase == TEXT("dungeon")) {
        From = PositionField(State, TEXT("mine_pos"));
        Action = TEXT("move_dungeon");
    } else if (CurrentPhase == TEXT("battle")) {
        const auto Battle = ObjectField(State, TEXT("battle"));
        const TArray<TSharedPtr<FJsonValue>>* Actors = nullptr;
        const TArray<TSharedPtr<FJsonValue>>* Order = nullptr;
        if (!Battle.IsValid() || !Battle->TryGetArrayField(TEXT("actors"), Actors) ||
            !Battle->TryGetArrayField(TEXT("order"), Order)) {
            return;
        }
        const FString ActingId = ArrayString(*Order, NumberField(Battle, TEXT("turn_index")));
        for (const auto& Actor : *Actors) {
            if (StringField(Actor->AsObject(), TEXT("id")) == ActingId) {
                From = PositionField(Actor->AsObject(), TEXT("pos"));
                break;
            }
        }
        Action = TEXT("move_battle");
    }
    if (!Action.IsEmpty()) {
        Command(CommandWithPosition(Action, From.X + Q, From.Y + R));
    }
}

void AOdrGameMode::CreateGeneratedRun() {
    const auto Runs = ObjectField(Snapshot(), TEXT("runs"));
    const int32 Seed = Runs.IsValid() ? Runs->Values.Num() + 42 : 42;
    Command(FString::Printf(TEXT("{\"action\":\"create_run\",\"mode\":\"generated\",\"seed\":%d}"),
                            Seed));
}

void AOdrGameMode::CreateBossRun() {
    Command(TEXT("{\"action\":\"create_run\",\"mode\":\"generated\",\"seed\":73,\"boss\":true}"));
}

void AOdrGameMode::ChangeFloor() {
    if (Phase() == TEXT("dungeon")) {
        Command(NumberField(Snapshot(), TEXT("mine_floor")) == 1 ? TEXT("{\"action\":\"descend\"}")
                                                                 : TEXT("{\"action\":\"ascend\"}"));
    }
}

void AOdrGameMode::SetAllActive() {
    const auto State = Snapshot();
    const TArray<TSharedPtr<FJsonValue>>* Party = nullptr;
    if (!State.IsValid() || !State->TryGetArrayField(TEXT("party"), Party)) {
        return;
    }
    FString CommandText = TEXT("{\"action\":\"set_active\",\"ids\":[");
    for (int32 Index = 0; Index < Party->Num(); ++Index) {
        if (Index > 0) {
            CommandText += TEXT(",");
        }
        CommandText +=
            TEXT("\"") + StringField((*Party)[Index]->AsObject(), TEXT("id")) + TEXT("\"");
    }
    CommandText += TEXT("]}");
    Command(CommandText);
}

void AOdrGameMode::LevelHero() {
    const FString Hero = StringField(Snapshot(), TEXT("hero_id"));
    if (!Hero.IsEmpty()) {
        Command(FString::Printf(TEXT("{\"action\":\"level_up\",\"id\":\"%s\"}"), *Hero));
    }
}

FString AOdrGameMode::CurrentActorId(const TSharedPtr<FJsonObject>& State) const {
    const auto Battle = ObjectField(State, TEXT("battle"));
    const TArray<TSharedPtr<FJsonValue>>* Order = nullptr;
    if (!Battle.IsValid() || !Battle->TryGetArrayField(TEXT("order"), Order) || Order == nullptr) {
        return FString();
    }
    return ArrayString(*Order, NumberField(Battle, TEXT("turn_index")));
}

bool AOdrGameMode::IsUsableTarget(const TSharedPtr<FJsonObject>& Acting,
                                  const TSharedPtr<FJsonObject>& Candidate, bool bCast,
                                  bool bArea) const {
    if (!Acting.IsValid() || !Candidate.IsValid() || NumberField(Candidate, TEXT("hp")) <= 0) {
        return false;
    }
    if (bArea) {
        return StringField(Candidate, TEXT("team")) == TEXT("enemy");
    }
    const bool bHealing = bCast && StringField(Acting, TEXT("class")) == TEXT("Cleric");
    if (bHealing) {
        return StringField(Candidate, TEXT("team")) == TEXT("party") &&
               NumberField(Candidate, TEXT("hp")) < NumberField(Candidate, TEXT("max_hp")) &&
               NumberField(Acting, TEXT("spell_uses")) > 0;
    }
    return StringField(Candidate, TEXT("team")) == TEXT("enemy");
}

bool AOdrGameMode::SelectTargetForAction(bool bCast, bool bArea, bool bCycle) {
    const auto State = Snapshot();
    const auto Battle = ObjectField(State, TEXT("battle"));
    const TArray<TSharedPtr<FJsonValue>>* Actors = nullptr;
    if (!Battle.IsValid() || !Battle->TryGetArrayField(TEXT("actors"), Actors) ||
        Actors == nullptr) {
        return false;
    }
    const FString ActingId = CurrentActorId(State);
    TSharedPtr<FJsonObject> Acting;
    for (const auto& Actor : *Actors) {
        if (StringField(Actor->AsObject(), TEXT("id")) == ActingId) {
            Acting = Actor->AsObject();
        }
    }
    if (!Acting.IsValid()) {
        return false;
    }
    TArray<FString> Candidates;
    for (const auto& Actor : *Actors) {
        const auto Candidate = Actor->AsObject();
        if (IsUsableTarget(Acting, Candidate, bCast, bArea)) {
            Candidates.Add(StringField(Candidate, TEXT("id")));
        }
    }
    if (Candidates.IsEmpty()) {
        SelectedTarget.Empty();
        LastMessage = bCast && StringField(Acting, TEXT("class")) == TEXT("Cleric")
                          ? TEXT("No wounded ally can receive that spell.")
                          : TEXT("No usable target is available.");
        Refresh();
        return false;
    }
    int32 Index = Candidates.IndexOfByKey(SelectedTarget);
    if (Index == INDEX_NONE || bCycle) {
        Index = Index == INDEX_NONE ? 0 : (Index + 1) % Candidates.Num();
        SelectedTarget = Candidates[Index];
    }
    return true;
}

void AOdrGameMode::CycleTarget() {
    const auto State = Snapshot();
    const auto Battle = ObjectField(State, TEXT("battle"));
    const FString ActingId = CurrentActorId(State);
    TSharedPtr<FJsonObject> Acting;
    const TArray<TSharedPtr<FJsonValue>>* Actors = nullptr;
    if (Battle.IsValid() && Battle->TryGetArrayField(TEXT("actors"), Actors) && Actors != nullptr) {
        for (const auto& Actor : *Actors) {
            if (StringField(Actor->AsObject(), TEXT("id")) == ActingId) {
                Acting = Actor->AsObject();
                break;
            }
        }
    }
    if (Acting.IsValid()) {
        SelectTargetForAction(false, false, true);
        Refresh();
    }
}

void AOdrGameMode::CycleEnemyTarget() {
    CycleTarget();
}

void AOdrGameMode::CycleHealingTarget() {
    const auto State = Snapshot();
    const auto Battle = ObjectField(State, TEXT("battle"));
    if (!Battle.IsValid()) {
        return;
    }
    SelectTargetForAction(true, false, true);
    Refresh();
}

void AOdrGameMode::ActOnSelected(bool bCast, bool bArea) {
    if (!SelectedTarget.IsEmpty()) {
        const auto State = Snapshot();
        const auto Battle = ObjectField(State, TEXT("battle"));
        TSharedPtr<FJsonObject> Acting;
        TSharedPtr<FJsonObject> Target;
        if (Battle.IsValid()) {
            for (const auto& Value : Battle->GetArrayField(TEXT("actors"))) {
                const auto Candidate = Value->AsObject();
                if (StringField(Candidate, TEXT("id")) == CurrentActorId(State))
                    Acting = Candidate;
                if (StringField(Candidate, TEXT("id")) == SelectedTarget)
                    Target = Candidate;
            }
        }
        if (!IsUsableTarget(Acting, Target, bCast, bArea)) {
            LastMessage = TEXT("Select a suitable target for that action.");
            return;
        }
    }
    if (!SelectTargetForAction(bCast, bArea, false)) {
        return;
    }
    if (bArea) {
        const auto State = Snapshot();
        const auto Battle = ObjectField(State, TEXT("battle"));
        const TArray<TSharedPtr<FJsonValue>>* Actors = nullptr;
        if (Battle.IsValid() && Battle->TryGetArrayField(TEXT("actors"), Actors) &&
            Actors != nullptr) {
            for (const auto& ActorValue : *Actors) {
                if (StringField(ActorValue->AsObject(), TEXT("id")) == SelectedTarget) {
                    const FIntPoint Point = PositionField(ActorValue->AsObject(), TEXT("pos"));
                    Command(CommandWithPosition(TEXT("cast_area"), Point.X, Point.Y));
                    return;
                }
            }
        }
        return;
    }
    Command(FString::Printf(TEXT("{\"action\":\"%s\",\"target\":\"%s\"}"),
                            bCast ? TEXT("cast") : TEXT("attack"), *SelectedTarget));
}

void AOdrGameMode::ActOnNearest(bool bCast, bool bArea) {
    // Keep the old adapter entry point for scripts, but route it through the
    // explicit selection seam so keyboard and automation paths agree.
    SelectTargetForAction(bCast, bArea, false);
    ActOnSelected(bCast, bArea);
}

void AOdrGameMode::SaveSession() {
    CancelNavigation();
    if (Session == nullptr) {
        return;
    }
    const FString Path = AutomationSavePath.IsEmpty()
                             ? FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("session.json"))
                             : AutomationSavePath;
    const FString Pending = Path + TEXT(".pending");
    const FString Contents = UTF8_TO_TCHAR(odr_save(Session));
    if (!IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), true) ||
        !FFileHelper::SaveStringToFile(Contents, *Pending,
                                       FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM) ||
        !IFileManager::Get().Move(*Path, *Pending, true)) {
        LastMessage = FString::Printf(TEXT("Save failed: %s"), *Path);
        UE_LOG(LogTemp, Warning, TEXT("%s"), *LastMessage);
        return;
    }
    LastMessage = TEXT("Expedition saved.");
}

void AOdrGameMode::LoadSession() {
    CancelNavigation();
    if (Session == nullptr) {
        return;
    }
    FString Contents;
    const FString Path = AutomationSavePath.IsEmpty()
                             ? FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("session.json"))
                             : AutomationSavePath;
    if (!FFileHelper::LoadFileToString(Contents, *Path) ||
        !odr_load(Session, TCHAR_TO_UTF8(*Contents))) {
        LastMessage = FString::Printf(TEXT("Load failed: %s"), UTF8_TO_TCHAR(odr_error(Session)));
        return;
    }
    LastMessage = TEXT("Session loaded.");
    SelectedTarget.Empty();
    PresentationText.Empty();
    PresentationUntil = 0.0f;
    Refresh();
}

void AOdrGameMode::Refresh() {
    for (AActor* Marker : SceneActors) {
        if (IsValid(Marker)) {
            Marker->Destroy();
        }
    }
    SceneActors.Reset();
    SceneLabels.Reset();
    const auto State = Snapshot();
    if (!State.IsValid()) {
        return;
    }
    const FString CurrentPhase = StringField(State, TEXT("phase"));
    CachedState = State;
    BuildEnvironment(State);
    UpdateCamera(State);
    if (CurrentPhase == TEXT("creation") || CurrentPhase == TEXT("city")) {
        SpawnMarker(TEXT("placeholder.blacksmith"), TEXT("Blacksmith"), 0, 0, 0.0f);
        SpawnMarker(TEXT("placeholder.city"), TEXT("Town gate"), 3, 0, 0.0f);
        if (CurrentPhase == TEXT("city")) {
            SpawnMarker(TEXT("placeholder.hero"), TEXT("Party"), 1, 0, 0.0f);
        } else {
            SpawnMarker(TEXT("placeholder.hero"), TEXT(""), 1, 0, 0.0f);
        }
    } else if (CurrentPhase == TEXT("overworld")) {
        SpawnMarker(TEXT("placeholder.city"), TEXT("City"), 0, 0, 0.0f);
        SpawnMarker(TEXT("placeholder.mine"), TEXT("Mine"), 3, 1, 0.0f);
        SpawnMarker(TEXT("placeholder.ruins"), TEXT("Ruins"), -2, 1, 0.0f);
        const FIntPoint Party = PositionField(ObjectField(State, TEXT("world")), TEXT("pos"));
        SpawnMarker(TEXT("placeholder.hero"), TEXT("Party"), Party.X, Party.Y, 0.0f);
    } else if (CurrentPhase == TEXT("dungeon") || CurrentPhase == TEXT("battle")) {
        const auto Runs = ObjectField(State, TEXT("runs"));
        const auto Run = ObjectField(Runs, StringField(State, TEXT("active_run")));
        const auto Layout = ObjectField(Run, TEXT("layout"));
        if (Layout.IsValid()) {
            for (const TCHAR* Field : {TEXT("walls"), TEXT("rough")}) {
                for (const auto& Tile : Layout->GetArrayField(Field)) {
                    SceneLabels.Add({HexToWorld(Position(Tile->AsArray()), 30),
                                     FString(Field) == TEXT("walls") ? TEXT("WALL") : TEXT("ROUGH"),
                                     FLinearColor::Gray});
                }
            }
        }
        const auto Objects = ObjectField(Layout, TEXT("objects"));
        const auto Changes = ObjectField(Run, TEXT("changes"));
        const FIntPoint MinePosition = PositionField(State, TEXT("mine_pos"));
        const bool bLower = NumberField(State, TEXT("mine_floor")) == 2;
        const bool bBoss = Run.IsValid() && Run->GetBoolField(TEXT("boss"));
        for (const TCHAR* Name :
             {TEXT("entry"), TEXT("clue"), TEXT("hidden_loot"), TEXT("main_trigger"), TEXT("ore"),
              TEXT("secret_trigger"), TEXT("stairs_down"), TEXT("stairs_up")}) {
            const FString ObjectName(Name);
            if ((bLower && ObjectName != TEXT("main_trigger") && ObjectName != TEXT("ore") &&
                 ObjectName != TEXT("stairs_up")) ||
                (!bLower && bBoss &&
                 (ObjectName == TEXT("main_trigger") || ObjectName == TEXT("ore") ||
                  ObjectName == TEXT("stairs_up"))) ||
                (!bBoss && ObjectName.StartsWith(TEXT("stairs")))) {
                continue;
            }
            const bool bHiddenLootTaken =
                Changes.IsValid() && Changes->GetBoolField(TEXT("hidden_loot_taken"));
            const bool bOreTaken = Changes.IsValid() && Changes->GetBoolField(TEXT("ore_taken"));
            const bool bMainWon = Changes.IsValid() && Changes->GetBoolField(TEXT("main_won"));
            const bool bSecretFound =
                Changes.IsValid() && Changes->GetBoolField(TEXT("secret_found"));
            const bool bSecretWon = Changes.IsValid() && Changes->GetBoolField(TEXT("secret_won"));
            const bool bHiddenLootNearby =
                HexDistance(MinePosition, PositionField(Objects, Name)) <= 1;
            const bool bConsumed =
                (ObjectName == TEXT("hidden_loot") && bHiddenLootTaken) ||
                (ObjectName == TEXT("ore") && bOreTaken) ||
                (ObjectName == TEXT("main_trigger") && bMainWon) ||
                (ObjectName == TEXT("secret_trigger") && (!bSecretFound || bSecretWon)) ||
                (ObjectName == TEXT("clue") && bSecretFound) ||
                (ObjectName == TEXT("hidden_loot") && !bHiddenLootNearby && !bSecretFound);
            if (bConsumed) {
                continue;
            }
            if (CurrentPhase == TEXT("battle")) {
                continue;
            }
            const FIntPoint Point = PositionField(Objects, Name);
            FString Label;
            if (ObjectName == TEXT("entry")) {
                Label = TEXT("ENTRY");
            } else if (ObjectName == TEXT("clue")) {
                Label = TEXT("Clue");
            } else if (ObjectName == TEXT("hidden_loot")) {
                Label = TEXT("Cache");
            } else if (ObjectName == TEXT("main_trigger")) {
                Label = TEXT("Main battle");
            } else if (ObjectName == TEXT("secret_trigger")) {
                Label = TEXT("Secret battle");
            } else if (ObjectName == TEXT("stairs_down") || ObjectName == TEXT("stairs_up")) {
                Label = TEXT("Stairs");
            } else {
                Label = TEXT("Ore");
            }
            SpawnMarker(TEXT("placeholder.objective"), Label, Point.X, Point.Y, 0.0f);
        }
        if (CurrentPhase == TEXT("dungeon")) {
            const FIntPoint Party = PositionField(State, TEXT("mine_pos"));
            SpawnMarker(TEXT("placeholder.hero"), TEXT("Party"), Party.X, Party.Y, 0.0f);
        } else {
            const auto Battle = ObjectField(State, TEXT("battle"));
            const FIntPoint Approach = PositionField(Battle, TEXT("approach"));
            SpawnMarker(TEXT("placeholder.objective"), TEXT("RETREAT"), Approach.X, Approach.Y,
                        30.0f);
            const TArray<TSharedPtr<FJsonValue>>* Actors = nullptr;
            if (Battle.IsValid() && Battle->TryGetArrayField(TEXT("actors"), Actors)) {
                int32 EnemyIndex = 0;
                for (const auto& ActorValue : *Actors) {
                    const auto Actor = ActorValue->AsObject();
                    const bool bEnemy = StringField(Actor, TEXT("team")) == TEXT("enemy");
                    if (bEnemy) {
                        ++EnemyIndex;
                    }
                    if (NumberField(Actor, TEXT("hp")) > 0) {
                        const FIntPoint Point = PositionField(Actor, TEXT("pos"));
                        const bool bCurrent =
                            StringField(Actor, TEXT("id")) == CurrentActorId(State);
                        FString Label = bEnemy ? FString::Printf(TEXT("E%d"), EnemyIndex)
                                               : StringField(Actor, TEXT("name")).Left(7);
                        if (bCurrent) {
                            Label += TEXT("*");
                        }
                        if (StringField(Actor, TEXT("id")) == SelectedTarget) {
                            Label += TEXT(">");
                        }
                        SpawnMarker(StringField(Actor, TEXT("visual_id")), Label, Point.X, Point.Y,
                                    0.0f);
                        if (!SceneLabels.IsEmpty()) {
                            auto& Badge = SceneLabels.Last();
                            Badge.ActorId = StringField(Actor, TEXT("id"));
                            Badge.Text = StringField(Actor, TEXT("name"));
                            Badge.HP = NumberField(Actor, TEXT("hp"));
                            Badge.MaxHP = NumberField(Actor, TEXT("max_hp"));
                            Badge.bCurrent = bCurrent;
                            Badge.bSelected = Badge.ActorId == SelectedTarget;
                        }
                    }
                }
            }
        }
    }
}

FString AOdrGameMode::Help() const {
    const FString CurrentPhase = Phase();
    if (CurrentPhase == TEXT("creation")) {
        return TEXT("1-6 class | C ancestry | B background | Enter create hero");
    }
    if (CurrentPhase == TEXT("city")) {
        return TEXT("N recruit | P activate all | M authored | G generated | V boss | U upgrade | "
                    "L level | R rest | X leave");
    }
    if (CurrentPhase == TEXT("battle")) {
        return TEXT("Q W E A S D move | T enemy target | Y wounded ally | F attack | C cast | Z "
                    "mage area | Space defend | R retreat | * turn | > target");
    }
    if (CurrentPhase == TEXT("dungeon")) {
        return TEXT("Q W E A S D move | 1 search | 2 loot | 3 main | 4 secret | 5 ore | 6 stairs | "
                    "X exit | R camp");
    }
    return TEXT("Q W E A S D travel | X enter city or mine | I inspect ruins");
}

FString AOdrGameMode::Status() const {
    const auto State = Snapshot();
    if (!State.IsValid()) {
        return LastMessage;
    }
    FString Result = FString::Printf(
        TEXT("%s | time %d | party %d | run %s"), *StringField(State, TEXT("phase")),
        NumberField(State, TEXT("clock_minutes")), State->GetArrayField(TEXT("active_party")).Num(),
        *StringField(State, TEXT("active_run")));
    if (Phase() == TEXT("creation")) {
        Result += FString::Printf(TEXT("\nHero: %s / %s / %s"), *SelectedAncestry(),
                                  *SelectedClass(), *SelectedBackground());
    }
    const auto Inventory = ObjectField(State, TEXT("inventory"));
    Result +=
        FString::Printf(TEXT("\nGold %d | ore %d | equipment tier %d"),
                        NumberField(Inventory, TEXT("gold")), NumberField(Inventory, TEXT("ore")),
                        NumberField(ObjectField(State, TEXT("equipment")), TEXT("tier")));
    if (Phase() == TEXT("dungeon") || Phase() == TEXT("battle")) {
        Result += FString::Printf(TEXT(" | mine floor %d"), NumberField(State, TEXT("mine_floor")));
    }
    if (Phase() == TEXT("battle")) {
        const auto Battle = ObjectField(State, TEXT("battle"));
        const FString ActingId = CurrentActorId(State);
        const TArray<TSharedPtr<FJsonValue>>* Actors = nullptr;
        if (Battle.IsValid() && Battle->TryGetArrayField(TEXT("actors"), Actors) &&
            Actors != nullptr) {
            for (const auto& ActorValue : *Actors) {
                const auto Actor = ActorValue->AsObject();
                if (StringField(Actor, TEXT("id")) == ActingId) {
                    Result += FString::Printf(
                        TEXT("\nTurn: %s  HP %d/%d  move %d  action %s  spells remaining %d"),
                        *StringField(Actor, TEXT("name")), NumberField(Actor, TEXT("hp")),
                        NumberField(Actor, TEXT("max_hp")), NumberField(Actor, TEXT("move_left")),
                        Actor->GetBoolField(TEXT("acted")) ? TEXT("spent") : TEXT("ready"),
                        NumberField(Actor, TEXT("spell_uses")));
                    break;
                }
            }
        }
        if (!SelectedTarget.IsEmpty() && Actors != nullptr) {
            int32 EnemyIndex = 0;
            for (const auto& ActorValue : *Actors) {
                const auto Actor = ActorValue->AsObject();
                const bool bEnemy = StringField(Actor, TEXT("team")) == TEXT("enemy");
                if (bEnemy) {
                    ++EnemyIndex;
                }
                if (StringField(Actor, TEXT("id")) == SelectedTarget) {
                    const FString Marker =
                        bEnemy ? FString::Printf(TEXT("E%d: "), EnemyIndex) : FString();
                    Result += FString::Printf(TEXT("\nTarget: %s%s  HP %d/%d"), *Marker,
                                              *StringField(Actor, TEXT("name")),
                                              NumberField(Actor, TEXT("hp")),
                                              NumberField(Actor, TEXT("max_hp")));
                    break;
                }
            }
        }
    }
    const TArray<TSharedPtr<FJsonValue>>* Events = nullptr;
    if (State->TryGetArrayField(TEXT("events"), Events) && Events != nullptr &&
        !Events->IsEmpty()) {
        Result += TEXT("\n") + StringField(Events->Last()->AsObject(), TEXT("text"));
    }
    if (!LastMessage.IsEmpty()) {
        Result += TEXT("\n") + LastMessage;
    }
    return Result;
}

FString AOdrGameMode::Presentation() const {
    return GetWorld() != nullptr && GetWorld()->GetTimeSeconds() < PresentationUntil
               ? PresentationText
               : FString();
}

FString AOdrGameMode::CanonicalState() const {
    return Session == nullptr ? FString() : UTF8_TO_TCHAR(odr_snapshot(Session));
}

void AOdrGameMode::RebuildPresentationForAutomation(const FString& Bindings) {
    if (!Bindings.IsEmpty()) {
        Visuals = ParseObject(Bindings);
    }
    Refresh();
}

int32 AOdrGameMode::SkeletalMarkerCountForAutomation() const {
    int32 Count = 0;
    for (const AActor* Marker : SceneActors) {
        if (Marker != nullptr &&
            Marker->FindComponentByClass<USkeletalMeshComponent>() != nullptr) {
            ++Count;
        }
    }
    return Count;
}

void AOdrGameMode::OdrSmoke() {
    bool bSuccess = Session != nullptr;
    if (bSuccess) {
        const FString BeforeRejected = CanonicalState();
        bSuccess &= !Command(TEXT("{\"action\":\"enter_mine\"}"));
        bSuccess &= CanonicalState() == BeforeRejected;
    }
    FString Script;
    bSuccess &= FFileHelper::LoadFileToString(Script, *DataPath(TEXT("blacksmith_mine.jsonl")));
    TArray<FString> Lines;
    Script.ParseIntoArrayLines(Lines, true);
    for (const FString& Line : Lines) {
        if (!bSuccess) {
            break;
        }
        if (!Line.IsEmpty() && !Line.StartsWith(TEXT("#"))) {
            bSuccess = Command(Line);
            if (bSuccess && Line.Contains(TEXT("create_hero"))) {
                bSuccess = SkeletalMarkerCountForAutomation() > 0;
                if (!bSuccess) {
                    UE_LOG(LogTemp, Error,
                           TEXT("ODR smoke skeletal binding was not cooked or loaded"));
                }
            }
            if (!bSuccess) {
                UE_LOG(LogTemp, Error, TEXT("ODR smoke command failed: %s (%s)"), *Line,
                       UTF8_TO_TCHAR(odr_error(Session)));
            }
        }
    }
    const auto State = Snapshot();
    bSuccess &= StringField(State, TEXT("phase")) == TEXT("city") &&
                NumberField(ObjectField(State, TEXT("equipment")), TEXT("tier")) == 1;
    FString Expected;
    bSuccess &=
        FFileHelper::LoadFileToString(Expected, *DataPath(TEXT("blacksmith_mine.expected.json")));
    Expected.TrimEndInline();
    bSuccess &= Session != nullptr && FString(UTF8_TO_TCHAR(odr_snapshot(Session))) == Expected;
    UE_LOG(LogTemp, Log, TEXT("ODR_SMOKE_%s"), bSuccess ? TEXT("PASS") : TEXT("FAIL"));
    FPlatformMisc::RequestExitWithStatus(true, bSuccess ? 0 : 1);
}

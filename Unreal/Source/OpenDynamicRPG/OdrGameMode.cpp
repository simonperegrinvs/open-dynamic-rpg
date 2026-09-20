#include "OdrGameMode.h"

#include "OdrController.h"
#include "OdrHUD.h"
#include "odr/session.h"

#include "Animation/AnimInstance.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/MeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
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
        SceneCamera->GetCameraComponent()->SetOrthoWidth(3600.0f);
        if (APlayerController* Controller = UGameplayStatics::GetPlayerController(GetWorld(), 0)) {
            Controller->SetViewTarget(SceneCamera);
        }
    }
    GetWorld()->SpawnActor<ADirectionalLight>(FVector(0.0, 0.0, 1500.0),
                                              FRotator(-60.0, 25.0, 0.0));
    Refresh();
    if (FParse::Param(FCommandLine::Get(), TEXT("odrsmoke"))) {
        OdrSmoke();
    }
}

void AOdrGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason) {
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
    if (Session == nullptr) {
        return false;
    }
    const bool bSuccess = odr_apply(Session, TCHAR_TO_UTF8(*JsonCommand)) != 0;
    LastMessage = bSuccess ? TEXT("") : UTF8_TO_TCHAR(odr_error(Session));
    if (!bSuccess) {
        UE_LOG(LogTemp, Warning, TEXT("ODR command rejected: %s (%s)"), *JsonCommand,
               *LastMessage);
    } else {
        const auto State = Snapshot();
        const TArray<TSharedPtr<FJsonValue>>* Events = nullptr;
        if (State.IsValid() && State->TryGetArrayField(TEXT("events"), Events) &&
            Events != nullptr && !Events->IsEmpty()) {
            PresentationText = StringField(Events->Last()->AsObject(), TEXT("text"));
            PresentationUntil = GetWorld()->GetTimeSeconds() + 0.75f;
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

void AOdrGameMode::ActOnNearest(bool bCast, bool bArea) {
    const auto State = Snapshot();
    const auto Battle = ObjectField(State, TEXT("battle"));
    const TArray<TSharedPtr<FJsonValue>>* Actors = nullptr;
    const TArray<TSharedPtr<FJsonValue>>* Order = nullptr;
    if (!Battle.IsValid() || !Battle->TryGetArrayField(TEXT("actors"), Actors) ||
        !Battle->TryGetArrayField(TEXT("order"), Order)) {
        return;
    }
    const FString ActingId = ArrayString(*Order, NumberField(Battle, TEXT("turn_index")));
    TSharedPtr<FJsonObject> Acting;
    for (const auto& Actor : *Actors) {
        if (StringField(Actor->AsObject(), TEXT("id")) == ActingId) {
            Acting = Actor->AsObject();
        }
    }
    if (!Acting.IsValid()) {
        return;
    }
    const FIntPoint From = PositionField(Acting, TEXT("pos"));
    int32 Best = MAX_int32;
    FString Target;
    FIntPoint TargetPosition;
    const bool bHealing = bCast && !bArea && StringField(Acting, TEXT("class")) == TEXT("Cleric");
    for (const auto& Actor : *Actors) {
        const auto Candidate = Actor->AsObject();
        if (StringField(Candidate, TEXT("team")) != (bHealing ? TEXT("party") : TEXT("enemy")) ||
            NumberField(Candidate, TEXT("hp")) <= 0) {
            continue;
        }
        const FIntPoint To = PositionField(Candidate, TEXT("pos"));
        const int32 DQ = From.X - To.X;
        const int32 DR = From.Y - To.Y;
        const int32 Distance = (FMath::Abs(DQ) + FMath::Abs(DR) + FMath::Abs(DQ + DR)) / 2;
        if (Distance < Best) {
            Best = Distance;
            Target = StringField(Candidate, TEXT("id"));
            TargetPosition = To;
        }
    }
    if (!Target.IsEmpty()) {
        if (bArea) {
            Command(CommandWithPosition(TEXT("cast_area"), TargetPosition.X, TargetPosition.Y));
        } else {
            Command(FString::Printf(TEXT("{\"action\":\"%s\",\"target\":\"%s\"}"),
                                    bCast ? TEXT("cast") : TEXT("attack"), *Target));
        }
    }
}

void AOdrGameMode::SaveSession() {
    if (Session == nullptr) {
        return;
    }
    const FString Path = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("session.json"));
    const FString Pending = Path + TEXT(".pending");
    const FString Contents = UTF8_TO_TCHAR(odr_save(Session));
    if (!FFileHelper::SaveStringToFile(Contents, *Pending,
                                       FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM) ||
        !IFileManager::Get().Move(*Path, *Pending, true)) {
        LastMessage = TEXT("Save failed.");
        return;
    }
    LastMessage = FString::Printf(TEXT("Saved to %s"), *Path);
}

void AOdrGameMode::LoadSession() {
    if (Session == nullptr) {
        return;
    }
    FString Contents;
    const FString Path = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("session.json"));
    if (!FFileHelper::LoadFileToString(Contents, *Path) ||
        !odr_load(Session, TCHAR_TO_UTF8(*Contents))) {
        LastMessage = FString::Printf(TEXT("Load failed: %s"), UTF8_TO_TCHAR(odr_error(Session)));
        return;
    }
    LastMessage = TEXT("Session loaded.");
    PresentationText.Empty();
    PresentationUntil = 0.0f;
    Refresh();
}

void AOdrGameMode::SpawnMarker(const FString& VisualId, const FString& Label, int32 Q, int32 R,
                               float Height) {
    AActor* Marker = GetWorld()->SpawnActor<AActor>();
    if (Marker == nullptr) {
        return;
    }
    SceneActors.Add(Marker);
    USceneComponent* Root = NewObject<USceneComponent>(Marker);
    Marker->SetRootComponent(Root);
    Marker->AddInstanceComponent(Root);
    Root->RegisterComponent();
    const FVector Location(145.0 * (Q + 0.5 * R), 126.0 * R, Height);
    Marker->SetActorLocation(Location);

    const auto Binding = ObjectField(Visuals, VisualId);
    const FString ModelPath = StringField(Binding, TEXT("model"));
    UMeshComponent* Mesh = nullptr;
    if (StringField(Binding, TEXT("mesh_type")) == TEXT("skeletal") && !ModelPath.IsEmpty()) {
        if (USkeletalMesh* Model = LoadObject<USkeletalMesh>(nullptr, *ModelPath)) {
            USkeletalMeshComponent* Skeletal = NewObject<USkeletalMeshComponent>(Marker);
            Skeletal->SetSkeletalMeshAsset(Model);
            const FString AnimationClass = StringField(Binding, TEXT("animation_class"));
            if (!AnimationClass.IsEmpty()) {
                if (UClass* Animation = LoadClass<UAnimInstance>(nullptr, *AnimationClass)) {
                    Skeletal->SetAnimInstanceClass(Animation);
                }
            }
            Mesh = Skeletal;
        }
    }
    if (Mesh == nullptr) {
        FString MeshPath = ModelPath;
        if (MeshPath.IsEmpty() || StringField(Binding, TEXT("mesh_type")) == TEXT("skeletal")) {
            MeshPath = StringField(Binding, TEXT("fallback_mesh"));
        }
        UStaticMesh* MeshAsset = LoadObject<UStaticMesh>(nullptr, *MeshPath);
        if (MeshAsset == nullptr) {
            MeshAsset = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
        }
        UStaticMeshComponent* Static = NewObject<UStaticMeshComponent>(Marker);
        Static->SetStaticMesh(MeshAsset);
        Mesh = Static;
    }
    Marker->AddInstanceComponent(Mesh);
    Mesh->SetupAttachment(Root);
    Mesh->SetWorldScale3D(Label.IsEmpty() ? FVector(0.8, 0.8, 0.10) : FVector(1.2, 1.2, 1.3));
    const FString MaterialPath = StringField(Binding, TEXT("material"));
    if (!MaterialPath.IsEmpty()) {
        if (UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, *MaterialPath)) {
            Mesh->SetMaterial(0, Material);
        }
    }
    Mesh->RegisterComponent();
    if (!Label.IsEmpty()) {
        const FLinearColor Color = VisualId.Contains(TEXT("enemy")) ||
                                           VisualId.Contains(TEXT("boss"))
                                       ? FLinearColor::Red
                                   : VisualId.Contains(TEXT("hero")) ? FLinearColor(0.0f, 1.0f, 1.0f)
                                                                      : FLinearColor::Yellow;
        SceneLabels.Add({Location + FVector(0.0, 0.0, 170.0), Label, Color});
    }
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
    if (SceneCamera != nullptr) {
        if (CurrentPhase == TEXT("creation") || CurrentPhase == TEXT("city")) {
            SceneCamera->SetActorLocation(FVector(220.0, 0.0, 2400.0));
            SceneCamera->GetCameraComponent()->SetOrthoWidth(1600.0f);
        } else if (CurrentPhase == TEXT("overworld")) {
            SceneCamera->SetActorLocation(FVector(100.0, 0.0, 2400.0));
            SceneCamera->GetCameraComponent()->SetOrthoWidth(2200.0f);
        } else {
            SceneCamera->SetActorLocation(FVector(1230.0, 0.0, 3000.0));
            SceneCamera->GetCameraComponent()->SetOrthoWidth(3900.0f);
        }
    }
    if (CurrentPhase == TEXT("creation") || CurrentPhase == TEXT("city")) {
        SpawnMarker(TEXT("placeholder.blacksmith"), TEXT("Blacksmith"), 0, 0, 0.0f);
        SpawnMarker(TEXT("placeholder.city"), TEXT("Town gate"), 3, 0, 0.0f);
        if (CurrentPhase == TEXT("city")) {
            SpawnMarker(TEXT("placeholder.hero"), TEXT("Party"), 1, 0, 0.0f);
        }
    } else if (CurrentPhase == TEXT("overworld")) {
        SpawnMarker(TEXT("placeholder.city"), TEXT("City"), 0, 0, 0.0f);
        SpawnMarker(TEXT("placeholder.mine"), TEXT("Mine"), 3, 1, 0.0f);
        SpawnMarker(TEXT("placeholder.ruins"), TEXT("Ruins"), -2, 1, 0.0f);
        const FIntPoint Party = PositionField(ObjectField(State, TEXT("world")), TEXT("pos"));
        SpawnMarker(TEXT("placeholder.hero"), TEXT("Party"), Party.X, Party.Y, 100.0f);
    } else if (CurrentPhase == TEXT("dungeon") || CurrentPhase == TEXT("battle")) {
        const auto Runs = ObjectField(State, TEXT("runs"));
        const auto Run = ObjectField(Runs, StringField(State, TEXT("active_run")));
        const auto Layout = ObjectField(Run, TEXT("layout"));
        const TArray<TSharedPtr<FJsonValue>>* Tiles = nullptr;
        if (Layout.IsValid() && Layout->TryGetArrayField(TEXT("tiles"), Tiles)) {
            for (const auto& Tile : *Tiles) {
                const FIntPoint Point = Position(Tile->AsArray());
                SpawnMarker(TEXT("placeholder.floor"), TEXT(""), Point.X, Point.Y, -80.0f);
            }
        }
        const auto Objects = ObjectField(Layout, TEXT("objects"));
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
            const FIntPoint Point = PositionField(Objects, Name);
            SpawnMarker(TEXT("placeholder.objective"), Name, Point.X, Point.Y, 0.0f);
        }
        if (CurrentPhase == TEXT("dungeon")) {
            const FIntPoint Party = PositionField(State, TEXT("mine_pos"));
            SpawnMarker(TEXT("placeholder.hero"), TEXT("Party"), Party.X, Party.Y, 100.0f);
        } else {
            const auto Battle = ObjectField(State, TEXT("battle"));
            const TArray<TSharedPtr<FJsonValue>>* Actors = nullptr;
            if (Battle.IsValid() && Battle->TryGetArrayField(TEXT("actors"), Actors)) {
                for (const auto& ActorValue : *Actors) {
                    const auto Actor = ActorValue->AsObject();
                    if (NumberField(Actor, TEXT("hp")) > 0) {
                        const FIntPoint Point = PositionField(Actor, TEXT("pos"));
                        SpawnMarker(StringField(Actor, TEXT("visual_id")),
                                    StringField(Actor, TEXT("name")), Point.X, Point.Y, 100.0f);
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
        return TEXT(
            "Q W E A S D move | F attack | C cast | Z mage area | Space defend | R retreat");
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

void AOdrGameMode::OdrSmoke() {
    bool bSuccess = Session != nullptr;
    if (bSuccess) {
        const FString BeforeRejected = UTF8_TO_TCHAR(odr_snapshot(Session));
        bSuccess &= odr_apply(Session, "{\"action\":\"enter_mine\"}") == 0;
        bSuccess &= FString(UTF8_TO_TCHAR(odr_snapshot(Session))) == BeforeRejected;
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
            bSuccess = odr_apply(Session, TCHAR_TO_UTF8(*Line)) != 0;
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

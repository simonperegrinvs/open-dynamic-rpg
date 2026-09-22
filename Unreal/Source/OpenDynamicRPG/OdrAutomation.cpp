#include "odr/session.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "OdrGameMode.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOdrBlacksmithMineTest, "ODR.BlacksmithMine",
                                 EAutomationTestFlags::EditorContext |
                                     EAutomationTestFlags::EngineFilter)

bool FOdrBlacksmithMineTest::RunTest(const FString& Parameters) {
    const FString Data = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data"));
    FString Definition;
    FString Scenario;
    if (!TestTrue(
            TEXT("staged definition exists"),
            FFileHelper::LoadFileToString(Definition, *FPaths::Combine(Data, TEXT("mine.json")))) ||
        !TestTrue(TEXT("staged scenario exists"),
                  FFileHelper::LoadFileToString(
                      Scenario, *FPaths::Combine(Data, TEXT("blacksmith_mine.jsonl"))))) {
        return false;
    }
    OdrSession* Session = odr_create(TCHAR_TO_UTF8(*Definition));
    if (!TestNotNull(TEXT("portable session initializes"), Session)) {
        return false;
    }
    const FString BeforeRejected = UTF8_TO_TCHAR(odr_snapshot(Session));
    const bool bRejected = odr_apply(Session, "{\"action\":\"enter_mine\"}") == 0;
    TestTrue(TEXT("unavailable interaction is rejected without crashing"), bRejected);
    TestEqual(TEXT("rejected interaction preserves state"),
              FString(UTF8_TO_TCHAR(odr_snapshot(Session))), BeforeRejected);
    TArray<FString> Lines;
    Scenario.ParseIntoArrayLines(Lines, true);
    bool bSuccess = true;
    for (int32 Index = 0; Index < Lines.Num(); ++Index) {
        const FString& Line = Lines[Index];
        if (Line.IsEmpty() || Line.StartsWith(TEXT("#"))) {
            continue;
        }
        if (!odr_apply(Session, TCHAR_TO_UTF8(*Line))) {
            AddError(FString::Printf(TEXT("scenario line %d: %s"), Index + 1,
                                     UTF8_TO_TCHAR(odr_error(Session))));
            bSuccess = false;
            break;
        }
        if (Index == Lines.Num() / 2) {
            const FString Before = UTF8_TO_TCHAR(odr_save(Session));
            OdrSession* Loaded = odr_create(TCHAR_TO_UTF8(*Definition));
            if (Loaded == nullptr || !odr_load(Loaded, TCHAR_TO_UTF8(*Before)) ||
                Before != UTF8_TO_TCHAR(odr_snapshot(Loaded))) {
                AddError(TEXT("Unreal host save/reload changed canonical state"));
                bSuccess = false;
            }
            odr_destroy(Loaded);
            if (!bSuccess) {
                break;
            }
        }
    }
    const FString Final = UTF8_TO_TCHAR(odr_snapshot(Session));
    FString Expected;
    if (TestTrue(TEXT("headless canonical state is staged"),
                 FFileHelper::LoadFileToString(
                     Expected, *FPaths::Combine(Data, TEXT("blacksmith_mine.expected.json"))))) {
        Expected.TrimEndInline();
        bSuccess &=
            TestEqual(TEXT("Unreal state equals headless canonical state"), Final, Expected);
    } else {
        bSuccess = false;
    }
    TSharedPtr<FJsonObject> State;
    const auto Reader = TJsonReaderFactory<>::Create(Final);
    bSuccess &= FJsonSerializer::Deserialize(Reader, State) && State.IsValid();
    if (State.IsValid()) {
        bSuccess &= State->GetStringField(TEXT("phase")) == TEXT("city");
        bSuccess &= State->GetObjectField(TEXT("equipment"))->GetIntegerField(TEXT("tier")) == 1;
    }
    TestTrue(TEXT("blacksmith mine ends in town with tier-one equipment"), bSuccess);
    odr_destroy(Session);
    return bSuccess;
}

#endif

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOdrPlayerAdapterTest, "ODR.PlayerAdapter",
                                 EAutomationTestFlags::EditorContext |
                                     EAutomationTestFlags::EngineFilter)

namespace {
TSharedPtr<FJsonObject> ReadState(const FString& Text) {
    TSharedPtr<FJsonObject> State;
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), State);
    return State;
}

TSharedPtr<FJsonObject> BattleActor(const TSharedPtr<FJsonObject>& State, const FString& Id) {
    for (const auto& Value : State->GetObjectField(TEXT("battle"))->GetArrayField(TEXT("actors"))) {
        if (Value->AsObject()->GetStringField(TEXT("id")) == Id) {
            return Value->AsObject();
        }
    }
    return nullptr;
}

void SetPosition(const TSharedPtr<FJsonObject>& Object, int32 Q, int32 R) {
    Object->SetArrayField(TEXT("pos"),
                          {MakeShared<FJsonValueNumber>(Q), MakeShared<FJsonValueNumber>(R)});
}

bool HasLabel(const AOdrGameMode* Mode, const FString& Text) {
    return Mode->Labels().ContainsByPredicate(
        [&](const FOdrSceneLabel& Label) { return Label.Text == Text; });
}
} // namespace

bool FOdrPlayerAdapterTest::RunTest(const FString& Parameters) {
    if (!TestNotNull(TEXT("engine for isolated game world"), GEngine)) {
        return false;
    }
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("isolated Unreal automation world"), World)) {
        return false;
    }
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    AOdrGameMode* Mode = World->SpawnActor<AOdrGameMode>();
    const FString SavePath =
        FPaths::Combine(FPaths::ProjectSavedDir(),
                        FString::Printf(TEXT("automation-%s.json"),
                                        *FGuid::NewGuid().ToString(EGuidFormats::Digits)));
    ON_SCOPE_EXIT {
        if (Mode != nullptr) {
            Mode->EndPlay(EEndPlayReason::Quit);
        }
        IFileManager::Get().Delete(*SavePath, false, true, true);
        GEngine->DestroyWorldContext(World);
        World->DestroyWorld(false);
    };
    if (!TestNotNull(TEXT("isolated GameMode"), Mode)) {
        return false;
    }
    Mode->DispatchBeginPlay();
    Mode->SetAutomationSavePath(SavePath);
    if (!TestEqual(TEXT("new adapter session"), Mode->Phase(), FString(TEXT("creation")))) {
        return false;
    }
    Mode->ChooseClass(2);
    Mode->CycleAncestry();
    Mode->CycleAncestry();
    Mode->CycleBackground();
    Mode->CreateSelectedHero();
    for (int32 Index = 0; Index < 7; ++Index) {
        Mode->RecruitNext();
    }
    const FString Town = Mode->CanonicalState();
    TestTrue(TEXT("original skeletal probe resolves in the game scene"),
             Mode->SkeletalMarkerCountForAutomation() > 0);
    Mode->RebuildPresentationForAutomation(TEXT("{}"));
    TestEqual(TEXT("unbound visuals fall back to primitives"),
              Mode->SkeletalMarkerCountForAutomation(), 0);
    TestEqual(TEXT("model substitution preserves gameplay"), Mode->CanonicalState(), Town);
    FString Bindings;
    FFileHelper::LoadFileToString(
        Bindings, *FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data/visuals.json")));
    Mode->RebuildPresentationForAutomation(Bindings);
    TestEqual(TEXT("model restoration preserves gameplay"), Mode->CanonicalState(), Town);
    if (!TestTrue(TEXT("accept mine"), Mode->Command(TEXT("{\"action\":\"create_run\"}"))) ||
        !TestTrue(TEXT("leave city"), Mode->Command(TEXT("{\"action\":\"leave_city\"}")))) {
        return false;
    }
    for (int32 Index = 0; Index < 3; ++Index) {
        Mode->Step(1, 0);
    }
    Mode->Step(0, 1);
    if (!TestTrue(TEXT("enter mine via adapter"),
                  Mode->Command(TEXT("{\"action\":\"enter_mine\"}")))) {
        return false;
    }
    TestTrue(TEXT("walls are visible"), HasLabel(Mode, TEXT("WALL")));
    TestTrue(TEXT("rough terrain is visible"), HasLabel(Mode, TEXT("ROUGH")));
    TestFalse(TEXT("secret is undiscovered"), HasLabel(Mode, TEXT("Secret battle")));
    TestFalse(TEXT("cache is hidden at a distance"), HasLabel(Mode, TEXT("Cache")));
    for (int32 Index = 0; Index < 4; ++Index) {
        Mode->Step(1, 0);
    }
    Mode->Step(0, 1);
    Mode->Step(0, 1);
    TestTrue(TEXT("clue search"), Mode->Command(TEXT("{\"action\":\"search\"}")));
    TestTrue(TEXT("discovered secret is visible"), HasLabel(Mode, TEXT("Secret battle")));
    Mode->Step(1, 0);
    Mode->Step(0, 1);
    TestTrue(TEXT("nearby cache is visible"), HasLabel(Mode, TEXT("Cache")));
    TestTrue(TEXT("take cache"), Mode->Command(TEXT("{\"action\":\"take_hidden_loot\"}")));
    TestFalse(TEXT("collected cache is removed"), HasLabel(Mode, TEXT("Cache")));
    Mode->Step(1, -1);
    Mode->Step(1, -1);
    if (!TestTrue(TEXT("enter battle from reached approach"),
                  Mode->Command(TEXT("{\"action\":\"begin_main\"}")))) {
        return false;
    }
    TestTrue(TEXT("retreat entry is visible"), HasLabel(Mode, TEXT("RETREAT")));
    const FString BattleBaseline = Mode->CanonicalState();
    // Valid, controlled battle fixtures isolate targeting from enemy AI and RNG.
    // They enter via the normal save-file loader and keep run/party identities.
    auto Fixture = [&](const FString& ActingId) {
        auto State = ReadState(BattleBaseline);
        State->SetNumberField(TEXT("rng"), 1);
        auto Battle = State->GetObjectField(TEXT("battle"));
        const auto& Order = Battle->GetArrayField(TEXT("order"));
        for (int32 Index = 0; Index < Order.Num(); ++Index) {
            if (Order[Index]->AsString() == ActingId) {
                Battle->SetNumberField(TEXT("turn_index"), Index);
            }
        }
        int32 PartyIndex = 0;
        int32 EnemyIndex = 0;
        for (const auto& Value : Battle->GetArrayField(TEXT("actors"))) {
            auto Actor = Value->AsObject();
            Actor->SetNumberField(TEXT("hp"), Actor->GetIntegerField(TEXT("max_hp")));
            Actor->SetNumberField(TEXT("poison"), 0);
            Actor->SetNumberField(TEXT("move_left"), 4);
            Actor->SetBoolField(TEXT("acted"), false);
            if (Actor->GetStringField(TEXT("team")) == TEXT("party")) {
                SetPosition(Actor, 4, -5 + PartyIndex++);
            } else {
                SetPosition(Actor, 8 + EnemyIndex++, 1);
            }
        }
        return State;
    };
    auto LoadFixture = [&](const TSharedPtr<FJsonObject>& State) {
        FString Text;
        FJsonSerializer::Serialize(State.ToSharedRef(), TJsonWriterFactory<>::Create(&Text));
        if (!TestTrue(TEXT("write isolated save fixture"),
                      FFileHelper::SaveStringToFile(Text, *SavePath))) {
            return false;
        }
        Mode->LoadSession();
        return TestEqual(
            TEXT("fixture loaded by actual save path"),
            ReadState(Mode->CanonicalState())
                ->GetObjectField(TEXT("battle"))
                ->GetIntegerField(TEXT("turn_index")),
            State->GetObjectField(TEXT("battle"))->GetIntegerField(TEXT("turn_index")));
    };
    auto Healing = Fixture(TEXT("bran"));
    SetPosition(BattleActor(Healing, TEXT("bran")), 6, 1);
    SetPosition(BattleActor(Healing, TEXT("rowan")), 7, 1);
    BattleActor(Healing, TEXT("rowan"))->SetNumberField(TEXT("hp"), 5);
    if (!LoadFixture(Healing)) {
        return false;
    }
    const int32 Uses = BattleActor(Healing, TEXT("bran"))->GetIntegerField(TEXT("spell_uses"));
    Mode->CycleHealingTarget();
    TestEqual(TEXT("Y selects wounded ally instead of caster"), Mode->SelectedTargetId(),
              FString(TEXT("rowan")));
    Mode->ActOnSelected(true);
    auto Healed = ReadState(Mode->CanonicalState());
    TestEqual(TEXT("C heals selected ally"),
              BattleActor(Healed, TEXT("rowan"))->GetIntegerField(TEXT("hp")), 13);
    TestEqual(TEXT("healing spends exactly one use"),
              BattleActor(Healed, TEXT("bran"))->GetIntegerField(TEXT("spell_uses")), Uses - 1);
    auto FullHealth = Fixture(TEXT("bran"));
    if (!LoadFixture(FullHealth)) {
        return false;
    }
    const FString BeforeNoHeal = Mode->CanonicalState();
    Mode->ActOnSelected(true);
    TestEqual(TEXT("full-health party does not spend a spell or action"), Mode->CanonicalState(),
              BeforeNoHeal);

    const FString HeroId = ReadState(BattleBaseline)->GetStringField(TEXT("hero_id"));
    auto Attack = Fixture(HeroId);
    SetPosition(BattleActor(Attack, HeroId), 6, 1);
    if (!LoadFixture(Attack)) {
        return false;
    }
    Mode->Step(1, 0);
    Mode->CycleEnemyTarget();
    const FString FirstEnemy = Mode->SelectedTargetId();
    Mode->CycleEnemyTarget();
    const FString Target = Mode->SelectedTargetId();
    TestTrue(TEXT("T advances to a different living enemy"),
             !Target.IsEmpty() && Target != FirstEnemy);
    const FString BeforeAction = Mode->CanonicalState();
    const int32 TargetHP =
        BattleActor(ReadState(BeforeAction), Target)->GetIntegerField(TEXT("hp"));
    Mode->SaveSession();
    TestTrue(TEXT("save file exists between movement and action"),
             IFileManager::Get().FileExists(*SavePath));
    Mode->ActOnSelected(false);
    auto Attacked = ReadState(Mode->CanonicalState());
    TestTrue(TEXT("F damages the selected second enemy"),
             BattleActor(Attacked, Target)->GetIntegerField(TEXT("hp")) < TargetHP);
    TestTrue(TEXT("action changed canonical state"), Mode->CanonicalState() != BeforeAction);
    Mode->LoadSession();
    TestEqual(TEXT("file reload restores movement and unspent action"), Mode->CanonicalState(),
              BeforeAction);
    Mode->RebuildPresentationForAutomation();
    TestEqual(TEXT("presentation rebuild preserves canonical state"), Mode->CanonicalState(),
              BeforeAction);
    TestFalse(TEXT("adapter rejects unavailable interaction"),
              Mode->Command(TEXT("{\"action\":\"enter_mine\"}")));
    TestEqual(TEXT("rejected adapter command rolls back"), Mode->CanonicalState(), BeforeAction);
    return true;
}

#endif

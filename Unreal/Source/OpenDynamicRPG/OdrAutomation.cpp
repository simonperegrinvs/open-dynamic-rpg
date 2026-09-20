#include "odr/session.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
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

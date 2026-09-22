#include "OdrGameMode.h"

#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "TimerManager.h"

namespace {

using FJsonArray = TArray<TSharedPtr<FJsonValue>>;

TSharedPtr<FJsonObject> ObjectField(const TSharedPtr<FJsonObject>& Object, const FString& Name) {
    const TSharedPtr<FJsonObject>* Value = nullptr;
    return Object.IsValid() && Object->TryGetObjectField(Name, Value) && Value != nullptr ? *Value
                                                                                          : nullptr;
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

FIntPoint Position(const TSharedPtr<FJsonValue>& Value) {
    if (!Value.IsValid() || Value->Type != EJson::Array) {
        return FIntPoint::ZeroValue;
    }
    const FJsonArray& Values = Value->AsArray();
    return Values.Num() >= 2 ? FIntPoint(static_cast<int32>(Values[0]->AsNumber()),
                                         static_cast<int32>(Values[1]->AsNumber()))
                             : FIntPoint::ZeroValue;
}

FIntPoint PositionField(const TSharedPtr<FJsonObject>& Object, const FString& Name) {
    const FJsonArray* Values = nullptr;
    if (!Object.IsValid() || !Object->TryGetArrayField(Name, Values) || Values == nullptr ||
        Values->Num() < 2) {
        return FIntPoint::ZeroValue;
    }
    return FIntPoint(static_cast<int32>((*Values)[0]->AsNumber()),
                     static_cast<int32>((*Values)[1]->AsNumber()));
}

int32 HexDistance(const FIntPoint& A, const FIntPoint& B) {
    const int32 DQ = A.X - B.X;
    const int32 DR = A.Y - B.Y;
    return (FMath::Abs(DQ) + FMath::Abs(DR) + FMath::Abs(DQ + DR)) / 2;
}

TArray<FIntPoint> Neighbors(const FIntPoint& Point) {
    return {{Point.X + 1, Point.Y}, {Point.X + 1, Point.Y - 1}, {Point.X, Point.Y - 1},
            {Point.X - 1, Point.Y}, {Point.X - 1, Point.Y + 1}, {Point.X, Point.Y + 1}};
}

bool ContainsPoint(const TSharedPtr<FJsonObject>& Object, const FString& Name,
                   const FIntPoint& Point) {
    const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
    if (!Object.IsValid() || !Object->TryGetArrayField(Name, Values) || Values == nullptr) {
        return false;
    }
    for (const TSharedPtr<FJsonValue>& Value : *Values) {
        if (Position(Value) == Point) {
            return true;
        }
    }
    return false;
}

FString PositionCommand(const TCHAR* Action, const FIntPoint& Point) {
    return FString::Printf(TEXT("{\"action\":\"%s\",\"q\":%d,\"r\":%d}"), Action, Point.X, Point.Y);
}

} // namespace

bool AOdrGameMode::IsNavigating() const {
    return NavigationQueue.Num() > 0;
}

void AOdrGameMode::CancelNavigation() {
    bFollowingPath = false;
    NavigationQueue.Reset();
    if (GetWorld() != nullptr) {
        GetWorld()->GetTimerManager().ClearTimer(NavigationTimer);
    }
}

FIntPoint AOdrGameMode::PartyHex() const {
    const TSharedPtr<FJsonObject> State = CachedState.IsValid() ? CachedState : Snapshot();
    const FString CurrentPhase = StringField(State, TEXT("phase"));
    if (CurrentPhase == TEXT("overworld")) {
        return PositionField(ObjectField(State, TEXT("world")), TEXT("pos"));
    }
    if (CurrentPhase == TEXT("dungeon")) {
        return PositionField(State, TEXT("mine_pos"));
    }
    if (CurrentPhase == TEXT("battle")) {
        const TSharedPtr<FJsonObject> Battle = ObjectField(State, TEXT("battle"));
        const FString ActorId = CurrentActorId(State);
        const TArray<TSharedPtr<FJsonValue>>* Actors = nullptr;
        if (Battle.IsValid() && Battle->TryGetArrayField(TEXT("actors"), Actors) &&
            Actors != nullptr) {
            for (const TSharedPtr<FJsonValue>& Value : *Actors) {
                const TSharedPtr<FJsonObject> Actor = Value->AsObject();
                if (StringField(Actor, TEXT("id")) == ActorId) {
                    return PositionField(Actor, TEXT("pos"));
                }
            }
        }
    }
    return FIntPoint::ZeroValue;
}

bool AOdrGameMode::IsWalkableHex(FIntPoint Point) const {
    const TSharedPtr<FJsonObject> State = CachedState.IsValid() ? CachedState : Snapshot();
    const FString CurrentPhase = StringField(State, TEXT("phase"));
    if (CurrentPhase == TEXT("overworld")) {
        return HexDistance(FIntPoint::ZeroValue, Point) <= 5;
    }
    if (CurrentPhase != TEXT("dungeon") && CurrentPhase != TEXT("battle")) {
        return false;
    }
    TSharedPtr<FJsonObject> Layout;
    if (CurrentPhase == TEXT("dungeon")) {
        const TSharedPtr<FJsonObject> Runs = ObjectField(State, TEXT("runs"));
        Layout =
            ObjectField(ObjectField(Runs, StringField(State, TEXT("active_run"))), TEXT("layout"));
    } else {
        const TSharedPtr<FJsonObject> Battle = ObjectField(State, TEXT("battle"));
        const TSharedPtr<FJsonObject> Runs = ObjectField(State, TEXT("runs"));
        Layout =
            ObjectField(ObjectField(Runs, StringField(State, TEXT("active_run"))), TEXT("layout"));
        if (!Battle.IsValid()) {
            return false;
        }
    }
    return Layout.IsValid() && ContainsPoint(Layout, TEXT("tiles"), Point) &&
           !ContainsPoint(Layout, TEXT("walls"), Point);
}

FString AOdrGameMode::Objective() const {
    const TSharedPtr<FJsonObject> State = CachedState.IsValid() ? CachedState : Snapshot();
    const FString CurrentPhase = StringField(State, TEXT("phase"));
    if (CurrentPhase == TEXT("creation"))
        return TEXT("Create your hero");
    if (CurrentPhase == TEXT("city")) {
        if (NumberField(ObjectField(State, TEXT("equipment")), TEXT("tier")) >= 1) {
            return TEXT("The signature blade is improved");
        }
        if (NumberField(ObjectField(State, TEXT("inventory")), TEXT("ore")) > 0) {
            return TEXT("Visit the forge to improve the blade");
        }
        const TSharedPtr<FJsonObject> Runs = ObjectField(State, TEXT("runs"));
        if (Runs.IsValid() && Runs->Values.Num() == 0)
            return TEXT("Accept an expedition");
        return TEXT("Prepare the party and leave town");
    }
    if (CurrentPhase == TEXT("overworld")) {
        if (NumberField(ObjectField(State, TEXT("inventory")), TEXT("ore")) > 0) {
            return TEXT("Return to town and visit the forge");
        }
        return TEXT("Travel to the mine");
    }
    if (CurrentPhase == TEXT("battle"))
        return TEXT("Win the encounter");
    if (CurrentPhase == TEXT("dungeon")) {
        const TSharedPtr<FJsonObject> Runs = ObjectField(State, TEXT("runs"));
        const TSharedPtr<FJsonObject> Run =
            ObjectField(Runs, StringField(State, TEXT("active_run")));
        const TSharedPtr<FJsonObject> Changes = ObjectField(Run, TEXT("changes"));
        if (Changes.IsValid() && !Changes->GetBoolField(TEXT("main_won")))
            return TEXT("Reach and clear the main encounter");
        if (Changes.IsValid() && !Changes->GetBoolField(TEXT("ore_taken")))
            return TEXT("Recover the guarded ore");
        return TEXT("Return to town");
    }
    return TEXT("Continue the adventure");
}

TArray<FIntPoint> AOdrGameMode::PreviewPath(FIntPoint Destination) const {
    TArray<FIntPoint> Empty;
    const TSharedPtr<FJsonObject> State = CachedState.IsValid() ? CachedState : Snapshot();
    const FString CurrentPhase = StringField(State, TEXT("phase"));
    if (CurrentPhase != TEXT("overworld") && CurrentPhase != TEXT("dungeon") &&
        CurrentPhase != TEXT("battle"))
        return Empty;
    const TSharedPtr<FJsonObject> Runs = ObjectField(State, TEXT("runs"));
    const TSharedPtr<FJsonObject> Run = ObjectField(Runs, StringField(State, TEXT("active_run")));
    const TSharedPtr<FJsonObject> Layout = ObjectField(Run, TEXT("layout"));
    const FIntPoint Origin = PartyHex();
    if (Origin == Destination || !IsWalkableHex(Destination))
        return Empty;
    TSet<FIntPoint> Blocked;
    if (CurrentPhase == TEXT("battle")) {
        const TSharedPtr<FJsonObject> Battle = ObjectField(State, TEXT("battle"));
        const TArray<TSharedPtr<FJsonValue>>* Actors = nullptr;
        if (Battle.IsValid() && Battle->TryGetArrayField(TEXT("actors"), Actors) &&
            Actors != nullptr) {
            for (const TSharedPtr<FJsonValue>& Value : *Actors) {
                const TSharedPtr<FJsonObject> Actor = Value->AsObject();
                if (NumberField(Actor, TEXT("hp")) > 0 &&
                    StringField(Actor, TEXT("id")) != CurrentActorId(State)) {
                    Blocked.Add(PositionField(Actor, TEXT("pos")));
                }
            }
        }
    }
    int32 Budget = 999;
    if (CurrentPhase == TEXT("battle")) {
        const TSharedPtr<FJsonObject> Battle = ObjectField(State, TEXT("battle"));
        const TArray<TSharedPtr<FJsonValue>>* Actors = nullptr;
        if (Battle.IsValid() && Battle->TryGetArrayField(TEXT("actors"), Actors) &&
            Actors != nullptr) {
            for (const TSharedPtr<FJsonValue>& Value : *Actors) {
                const TSharedPtr<FJsonObject> Actor = Value->AsObject();
                if (StringField(Actor, TEXT("id")) == CurrentActorId(State)) {
                    Budget = NumberField(Actor, TEXT("move_left"));
                    break;
                }
            }
        }
    }
    TArray<FIntPoint> Pending;
    TMap<FIntPoint, FIntPoint> Previous;
    TMap<FIntPoint, int32> Cost;
    Pending.Add(Origin);
    Previous.Add(Origin, Origin);
    Cost.Add(Origin, 0);
    while (Pending.Num() > 0) {
        Pending.Sort([&Cost](const FIntPoint& Left, const FIntPoint& Right) {
            return Cost.FindChecked(Left) < Cost.FindChecked(Right);
        });
        const FIntPoint Current = Pending[0];
        Pending.RemoveAt(0);
        if (Current == Destination)
            break;
        for (const FIntPoint& Next : Neighbors(Current)) {
            if (Blocked.Contains(Next) || !IsWalkableHex(Next))
                continue;
            const int32 StepCost = ContainsPoint(Layout, TEXT("rough"), Next) ? 2 : 1;
            const int32 NewCost = Cost.FindChecked(Current) + StepCost;
            const int32* ExistingCost = Cost.Find(Next);
            if (ExistingCost == nullptr || NewCost < *ExistingCost) {
                Cost.Add(Next, NewCost);
                Previous.Add(Next, Current);
                Pending.Add(Next);
            }
        }
    }
    if (!Previous.Contains(Destination))
        return Empty;
    for (FIntPoint Current = Destination; Current != Origin; Current = Previous[Current]) {
        Empty.Insert(Current, 0);
    }
    if (CurrentPhase == TEXT("battle") && Cost.FindChecked(Destination) > FMath::Max(0, Budget)) {
        return TArray<FIntPoint>();
    }
    return Empty;
}

void AOdrGameMode::SelectActor(const FString& Id) {
    if (Id.IsEmpty()) {
        SelectedTarget.Empty();
        Refresh();
        return;
    }
    const TSharedPtr<FJsonObject> State = Snapshot();
    const TSharedPtr<FJsonObject> Battle = ObjectField(State, TEXT("battle"));
    const TArray<TSharedPtr<FJsonValue>>* Actors = nullptr;
    if (!Battle.IsValid() && State.IsValid() && State->TryGetArrayField(TEXT("party"), Actors)) {
        for (const auto& Value : *Actors) {
            const auto Member = Value->AsObject();
            if (StringField(Member, TEXT("id")) == Id) {
                SelectedTarget = Id;
                LastMessage = FString::Printf(
                    TEXT("%s / %s / Level %d / HP %d of %d / XP %d"),
                    *StringField(Member, TEXT("name")), *StringField(Member, TEXT("class")),
                    NumberField(Member, TEXT("level")), NumberField(Member, TEXT("hp")),
                    NumberField(Member, TEXT("max_hp")), NumberField(Member, TEXT("xp")));
                Refresh();
                return;
            }
        }
    }
    if (!Battle.IsValid() || !Battle->TryGetArrayField(TEXT("actors"), Actors) || Actors == nullptr)
        return;
    for (const TSharedPtr<FJsonValue>& Value : *Actors) {
        const TSharedPtr<FJsonObject> Actor = Value->AsObject();
        if (StringField(Actor, TEXT("id")) == Id && NumberField(Actor, TEXT("hp")) > 0) {
            SelectedTarget = Id;
            Refresh();
            return;
        }
    }
}

void AOdrGameMode::ClickHex(FIntPoint Point) {
    const TSharedPtr<FJsonObject> State = Snapshot();
    if (StringField(State, TEXT("phase")) == TEXT("battle")) {
        const TSharedPtr<FJsonObject> Battle = ObjectField(State, TEXT("battle"));
        const TArray<TSharedPtr<FJsonValue>>* Actors = nullptr;
        if (Battle.IsValid() && Battle->TryGetArrayField(TEXT("actors"), Actors) &&
            Actors != nullptr) {
            for (const TSharedPtr<FJsonValue>& Value : *Actors) {
                const TSharedPtr<FJsonObject> Actor = Value->AsObject();
                if (NumberField(Actor, TEXT("hp")) > 0 &&
                    PositionField(Actor, TEXT("pos")) == Point) {
                    SelectActor(StringField(Actor, TEXT("id")));
                    return;
                }
            }
        }
    }
    CancelNavigation();
    if (StringField(State, TEXT("phase")) == TEXT("battle")) {
        const TArray<FIntPoint> Path = PreviewPath(Point);
        if (Path.Num() == 0) {
            LastMessage = TEXT("That hex is outside the acting unit's movement range.");
            Refresh();
            return;
        }
        Command(PositionCommand(TEXT("move_battle"), Point));
        return;
    }
    const TArray<FIntPoint> Path = PreviewPath(Point);
    if (Path.Num() == 0) {
        LastMessage = TEXT("That hex cannot be reached.");
        Refresh();
        return;
    }
    NavigationQueue = Path;
    bFollowingPath = false;
    if (GetWorld() != nullptr) {
        GetWorld()->GetTimerManager().SetTimer(NavigationTimer, this,
                                               &AOdrGameMode::AdvanceNavigation, 0.14f, true);
    }
}

void AOdrGameMode::AdvanceNavigation() {
    if (NavigationQueue.Num() == 0) {
        CancelNavigation();
        return;
    }
    const FIntPoint Destination = NavigationQueue[0];
    const FString CurrentPhase = Phase();
    if ((CurrentPhase != TEXT("overworld") && CurrentPhase != TEXT("dungeon")) ||
        !IsWalkableHex(Destination) || HexDistance(PartyHex(), Destination) != 1) {
        CancelNavigation();
        return;
    }
    const TCHAR* Action = CurrentPhase == TEXT("overworld") ? TEXT("travel")
                          : CurrentPhase == TEXT("dungeon") ? TEXT("move_dungeon")
                                                            : TEXT("move_battle");
    const FString PhaseBeforeCommand = CurrentPhase;
    bFollowingPath = true;
    const bool bSuccess = Command(PositionCommand(Action, Destination));
    bFollowingPath = false;
    if (!bSuccess || Phase() != PhaseBeforeCommand || NavigationQueue.Num() == 0) {
        CancelNavigation();
        return;
    }
    NavigationQueue.RemoveAt(0);
    if (NavigationQueue.Num() == 0)
        CancelNavigation();
}

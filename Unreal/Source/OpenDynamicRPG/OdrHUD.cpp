#include "OdrHUD.h"
#include "Dom/JsonObject.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "OdrGameMode.h"

namespace {
const FLinearColor Ink(0.025f, 0.039f, 0.051f, 0.96f);
const FLinearColor Brass(0.91f, 0.68f, 0.33f);
const FLinearColor Paper(0.91f, 0.90f, 0.83f);
const FLinearColor Muted(0.57f, 0.65f, 0.67f);
const FLinearColor Teal(0.22f, 0.77f, 0.72f);
const FLinearColor Red(0.96f, 0.39f, 0.30f);
TSharedPtr<FJsonObject> Obj(const TSharedPtr<FJsonObject>& Value, const FString& Key) {
    const TSharedPtr<FJsonObject>* Result = nullptr;
    return Value.IsValid() && Value->TryGetObjectField(Key, Result) ? *Result : nullptr;
}
FString Str(const TSharedPtr<FJsonObject>& Value, const TCHAR* Key) {
    FString Result;
    if (Value.IsValid())
        Value->TryGetStringField(Key, Result);
    return Result;
}
int32 Num(const TSharedPtr<FJsonObject>& Value, const TCHAR* Key) {
    double Result = 0;
    if (Value.IsValid())
        Value->TryGetNumberField(Key, Result);
    return static_cast<int32>(Result);
}
bool Flag(const TSharedPtr<FJsonObject>& Value, const TCHAR* Key) {
    bool Result = false;
    if (Value.IsValid())
        Value->TryGetBoolField(Key, Result);
    return Result;
}
TArray<TSharedPtr<FJsonValue>> Array(const TSharedPtr<FJsonObject>& Value, const TCHAR* Key) {
    const TArray<TSharedPtr<FJsonValue>>* Result = nullptr;
    return Value.IsValid() && Value->TryGetArrayField(Key, Result)
               ? *Result
               : TArray<TSharedPtr<FJsonValue>>();
}
FIntPoint Pos(const TSharedPtr<FJsonObject>& Value, const TCHAR* Key) {
    const auto Values = Array(Value, Key);
    return Values.Num() == 2 ? FIntPoint(Values[0]->AsNumber(), Values[1]->AsNumber())
                             : FIntPoint::ZeroValue;
}
int32 Distance(FIntPoint A, FIntPoint B) {
    const auto D = A - B;
    return (FMath::Abs(D.X) + FMath::Abs(D.Y) + FMath::Abs(D.X + D.Y)) / 2;
}
TSharedPtr<FJsonObject> Actor(const TSharedPtr<FJsonObject>& State, const FString& Id) {
    const auto Battle = Obj(State, TEXT("battle"));
    for (const auto& Value : Array(Battle.IsValid() ? Battle : State,
                                   Battle.IsValid() ? TEXT("actors") : TEXT("party")))
        if (Str(Value->AsObject(), TEXT("id")) == Id)
            return Value->AsObject();
    return nullptr;
}
FString ActingId(const TSharedPtr<FJsonObject>& State) {
    const auto Battle = Obj(State, TEXT("battle"));
    const auto Order = Array(Battle, TEXT("order"));
    const int32 Index = Num(Battle, TEXT("turn_index"));
    return Order.IsValidIndex(Index) ? Order[Index]->AsString() : FString();
}
} // namespace

void AOdrHUD::Rect(float X, float Y, float W, float H, FLinearColor Color) {
    DrawRect(Color, X * Scale, Y * Scale, W * Scale, H * Scale);
}
void AOdrHUD::Text(const FString& Value, float X, float Y, float Size, FLinearColor Color) {
    auto* Font = GEngine->GetMediumFont();
    DrawText(Value, Color, X * Scale, Y * Scale, Font, Size * Scale / Font->GetMaxCharHeight());
}
float AOdrHUD::TextWidth(const FString& Value, float Size) const {
    float W = 0, H = 0;
    auto* Font = GEngine->GetMediumFont();
    GetTextSize(Value, W, H, Font, Size / Font->GetMaxCharHeight());
    return W;
}
float AOdrHUD::Wrapped(const FString& Value, float X, float Y, float W, float Size,
                       FLinearColor Color) {
    TArray<FString> Words;
    Value.ParseIntoArray(Words, TEXT(" "), true);
    FString Line;
    for (const auto& Word : Words) {
        const FString Next = Line.IsEmpty() ? Word : Line + TEXT(" ") + Word;
        if (!Line.IsEmpty() && TextWidth(Next, Size) > W) {
            Text(Line, X, Y, Size, Color);
            Y += Size + 5;
            Line = Word;
        } else
            Line = Next;
    }
    Text(Line, X, Y, Size, Color);
    return Y + Size + 5;
}
void AOdrHUD::Panel(float X, float Y, float W, float H) {
    Panels.Add(FBox2D(FVector2D(X, Y), FVector2D(X + W, Y + H)));
    Rect(X, Y, W, H, Ink);
    Rect(X, Y, W, 2, FLinearColor(Brass.R, Brass.G, Brass.B, 0.7f));
}
void AOdrHUD::Button(const FString& Action, const FString& Label, const FString& Hint, float X,
                     float Y, float W, bool bEnabled, bool bSelected, float H) {
    const FBox2D Bounds(FVector2D(X, Y), FVector2D(X + W, Y + H));
    const bool bHover = Bounds.IsInside(Mouse / Scale);
    Buttons.Add({Bounds, Action, Hint, bEnabled});
    const FLinearColor Fill = !bEnabled             ? FLinearColor(0.062f, 0.073f, 0.08f)
                              : bHover || bSelected ? FLinearColor(0.10f, 0.24f, 0.24f)
                                                    : FLinearColor(0.09f, 0.13f, 0.16f);
    Rect(X, Y, W, H, Fill);
    Rect(X, Y, bSelected ? 3 : W, bSelected ? H : 1, bSelected || bHover ? Brass : Muted * 0.35f);
    Text(Label, X + 12, Y + (H - 17) / 2 - 1, 17, bEnabled ? Paper : Muted * 0.67f);
}
bool AOdrHUD::OverPanel(FVector2D Position) const {
    for (const auto& Bounds : Panels)
        if (Bounds.IsInside(Position))
            return true;
    return false;
}
void AOdrHUD::HexOutline(FIntPoint Hex, FLinearColor Color, float Thickness) {
    const FVector Center = AOdrGameMode::HexToWorld(Hex, 3);
    FVector2D Vertices[6];
    for (int32 Index = 0; Index < 6; ++Index) {
        const float Angle = FMath::DegreesToRadians(30.0f + Index * 60.0f);
        if (!PlayerOwner->ProjectWorldLocationToScreen(
                Center + FVector(FMath::Cos(Angle) * 78, FMath::Sin(Angle) * 78, 0),
                Vertices[Index]))
            return;
    }
    for (int32 Index = 0; Index < 6; ++Index)
        DrawLine(Vertices[Index].X, Vertices[Index].Y, Vertices[(Index + 1) % 6].X,
                 Vertices[(Index + 1) % 6].Y, Color, Thickness * Scale);
}
void AOdrHUD::DrawWorld(AOdrGameMode* Mode) {
    ActorHits.Reset();
    if (!PlayerOwner)
        return;
    const auto State = Mode->ViewState();
    const FString Phase = Str(State, TEXT("phase"));
    if ((Phase == TEXT("dungeon") || Phase == TEXT("battle") || Phase == TEXT("overworld")) &&
        !OverPanel(Mouse / Scale)) {
        FVector Origin, Direction;
        if (PlayerOwner->DeprojectScreenPositionToWorld(Mouse.X, Mouse.Y, Origin, Direction) &&
            Direction.Z < -0.01) {
            const FIntPoint Point =
                AOdrGameMode::WorldToHex(Origin - Direction * Origin.Z / Direction.Z);
            if (Point != HoverHex || HoverState != State) {
                HoverHex = Point;
                HoverState = State;
                HoverPath = Mode->PreviewPath(Point);
            }
            if (Mode->IsWalkableHex(Point)) {
                for (const auto& Step : HoverPath)
                    HexOutline(Step, FLinearColor(0.25f, 0.75f, 0.70f, 0.55f), 1);
                HexOutline(Point, HoverPath.IsEmpty() && Point != Mode->PartyHex() ? Red : Teal);
            }
        }
    }
    for (const auto& Label : Mode->Labels()) {
        if (Label.Text == TEXT("WALL") || Label.Text == TEXT("ROUGH"))
            continue;
        FVector2D Screen;
        if (!PlayerOwner->ProjectWorldLocationToScreen(Label.Position, Screen))
            continue;
        Screen /= Scale;
        const bool bUnit = !Label.ActorId.IsEmpty();
        const float W = FMath::Max(54.0f, TextWidth(Label.Text, 14) + 14);
        if (bUnit) {
            FVector2D Body;
            if (!PlayerOwner->ProjectWorldLocationToScreen(Label.Position - FVector(0, 0, 85),
                                                           Body))
                continue;
            Body /= Scale;
            const FBox2D Hit(Body - FVector2D(22, 24), Body + FVector2D(22, 24));
            ActorHits.Add({Hit, Label.ActorId});
            if (Label.bCurrent || Label.bSelected)
                HexOutline(AOdrGameMode::WorldToHex(Label.Position), Label.bSelected ? Brass : Teal,
                           3);
            const bool bHovered = !OverPanel(Mouse / Scale) && Hit.IsInside(Mouse / Scale);
            // Keep the body visible in a tight deployment. Full names and stats
            // also remain available in the party strip and target inspector.
            if (Label.bCurrent || Label.bSelected || bHovered) {
                Rect(Screen.X - W / 2, Screen.Y - 25, W, 23, Ink);
                Text(Label.Text, Screen.X - W / 2 + 7, Screen.Y - 23, 14,
                     Label.bSelected  ? Brass
                     : Label.bCurrent ? Teal
                                      : Label.Color);
            }
            if (Label.MaxHP > 0) {
                Rect(Screen.X - 17, Screen.Y - 1, 34, 4, Ink);
                Rect(Screen.X - 16, Screen.Y,
                     32 * FMath::Clamp(float(Label.HP) / Label.MaxHP, 0.0f, 1.0f), 2, Label.Color);
            }
            continue;
        }
        Rect(Screen.X - W / 2, Screen.Y - 5, W, 23, Ink);
        Text(Label.Text, Screen.X - W / 2 + 7, Screen.Y - 3, 14,
             Label.bCurrent    ? Teal
             : Label.bSelected ? Brass
                               : Label.Color);
    }
}
void AOdrHUD::DrawParty(AOdrGameMode* Mode, const TSharedPtr<FJsonObject>& State) {
    const auto Active = Array(State, TEXT("active_party"));
    if (Active.IsEmpty())
        return;
    Panel(24, Height - 106, Width - 48, 86);
    const float Cell = FMath::Min(126.0f, (Width - 76) / Active.Num());
    for (int32 Index = 0; Index < Active.Num(); ++Index) {
        const FString Id = Active[Index]->AsString();
        const auto Member = Actor(State, Id);
        if (!Member)
            continue;
        const float X = 38 + Index * Cell;
        const bool bCurrent = Id == ActingId(State);
        const bool bAlive = Num(Member, TEXT("hp")) > 0;
        Button(TEXT("select:") + Id, Str(Member, TEXT("name")),
               Str(Member, TEXT("class")) + TEXT(" / select to inspect or heal"), X, Height - 94,
               Cell - 7, bAlive, bCurrent || Id == Mode->SelectedTargetId(), 30);
        Text(Str(Member, TEXT("class")), X + 7, Height - 60, 13, Muted);
        Text(FString::Printf(TEXT("%d / %d"), Num(Member, TEXT("hp")), Num(Member, TEXT("max_hp"))),
             X + 7, Height - 43, 13, bAlive ? Paper : Red);
        const float Ratio = Num(Member, TEXT("max_hp")) > 0
                                ? float(Num(Member, TEXT("hp"))) / Num(Member, TEXT("max_hp"))
                                : 0;
        Rect(X + 7, Height - 25, Cell - 21, 3, Muted * 0.3f);
        Rect(X + 7, Height - 25, (Cell - 21) * Ratio, 3, bCurrent ? Brass : Teal);
    }
}

void AOdrHUD::DrawHUD() {
    Super::DrawHUD();
    auto* Mode = GetWorld() ? GetWorld()->GetAuthGameMode<AOdrGameMode>() : nullptr;
    if (!Mode || !Canvas || !Mode->ViewState())
        return;
    Scale = FMath::Min(Canvas->SizeX / 1440.0f, Canvas->SizeY / 900.0f);
    Width = Canvas->SizeX / Scale;
    Height = Canvas->SizeY / Scale;
    DrawWorld(Mode);
    Buttons.Reset();
    Panels.Reset();
    const auto State = Mode->ViewState();
    const FString Phase = Str(State, TEXT("phase"));
    const auto Inventory = Obj(State, TEXT("inventory"));
    const auto Run = Obj(Obj(State, TEXT("runs")), Str(State, TEXT("active_run")));
    const auto Changes = Obj(Run, TEXT("changes"));
    const auto Objects = Obj(Obj(Run, TEXT("layout")), TEXT("objects"));
    const auto Party = Array(State, TEXT("party"));
    const FIntPoint Position = Mode->PartyHex();
    const bool bHasRun = Run.IsValid();
    Panel(24, 20, Width - 48, 76);
    Text(TEXT("THE BLACKSMITH'S MINE"), 44, 31, 25, Paper);
    const FString Place = Phase == TEXT("creation")    ? TEXT("Create your adventurer")
                          : Phase == TEXT("city")      ? TEXT("Hearthstead / town services")
                          : Phase == TEXT("overworld") ? TEXT("The old road / regional travel")
                          : Phase == TEXT("battle")    ? TEXT("The old mine / tactical combat")
                                                       : TEXT("The old mine / exploration");
    Text(Place, 45, 65, 15, Brass);
    if (Phase != TEXT("creation"))
        Text(FString::Printf(TEXT("%d gold / %d ore / %d camp supplies"),
                             Num(Inventory, TEXT("gold")), Num(Inventory, TEXT("ore")),
                             Num(Inventory, TEXT("camp_supplies"))),
             Width - 620, 41, 16, Paper);
    Button(TEXT("save"), TEXT("Save"), TEXT("Save this expedition, including an unfinished turn."),
           Width - 225, 39, 85, Phase != TEXT("creation"));
    Button(TEXT("load"), TEXT("Load"), TEXT("Return to your last saved expedition."), Width - 130,
           39, 85);
    Panel(24, 114, 310, Height - (Phase == TEXT("creation") ? 142 : 238));
    float Y = 133;
    Text(Phase == TEXT("creation") ? TEXT("YOUR HERO") : TEXT("EXPEDITION"), 44, Y, 14, Brass);
    Y = Wrapped(Mode->Objective(), 44, Y + 26, 267, 20, Paper) + 13;
    auto Row = [&](const FString& Action, const FString& Label, const FString& Hint,
                   bool Enabled = true, bool Selected = false) {
        Button(Action, Label, Hint, 44, Y, 270, Enabled, Selected);
        Y += 44;
    };
    if (Phase == TEXT("creation")) {
        const TCHAR* Names[] = {TEXT("Warrior"), TEXT("Rogue"),  TEXT("Ranger"),
                                TEXT("Mage"),    TEXT("Cleric"), TEXT("Barbarian")};
        const TCHAR* Descriptions[] = {TEXT("Armored frontline fighter with a reliable blade."),
                                       TEXT("Swift skirmisher. Precise strikes inflict poison."),
                                       TEXT("A bow reaches enemies across the battlefield."),
                                       TEXT("Limited spells strike one enemy or a small area."),
                                       TEXT("Restore a wounded ally with a healing spell."),
                                       TEXT("A durable melee fighter with heavy strikes.")};
        for (int32 Index = 0; Index < 6; ++Index)
            Row(FString::Printf(TEXT("class:%d"), Index), Names[Index], Descriptions[Index], true,
                Mode->SelectedClass() == Names[Index]);
        Y += 5;
        Row(TEXT("ancestry"), TEXT("Ancestry: ") + Mode->SelectedAncestry(),
            TEXT("Cycle Human, Elf, Dwarf, and Orc. Visual ancestry variants are still to come."));
        Row(TEXT("background"), TEXT("Background: ") + Mode->SelectedBackground(),
            TEXT("Cycle Guard, Scholar, and Outlaw."));
        Y += 8;
        Row(TEXT("create"), TEXT("Begin as Ari"),
            TEXT("Create your hero and arrive at the forge."));
        Wrapped(
            TEXT(
                "Recruit companions in town, recover star iron, and return to reforge your blade."),
            44, Y + 8, 263, 15, Muted);
    } else if (Phase == TEXT("city")) {
        Y = Wrapped(TEXT("The forge needs star iron from the old mine. Gather a party before "
                         "taking the road."),
                    44, Y, 266, 15, Muted) +
            10;
        const TCHAR* Recruits[] = {
            TEXT("Rowan / Warrior"), TEXT("Sable / Rogue"), TEXT("Tala / Ranger"),
            TEXT("Iona / Mage"),     TEXT("Bran / Cleric"), TEXT("Kora / Barbarian"),
            TEXT("Eden / Warrior"),  TEXT("Fenn / Ranger"), TEXT("Mira / Cleric"),
            TEXT("Orin / Mage"),     TEXT("Pax / Rogue")};
        const int32 Next = Party.Num() - 1;
        Row(TEXT("recruit"),
            Next >= 0 && Next < 11 ? TEXT("Recruit ") + FString(Recruits[Next])
                                   : TEXT("All companions recruited"),
            TEXT("Add one companion. The first eight join the active expedition."),
            Next >= 0 && Next < 11);
        Button(TEXT("active:8"), TEXT("Party of 8"),
               TEXT("Use the first eight recruited adventurers."), 44, Y, 130, Party.Num() >= 8,
               Array(State, TEXT("active_party")).Num() == 8);
        Button(TEXT("active:12"), TEXT("Party of 12"),
               TEXT("Recruit all eleven companions before testing twelve."), 184, Y, 130,
               Party.Num() == 12, Array(State, TEXT("active_party")).Num() == 12);
        Y += 49;
        Row(TEXT("authored"),
            bHasRun ? TEXT("New authored expedition") : TEXT("Accept the mine expedition"),
            TEXT("Accept a separate authored mine. Previous runs retain their progress."));
        Button(TEXT("generated"), TEXT("New variant"), TEXT("Accept a validated generated layout."),
               44, Y, 130);
        Button(TEXT("boss"), TEXT("Boss variant"),
               TEXT("Accept a mine with a separate lower-floor boss."), 184, Y, 130);
        Y += 49;
        Row(TEXT("forge"),
            Num(Obj(State, TEXT("equipment")), TEXT("tier")) > 0
                ? TEXT("Reforge another blade tier")
                : TEXT("Reforge the blade"),
            TEXT("Bring a new star iron sample to the blacksmith."),
            Num(Inventory, TEXT("ore")) > 0 && !Flag(Changes, TEXT("upgraded")));
        Button(TEXT("rest"), TEXT("Rest"), TEXT("Recover health, wounds, and spells in town."), 44,
               Y, 130);
        const auto Hero = Actor(State, Str(State, TEXT("hero_id")));
        Button(TEXT("level"), TEXT("Train hero"), TEXT("Spend earned experience to gain a level."),
               184, Y, 130,
               Num(Hero, TEXT("xp")) >= Num(Hero, TEXT("level")) * 100 &&
                   Num(Hero, TEXT("level")) < 3);
        Y += 49;
        Row(TEXT("depart"), TEXT("Take the road"), TEXT("Leave town with the active party."),
            bHasRun);
    } else if (Phase == TEXT("overworld")) {
        Y = Wrapped(TEXT("Click the terrain to walk. Use a destination button to follow a route, "
                         "then Enter when the party arrives."),
                    44, Y, 264, 16, Muted) +
            16;
        Row(TEXT("go:mine"), TEXT("Walk to the old mine"),
            TEXT("Follow a legal route to the mine entrance."));
        Row(TEXT("enter_mine"), TEXT("Enter the mine"), TEXT("Stand on the mine entrance first."),
            Position == FIntPoint(3, 1) && bHasRun);
        Row(TEXT("go:city"), TEXT("Walk to Hearthstead"), TEXT("Return along the old road."));
        Row(TEXT("enter_city"), TEXT("Enter town"), TEXT("Stand at the town marker first."),
            Position == FIntPoint(0, 0));
        Row(TEXT("go:ruins"), TEXT("Explore the ruins"),
            TEXT("An optional detour, independent of the mine."));
        Row(TEXT("inspect_ruins"), TEXT("Inspect the ruins"),
            TEXT("Stand at the ruins to investigate them."), Position == FIntPoint(-2, 1));
    } else if (Phase == TEXT("dungeon")) {
        const int32 Floor = Num(State, TEXT("mine_floor"));
        const bool bMainFloor = Floor == (Flag(Run, TEXT("boss")) ? 2 : 1);
        auto Near = [&](const TCHAR* Key) { return Distance(Position, Pos(Objects, Key)) <= 1; };
        Y = Wrapped(FString::Printf(TEXT("Floor %d. Click a hex to walk. A teal path previews your "
                                         "route. Interact when close to a landmark."),
                                    Floor),
                    44, Y, 264, 16, Muted) +
            12;
        Row(TEXT("begin_main"), TEXT("Confront the mine guards"),
            TEXT("Approach the banner marking the main encounter."),
            bMainFloor && Near(TEXT("main_trigger")) && !Flag(Changes, TEXT("main_won")));
        Row(TEXT("take_ore"), TEXT("Recover star iron"),
            TEXT("Clear the main encounter, then approach the blue ore."),
            bMainFloor && Near(TEXT("ore")) && Flag(Changes, TEXT("main_won")) &&
                !Flag(Changes, TEXT("ore_taken")));
        Row(TEXT("search"), TEXT("Search for clues"),
            TEXT("Search near suspicious crates or side passages."), Floor == 1);
        if (Floor == 1 && (Flag(Changes, TEXT("secret_found")) || Near(TEXT("hidden_loot"))))
            Row(TEXT("take_hidden_loot"), TEXT("Open the hidden cache"),
                TEXT("Move next to the discovered chest."),
                Near(TEXT("hidden_loot")) && !Flag(Changes, TEXT("hidden_loot_taken")));
        if (Floor == 1 && Flag(Changes, TEXT("secret_found")))
            Row(TEXT("begin_secret"), TEXT("Enter the hidden encounter"),
                TEXT("Optional fight. Its reward is not needed by the forge."),
                Near(TEXT("secret_trigger")) && !Flag(Changes, TEXT("secret_won")));
        if (Flag(Run, TEXT("boss")))
            Row(TEXT("stairs"),
                Floor == 1 ? TEXT("Descend to the lower mine") : TEXT("Return to the upper mine"),
                TEXT("Move next to the stairs first."),
                Near(Floor == 1 ? TEXT("stairs_down") : TEXT("stairs_up")));
        Row(TEXT("camp"), TEXT("Make camp"), TEXT("Spend one camp supply to recover the party."),
            Num(Inventory, TEXT("camp_supplies")) > 0);
        Row(TEXT("leave_mine"), TEXT("Leave the mine"),
            TEXT("Return to the upper-floor exit banner."), Floor == 1 && Near(TEXT("exit")));
    } else if (Phase == TEXT("battle")) {
        const auto Current = Actor(State, ActingId(State));
        const auto Target = Actor(State, Mode->SelectedTargetId());
        const bool bEnemy = Str(Target, TEXT("team")) == TEXT("enemy");
        const FString Class = Str(Current, TEXT("class"));
        Text(Str(Current, TEXT("name")) + TEXT(" / ") + Class, 44, Y, 20, Teal);
        Y += 32;
        Text(FString::Printf(TEXT("HP %d/%d   Move %d   Spells %d"), Num(Current, TEXT("hp")),
                             Num(Current, TEXT("max_hp")), Num(Current, TEXT("move_left")),
                             Num(Current, TEXT("spell_uses"))),
             44, Y, 16, Paper);
        Y = Wrapped(TEXT("Move, then take one action. Click a character to select a target; click "
                         "an empty hex to move."),
                    44, Y + 30, 265, 15, Muted) +
            12;
        Row(TEXT("attack"), TEXT("Attack selected target"),
            bEnemy ? TEXT("Range and line of sight are checked by the rules.")
                   : TEXT("Select a living enemy first."),
            bEnemy && Num(Target, TEXT("hp")) > 0);
        const bool bHeal = Class == TEXT("Cleric");
        const bool bSpellTarget = bHeal ? Target.IsValid() && !bEnemy &&
                                              Num(Target, TEXT("hp")) < Num(Target, TEXT("max_hp"))
                                        : bEnemy;
        Row(TEXT("spell"), bHeal ? TEXT("Heal selected ally") : TEXT("Cast at selected target"),
            TEXT("Mage damages an enemy. Cleric restores a wounded ally. Costs one spell charge."),
            (bHeal || Class == TEXT("Mage")) && bSpellTarget &&
                Num(Current, TEXT("spell_uses")) > 0);
        Row(TEXT("area"), TEXT("Cast an area spell"),
            TEXT("Mage only. Center a small blast on the selected enemy."),
            Class == TEXT("Mage") && bEnemy && Num(Current, TEXT("spell_uses")) > 0);
        Row(TEXT("defend"), TEXT("Defend / end turn"),
            TEXT("Take a defensive stance and pass initiative."));
        bool bCanRetreat = true;
        const auto Entry = Pos(Obj(State, TEXT("battle")), TEXT("approach"));
        for (const auto& Value : Array(Obj(State, TEXT("battle")), TEXT("actors"))) {
            const auto Member = Value->AsObject();
            if (Str(Member, TEXT("team")) == TEXT("party") && Num(Member, TEXT("hp")) > 0 &&
                Distance(Pos(Member, TEXT("pos")), Entry) > 2)
                bCanRetreat = false;
        }
        Row(TEXT("retreat"), TEXT("Retreat through the entry"),
            TEXT("Every living companion must be within two hexes of the entry banner."),
            bCanRetreat);
        Y += 6;
        Text(TEXT("SELECTED TARGET"), 44, Y, 13, Brass);
        Y += 24;
        if (Target) {
            Y = Wrapped(Str(Target, TEXT("name")), 44, Y, 265, 20, Paper);
            Text(FString::Printf(TEXT("HP %d/%d   Armor %d   Range %d"), Num(Target, TEXT("hp")),
                                 Num(Target, TEXT("max_hp")), Num(Target, TEXT("armor")),
                                 Num(Target, TEXT("range"))),
                 44, Y + 3, 15, Muted);
        } else
            Text(TEXT("Click a unit on the battlefield."), 44, Y, 15, Muted);
        const auto Battle = Obj(State, TEXT("battle"));
        const auto Order = Array(Battle, TEXT("order"));
        const int32 Start = Num(Battle, TEXT("turn_index"));
        Panel(355, 114, Width - 379, 52);
        Text(TEXT("NEXT"), 370, 131, 13, Brass);
        float X = 424;
        for (int32 Offset = 0; Offset < Order.Num(); ++Offset) {
            const FString Id = Order[(Start + Offset) % Order.Num()]->AsString();
            const auto Next = Actor(State, Id);
            if (Num(Next, TEXT("hp")) <= 0)
                continue;
            Button(TEXT("select:") + Id, Str(Next, TEXT("name")).Left(12),
                   TEXT("Inspect this combatant."), X, 122, 122, true, Offset == 0, 34);
            X += 128;
            if (X + 122 > Width - 30)
                break;
        }
    }
    DrawParty(Mode, State);
    if (Phase != TEXT("creation")) {
        FString Message = Mode->LastFeedback();
        if (Message.IsEmpty())
            Message = Mode->Presentation();
        if (!Message.IsEmpty()) {
            const float ToastWidth = FMath::Min(Width - 400, 730.0f);
            Panel(355, Height - 200, ToastWidth, 66);
            Wrapped(Message, 373, Height - 187, ToastWidth - 36, 16, Brass);
        }
        Text(TEXT("Click: move/select / Wheel: zoom / Middle drag: pan / Right drag: rotate / "
                  "Home: recenter"),
             357, Height - 128, 13, Muted);
    }
    for (const auto& Item : Buttons)
        if (Item.Bounds.IsInside(Mouse / Scale) && !Item.Hint.IsEmpty()) {
            const float X = FMath::Clamp(float(Mouse.X / Scale) + 16, 350.0f, Width - 382);
            const float TipY = FMath::Clamp(float(Mouse.Y / Scale) - 76, 180.0f, Height - 210);
            Rect(X, TipY, 358, 72, Ink);
            Wrapped(Item.Hint, X + 12, TipY + 10, 334, 15, Item.bEnabled ? Paper : Muted);
            break;
        }
}

void AOdrHUD::Execute(const FString& Action, AOdrGameMode* Mode) {
    if (Action == TEXT("save"))
        Mode->SaveSession();
    else if (Action == TEXT("load"))
        Mode->LoadSession();
    else if (Action.StartsWith(TEXT("class:")))
        Mode->ChooseClass(FCString::Atoi(*Action.Mid(6)));
    else if (Action == TEXT("ancestry"))
        Mode->CycleAncestry();
    else if (Action == TEXT("background"))
        Mode->CycleBackground();
    else if (Action == TEXT("create"))
        Mode->CreateSelectedHero();
    else if (Action == TEXT("recruit"))
        Mode->RecruitNext();
    else if (Action == TEXT("generated"))
        Mode->CreateGeneratedRun();
    else if (Action == TEXT("boss"))
        Mode->CreateBossRun();
    else if (Action == TEXT("level"))
        Mode->LevelHero();
    else if (Action == TEXT("stairs"))
        Mode->ChangeFloor();
    else if (Action.StartsWith(TEXT("select:")))
        Mode->SelectActor(Action.Mid(7));
    else if (Action == TEXT("go:mine"))
        Mode->ClickHex({3, 1});
    else if (Action == TEXT("go:city"))
        Mode->ClickHex({0, 0});
    else if (Action == TEXT("go:ruins"))
        Mode->ClickHex({-2, 1});
    else if (Action.StartsWith(TEXT("active:"))) {
        const int32 Count = FCString::Atoi(*Action.Mid(7));
        const auto Members = Array(Mode->ViewState(), TEXT("party"));
        FString Command = TEXT("{\"action\":\"set_active\",\"ids\":[");
        for (int32 Index = 0; Index < FMath::Min(Count, Members.Num()); ++Index) {
            if (Index)
                Command += TEXT(",");
            Command += TEXT("\"") + Str(Members[Index]->AsObject(), TEXT("id")) + TEXT("\"");
        }
        Mode->Command(Command + TEXT("]}"));
    } else if (Action == TEXT("attack") || Action == TEXT("spell")) {
        // Mouse actions never substitute another target behind the player's back.
        Mode->Command(FString::Printf(TEXT("{\"action\":\"%s\",\"target\":\"%s\"}"),
                                      Action == TEXT("spell") ? TEXT("cast") : TEXT("attack"),
                                      *Mode->SelectedTargetId()));
    } else if (Action == TEXT("area")) {
        const auto Target = Actor(Mode->ViewState(), Mode->SelectedTargetId());
        if (Target) {
            const auto Point = Pos(Target, TEXT("pos"));
            Mode->Command(FString::Printf(TEXT("{\"action\":\"cast_area\",\"q\":%d,\"r\":%d}"),
                                          Point.X, Point.Y));
        }
    } else {
        const FString Command = Action == TEXT("authored") ? TEXT("create_run")
                                : Action == TEXT("depart") ? TEXT("leave_city")
                                : Action == TEXT("forge")  ? TEXT("upgrade")
                                                           : Action;
        Mode->Command(FString::Printf(TEXT("{\"action\":\"%s\"}"), *Command));
    }
}
bool AOdrHUD::HandleClick(const FVector2D& ScreenPosition) {
    auto* Mode = GetWorld() ? GetWorld()->GetAuthGameMode<AOdrGameMode>() : nullptr;
    if (!Mode)
        return false;
    const FVector2D Position = ScreenPosition / Scale;
    UE_LOG(LogTemp, Verbose, TEXT("ODR pointer raw %.0f,%.0f UI %.0f,%.0f scale %.3f"),
           ScreenPosition.X, ScreenPosition.Y, Position.X, Position.Y, Scale);
    for (const auto& Item : Buttons)
        if (Item.Bounds.IsInside(Position)) {
            if (Item.bEnabled)
                Execute(Item.Action, Mode);
            return true;
        }
    if (OverPanel(Position))
        return true;
    const TPair<FBox2D, FString>* Closest = nullptr;
    double Best = TNumericLimits<double>::Max();
    for (const auto& Hit : ActorHits)
        if (Hit.Key.IsInside(Position)) {
            const double Candidate = FVector2D::DistSquared(Hit.Key.GetCenter(), Position);
            if (Candidate < Best) {
                Closest = &Hit;
                Best = Candidate;
            }
        }
    if (Closest) {
        Mode->SelectActor(Closest->Value);
        return true;
    }
    return false;
}

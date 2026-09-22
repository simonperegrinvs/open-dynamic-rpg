#include "OdrGameMode.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Serialization/JsonSerializer.h"

namespace {
const FString Kit = TEXT("/Game/Art/Diorama/");

TSharedPtr<FJsonObject> Object(const TSharedPtr<FJsonObject>& Value, const TCHAR* Key) {
    const TSharedPtr<FJsonObject>* Result = nullptr;
    return Value.IsValid() && Value->TryGetObjectField(Key, Result) ? *Result : nullptr;
}
FString String(const TSharedPtr<FJsonObject>& Value, const TCHAR* Key) {
    FString Result;
    if (Value.IsValid())
        Value->TryGetStringField(Key, Result);
    return Result;
}
FIntPoint Point(const TSharedPtr<FJsonValue>& Value) {
    const auto& Array = Value->AsArray();
    return FIntPoint(Array[0]->AsNumber(), Array[1]->AsNumber());
}
void Root(AActor* Owner) {
    auto* Component = NewObject<USceneComponent>(Owner);
    Owner->SetRootComponent(Component);
    Owner->AddInstanceComponent(Component);
    Component->RegisterComponent();
}
UMaterialInterface* Material(const FString& Name) {
    return LoadObject<UMaterialInterface>(nullptr, *(Kit + TEXT("MI_") + Name));
}

// Environment instances are owned by presentation alone and have no collision.
// Layout validation and interaction continue to use the portable hex geometry.
void Instance(AActor* Owner, const FString& Name, FVector Position, FVector Scale = FVector(1),
              float Yaw = 0, const FString& Surface = FString()) {
    if (!Owner)
        return;
    const FName Tag(*(Name + Surface));
    UInstancedStaticMeshComponent* Component = nullptr;
    const auto Candidates =
        Owner->GetComponentsByTag(UInstancedStaticMeshComponent::StaticClass(), Tag);
    if (!Candidates.IsEmpty())
        Component = Cast<UInstancedStaticMeshComponent>(Candidates[0]);
    if (!Component) {
        auto* Asset = LoadObject<UStaticMesh>(nullptr, *(Kit + Name));
        if (!Asset)
            return;
        Component = NewObject<UInstancedStaticMeshComponent>(Owner);
        Owner->AddInstanceComponent(Component);
        Component->SetupAttachment(Owner->GetRootComponent());
        Component->SetStaticMesh(Asset);
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->ComponentTags.Add(Tag);
        if (!Surface.IsEmpty()) {
            for (int32 Index = 0; Index < Component->GetNumMaterials(); ++Index)
                Component->SetMaterial(Index, Material(Surface));
        }
        Component->RegisterComponent();
    }
    Component->AddInstance(FTransform(FRotator(0, Yaw, 0), Position, Scale), true);
}

void Lamp(AActor* Owner, FVector Position, FLinearColor Color, float Intensity = 6500) {
    auto* Light = NewObject<UPointLightComponent>(Owner);
    Owner->AddInstanceComponent(Light);
    Light->SetupAttachment(Owner->GetRootComponent());
    Light->SetWorldLocation(Position);
    Light->SetIntensity(Intensity);
    Light->SetLightColor(Color);
    Light->SetAttenuationRadius(410);
    Light->SetCastShadows(false);
    Light->RegisterComponent();
}

FLinearColor ClothColor(const FString& Id) {
    if (Id.Contains(TEXT("enemy")) || Id.Contains(TEXT("boss")))
        return {0.48f, 0.08f, 0.055f};
    if (Id.Contains(TEXT("Rogue")))
        return {0.28f, 0.17f, 0.38f};
    if (Id.Contains(TEXT("Ranger")))
        return {0.14f, 0.34f, 0.16f};
    if (Id.Contains(TEXT("Mage")))
        return {0.15f, 0.21f, 0.55f};
    if (Id.Contains(TEXT("Cleric")))
        return {0.7f, 0.58f, 0.31f};
    if (Id.Contains(TEXT("Barbarian")))
        return {0.5f, 0.25f, 0.06f};
    return {0.045f, 0.37f, 0.42f};
}
} // namespace

FVector AOdrGameMode::HexToWorld(FIntPoint Point, float Height) {
    return FVector(145.0 * (Point.X + 0.5 * Point.Y), 126.0 * Point.Y, Height);
}

FIntPoint AOdrGameMode::WorldToHex(FVector Position) {
    const double R = Position.Y / 126.0;
    const double Q = Position.X / 145.0 - R * 0.5;
    const double S = -Q - R;
    int32 IQ = FMath::RoundToInt(Q), IR = FMath::RoundToInt(R), IS = FMath::RoundToInt(S);
    const double DQ = FMath::Abs(IQ - Q), DR = FMath::Abs(IR - R), DS = FMath::Abs(IS - S);
    if (DQ > DR && DQ > DS)
        IQ = -IR - IS;
    else if (DR > DS)
        IR = -IQ - IS;
    return {IQ, IR};
}

void AOdrGameMode::AdjustZoom(float Delta) {
    if (!SceneCamera)
        return;
    auto* Camera = SceneCamera->GetCameraComponent();
    Camera->SetOrthoWidth(FMath::Clamp(Camera->OrthoWidth + Delta, 1100.0f, 4500.0f));
}

void AOdrGameMode::FocusCamera() {
    UpdateCamera(CachedState, true);
}

void AOdrGameMode::UpdateCamera(const TSharedPtr<FJsonObject>& State, bool bForce) {
    if (!SceneCamera || !State.IsValid())
        return;
    const FString PhaseName = String(State, TEXT("phase"));
    if (!bForce && CameraPhase == PhaseName)
        return;
    CameraPhase = PhaseName;
    FVector Focus;
    float Width;
    if (PhaseName == TEXT("creation") || PhaseName == TEXT("city")) {
        Focus = FVector(160, 120, 50);
        Width = 1950;
    } else if (PhaseName == TEXT("overworld")) {
        Focus = FVector(20, 0, 20);
        Width = 2150;
    } else {
        Focus = FVector(1160, 0, 0);
        Width = 3100;
    }
    const FVector Offset(-280, -1850, 2300);
    const FRotator Rotation = (-Offset).Rotation();
    // Center the diorama in the area beside the left action panel.
    Focus -= FRotationMatrix(Rotation).GetUnitAxis(EAxis::Y) * Width * 0.125;
    SceneCamera->SetActorLocation(Focus + Offset);
    SceneCamera->SetActorRotation(Rotation);
    SceneCamera->GetCameraComponent()->SetOrthoWidth(Width);
}

void AOdrGameMode::BuildEnvironment(const TSharedPtr<FJsonObject>& State) {
    FString PhaseName = String(State, TEXT("phase"));
    if (PhaseName == TEXT("creation"))
        PhaseName = TEXT("city");
    if (PhaseName == TEXT("battle"))
        PhaseName = TEXT("dungeon");
    const FString RunId = String(State, TEXT("active_run"));
    FString LayoutKey;
    const auto ActiveLayout = Object(Object(Object(State, TEXT("runs")), *RunId), TEXT("layout"));
    if (PhaseName == TEXT("dungeon") && ActiveLayout.IsValid())
        FJsonSerializer::Serialize(ActiveLayout.ToSharedRef(),
                                   TJsonWriterFactory<>::Create(&LayoutKey));
    const FString Key = PhaseName + RunId + LayoutKey;
    if (Key == EnvironmentKey && IsValid(Environment))
        return;
    if (IsValid(Environment))
        Environment->Destroy();
    EnvironmentKey = Key;
    Environment = GetWorld()->SpawnActor<AActor>();
    if (!Environment)
        return;
    Root(Environment);

    // A continuous dark plinth gives the diorama a readable silhouette.
    auto* Base = NewObject<UStaticMeshComponent>(Environment);
    Environment->AddInstanceComponent(Base);
    Base->SetupAttachment(Environment->GetRootComponent());
    Base->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
    Base->SetWorldLocation(FVector(1000, 0, -210));
    Base->SetWorldScale3D(FVector(110, 85, 3.4));
    Base->SetMaterial(0, Material(TEXT("Coal")));
    Base->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Base->RegisterComponent();

    if (PhaseName == TEXT("city")) {
        for (int32 Q = -5; Q <= 7; ++Q)
            for (int32 R = -4; R <= 4; ++R) {
                Instance(Environment, TEXT("SM_HexTile"), HexToWorld({Q, R}), FVector(1), 0,
                         FMath::Abs(R) <= 2 ? TEXT("Road") : TEXT("Grass"));
            }
        Instance(Environment, TEXT("SM_House"), FVector(-300, 370, 0));
        Instance(Environment, TEXT("SM_House"), FVector(220, 440, 0), FVector(0.9), -12);
        Instance(Environment, TEXT("SM_House"), FVector(700, 490, 0), FVector(0.8), 4);
        Instance(Environment, TEXT("SM_Timber"), FVector(-260, 155, 0), FVector(1.6));
        for (int32 Index = 0; Index < 10; ++Index)
            Instance(Environment, TEXT("SM_Tree"), FVector(-770 + Index * 190, 680, -5),
                     FVector(1.0 + (Index % 3) * 0.15));
        for (int32 Index = 0; Index < 4; ++Index) {
            Instance(Environment, TEXT("SM_Barrel"), FVector(-460 + Index * 75, 180, 0));
            Instance(Environment, TEXT("SM_Crate"), FVector(800 + Index * 30, 150 + Index * 48, 0));
        }
        Instance(Environment, TEXT("SM_Banner"), FVector(370, 150, 0), FVector(1.4));
        Lamp(Environment, FVector(-230, 230, 110), FLinearColor(1, 0.32f, 0.06f), 12000);
    } else if (PhaseName == TEXT("overworld")) {
        for (int32 Q = -5; Q <= 5; ++Q)
            for (int32 R = -5; R <= 5; ++R) {
                if ((FMath::Abs(Q) + FMath::Abs(R) + FMath::Abs(Q + R)) / 2 > 5)
                    continue;
                const bool bRoad = R == 0 || (Q == 3 && R == 1) || (Q == -2 && R == 1);
                Instance(Environment, TEXT("SM_HexTile"), HexToWorld({Q, R}), FVector(1), 0,
                         bRoad ? TEXT("Road") : TEXT("Grass"));
                // Off-road remains traversable; foliage is small enough to see the tile.
                if (!bRoad && (Q * 7 + R * 3) % 4 == 0)
                    Instance(Environment, TEXT("SM_Tree"), HexToWorld({Q, R}) + FVector(38, 32, 0),
                             FVector(0.42), Q * 23);
            }
        for (int32 Index = 0; Index < 9; ++Index)
            Instance(Environment, TEXT("SM_RockCluster"),
                     FVector(430 + Index * 53, 250 + (Index % 3) * 60, 0),
                     FVector(1.0, 0.8, 1.4 + (Index % 3) * 0.3));
    } else if (PhaseName == TEXT("dungeon")) {
        const auto Runs = Object(State, TEXT("runs"));
        const auto Layout = Object(Object(Runs, *RunId), TEXT("layout"));
        if (!Layout)
            return;
        TSet<FIntPoint> Tiles;
        for (const auto& Value : Layout->GetArrayField(TEXT("tiles"))) {
            const auto Hex = Point(Value);
            Tiles.Add(Hex);
            Instance(Environment, TEXT("SM_HexTile"), HexToWorld(Hex), FVector(1), 0,
                     (Hex.X + Hex.Y * 3) % 7 == 0 ? TEXT("StoneLight") : TEXT("Stone"));
        }
        TSet<FIntPoint> Rim;
        const FIntPoint Deltas[] = {{1, 0}, {1, -1}, {0, -1}, {-1, 0}, {-1, 1}, {0, 1}};
        for (const auto& Hex : Tiles)
            for (const auto& Delta : Deltas) {
                if (!Tiles.Contains(Hex + Delta))
                    Rim.Add(Hex + Delta);
            }
        for (const auto& Hex : Rim) {
            const float Height = Hex.Y < 0 ? 0.55f : 1.3f;
            Instance(Environment, TEXT("SM_RockCluster"), HexToWorld(Hex, -20),
                     FVector(1.45, 1.4, Height), (Hex.X * 57 + Hex.Y * 13) % 360);
        }
        for (const auto& Value : Layout->GetArrayField(TEXT("walls")))
            Instance(Environment, TEXT("SM_RockCluster"), HexToWorld(Point(Value)), FVector(1));
        for (const auto& Value : Layout->GetArrayField(TEXT("rough"))) {
            const FVector Center = HexToWorld(Point(Value));
            for (int32 Index = 0; Index < 3; ++Index)
                Instance(Environment, TEXT("SM_RockCluster"),
                         Center + FVector((Index - 1) * 29, Index % 2 * 30, 0),
                         FVector(0.32, 0.34, 0.17), Index * 75);
        }
        // Supports and storage stay on the perimeter, so they cannot imply false cover.
        for (int32 Q = 1; Q <= 16; Q += 3) {
            const FVector Edge = HexToWorld({Q, 6});
            Instance(Environment, TEXT("SM_Timber"), Edge + FVector(0, 110, 0));
            Instance(Environment, TEXT("SM_Torch"), Edge + FVector(-80, 40, 0));
            Lamp(Environment, Edge + FVector(-80, 40, 140), FLinearColor(1, 0.43f, 0.12f));
            Instance(Environment, TEXT("SM_Crate"), Edge + FVector(45, 40, 0), FVector(0.7), 14);
        }
        Instance(Environment, TEXT("SM_Timber"), HexToWorld({0, 0}) + FVector(-90, 0, 0),
                 FVector(1.3), 90);
        Instance(Environment, TEXT("SM_Crystal"), HexToWorld({15, 6}), FVector(1.1));
        Lamp(Environment, HexToWorld({15, 6}, 100), FLinearColor(0.08f, 0.55f, 1));
    }
}

void AOdrGameMode::SpawnMarker(const FString& VisualId, const FString& Label, int32 Q, int32 R,
                               float Height) {
    auto* Marker = GetWorld()->SpawnActor<AActor>();
    if (!Marker)
        return;
    SceneActors.Add(Marker);
    Root(Marker);
    FVector Location = HexToWorld({Q, R}, 0);
    const FString CurrentPhase = String(CachedState, TEXT("phase"));
    FString Prop;
    FVector Scale(1);
    if (VisualId == TEXT("placeholder.blacksmith")) {
        Prop = TEXT("SM_Forge");
        Location = FVector(-220, 110, 0);
    } else if (VisualId == TEXT("placeholder.city")) {
        Prop = TEXT("SM_Gate");
        if (CurrentPhase == TEXT("city") || CurrentPhase == TEXT("creation"))
            Location = FVector(530, 130, 0);
        else
            Scale = FVector(0.32);
    } else if (VisualId == TEXT("placeholder.mine")) {
        Prop = TEXT("SM_Timber");
        Scale = FVector(0.65);
    } else if (VisualId == TEXT("placeholder.ruins")) {
        Prop = TEXT("SM_Ruins");
        Scale = FVector(0.65);
    } else if (VisualId == TEXT("placeholder.objective")) {
        Prop = Label == TEXT("Ore")      ? TEXT("SM_Crystal")
               : Label == TEXT("Cache")  ? TEXT("SM_Chest")
               : Label == TEXT("Clue")   ? TEXT("SM_Crate")
               : Label == TEXT("Stairs") ? TEXT("SM_Stairs")
                                         : TEXT("SM_Banner");
        Scale = FVector(0.8);
    }
    if (!Prop.IsEmpty()) {
        Instance(Marker, Prop, Location, Scale);
    } else {
        Marker->SetActorLocation(Location);
        const auto Binding = Object(Visuals, *VisualId);
        const FString ModelPath = String(Binding, TEXT("model"));
        UMeshComponent* Mesh = nullptr;
        if (String(Binding, TEXT("mesh_type")) == TEXT("skeletal") && !ModelPath.IsEmpty()) {
            if (auto* Asset = LoadObject<USkeletalMesh>(nullptr, *ModelPath)) {
                auto* Skeletal = NewObject<USkeletalMeshComponent>(Marker);
                Skeletal->SetSkeletalMeshAsset(Asset);
                Mesh = Skeletal;
                Marker->AddInstanceComponent(Mesh);
                Mesh->SetupAttachment(Marker->GetRootComponent());
                Mesh->RegisterComponent();
                const FString AnimationClass = String(Binding, TEXT("animation_class"));
                const FString AnimationPath = String(Binding, TEXT("animation"));
                if (!AnimationClass.IsEmpty()) {
                    if (auto* Class = LoadClass<UAnimInstance>(nullptr, *AnimationClass))
                        Skeletal->SetAnimInstanceClass(Class);
                } else if (!AnimationPath.IsEmpty()) {
                    if (auto* Idle = LoadObject<UAnimSequence>(nullptr, *AnimationPath))
                        Skeletal->PlayAnimation(Idle, true);
                }
                FString TintId = VisualId;
                if (VisualId == TEXT("placeholder.hero")) {
                    FString HeroClass = SelectedClass();
                    const TArray<TSharedPtr<FJsonValue>>* Party = nullptr;
                    if (CachedState.IsValid() &&
                        CachedState->TryGetArrayField(TEXT("party"), Party))
                        for (const auto& Value : *Party)
                            if (String(Value->AsObject(), TEXT("id")) ==
                                String(CachedState, TEXT("hero_id")))
                                HeroClass = String(Value->AsObject(), TEXT("class"));
                    TintId = TEXT("placeholder.") + HeroClass;
                }
                const FLinearColor Tint = ClothColor(TintId);
                for (int32 Index = 0; Index < Asset->GetMaterials().Num(); ++Index) {
                    if (Asset->GetMaterials()[Index].MaterialSlotName.ToString().Contains(
                            TEXT("Cloth"))) {
                        auto* Dynamic = Mesh->CreateDynamicMaterialInstance(Index);
                        if (Dynamic)
                            Dynamic->SetVectorParameterValue(TEXT("Tint"), Tint);
                    }
                }
            }
        }
        if (!Mesh) {
            auto* Static = NewObject<UStaticMeshComponent>(Marker);
            FString Path =
                String(Binding, TEXT("mesh_type")) == TEXT("skeletal") ? FString() : ModelPath;
            if (Path.IsEmpty())
                Path = String(Binding, TEXT("fallback_mesh"));
            auto* Asset = Path.IsEmpty() ? nullptr : LoadObject<UStaticMesh>(nullptr, *Path);
            if (!Asset)
                Asset =
                    LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Capsule.Capsule"));
            if (!Asset)
                Asset = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
            Static->SetStaticMesh(Asset);
            Static->SetMaterial(0, Material(TEXT("Cloth")));
            Marker->AddInstanceComponent(Static);
            Static->SetupAttachment(Marker->GetRootComponent());
            Static->SetRelativeLocation(FVector(0, 0, 60));
            Static->SetRelativeScale3D(FVector(0.6, 0.6, 1.2));
            Static->RegisterComponent();
            Mesh = Static;
        }
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        const FString MaterialPath = String(Binding, TEXT("material"));
        if (!MaterialPath.IsEmpty()) {
            if (auto* Override = LoadObject<UMaterialInterface>(nullptr, *MaterialPath))
                Mesh->SetMaterial(0, Override);
        }
        if (VisualId.Contains(TEXT("boss")))
            Marker->SetActorScale3D(FVector(1.25));
        if (CurrentPhase == TEXT("overworld"))
            Marker->SetActorScale3D(FVector(0.8));
    }
    if (!Label.IsEmpty()) {
        const bool bEnemy = VisualId.Contains(TEXT("enemy")) || VisualId.Contains(TEXT("boss"));
        SceneLabels.Add(
            {Location + FVector(0, 0, Prop.IsEmpty() ? 150 : 180 * Scale.Z), Label,
             bEnemy ? FLinearColor(1, 0.35f, 0.25f) : FLinearColor(0.94f, 0.82f, 0.54f)});
    }
}

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "OdrGameMode.generated.h"

struct OdrSession;
class ACameraActor;
class FJsonObject;

struct FOdrSceneLabel {
    FVector Position;
    FString Text;
    FLinearColor Color;
};

UCLASS()
class AOdrGameMode : public AGameModeBase {
    GENERATED_BODY()

  public:
    AOdrGameMode();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    bool Command(const FString& JsonCommand);
    void SaveSession();
    void LoadSession();
    void ChooseClass(int32 Index);
    void CycleAncestry();
    void CycleBackground();
    void RecruitNext();
    void CreateSelectedHero();
    void Step(int32 Q, int32 R);
    void CreateGeneratedRun();
    void CreateBossRun();
    void ChangeFloor();
    void SetAllActive();
    void LevelHero();
    void ActOnNearest(bool bCast, bool bArea = false);
    void CycleTarget();
    void CycleEnemyTarget();
    void CycleHealingTarget();
    void ActOnSelected(bool bCast, bool bArea = false);
    FString Phase() const;
    FString Help() const;
    FString Status() const;
    FString Presentation() const;
    FString CanonicalState() const;
    FString SelectedTargetId() const {
        return SelectedTarget;
    }
    void RebuildPresentationForAutomation(const FString& Bindings = FString());
    int32 SkeletalMarkerCountForAutomation() const;
    void SetAutomationSavePath(const FString& Path) {
        AutomationSavePath = Path;
    }
    ACameraActor* Camera() const {
        return SceneCamera;
    }
    const TArray<FOdrSceneLabel>& Labels() const {
        return SceneLabels;
    }

    UFUNCTION(Exec)
    void OdrSmoke();

  private:
    void Refresh();
    void SpawnMarker(const FString& VisualId, const FString& Label, int32 Q, int32 R, float Height);
    TSharedPtr<FJsonObject> Snapshot() const;
    FString SelectedClass() const;
    FString SelectedAncestry() const;
    FString SelectedBackground() const;
    FString CurrentActorId(const TSharedPtr<FJsonObject>& State) const;
    bool IsUsableTarget(const TSharedPtr<FJsonObject>& Acting,
                        const TSharedPtr<FJsonObject>& Candidate, bool bCast, bool bArea) const;
    bool SelectTargetForAction(bool bCast, bool bArea, bool bCycle);

    OdrSession* Session = nullptr;
    TSharedPtr<FJsonObject> Visuals;
    UPROPERTY()
    TArray<TObjectPtr<AActor>> SceneActors;
    TArray<FOdrSceneLabel> SceneLabels;
    UPROPERTY()
    TObjectPtr<ACameraActor> SceneCamera;
    FString LastMessage;
    FString PresentationText;
    float PresentationUntil = 0.0f;
    int32 ClassIndex = 0;
    int32 AncestryIndex = 0;
    int32 BackgroundIndex = 0;
    FString SelectedTarget;
    FString AutomationSavePath;
};

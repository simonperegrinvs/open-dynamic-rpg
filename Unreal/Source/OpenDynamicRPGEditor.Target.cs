using UnrealBuildTool;

public class OpenDynamicRPGEditorTarget : TargetRules
{
    public OpenDynamicRPGEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("OpenDynamicRPG");
    }
}

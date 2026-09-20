using UnrealBuildTool;

public class OpenDynamicRPGTarget : TargetRules
{
    public OpenDynamicRPGTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("OpenDynamicRPG");
    }
}

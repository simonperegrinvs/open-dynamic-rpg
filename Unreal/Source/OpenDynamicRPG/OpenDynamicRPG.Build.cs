using UnrealBuildTool;

public class OpenDynamicRPG : ModuleRules
{
    public OpenDynamicRPG(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "InputCore", "Json", "OdrCore", "Slate", "SlateCore" });
    }
}

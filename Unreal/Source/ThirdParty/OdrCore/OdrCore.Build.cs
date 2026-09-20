using System.IO;
using UnrealBuildTool;

public class OdrCore : ModuleRules
{
    public OdrCore(ReadOnlyTargetRules Target) : base(Target)
    {
        Type = ModuleType.External;
        string Root = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../../.."));
        PublicIncludePaths.Add(Path.Combine(Root, "core", "include"));
        string Library = Path.Combine(Root, "build", "ue-release", "libodr_core.dylib");
        PublicAdditionalLibraries.Add(Library);
        RuntimeDependencies.Add("$(TargetOutputDir)/libodr_core.dylib", Library);
    }
}

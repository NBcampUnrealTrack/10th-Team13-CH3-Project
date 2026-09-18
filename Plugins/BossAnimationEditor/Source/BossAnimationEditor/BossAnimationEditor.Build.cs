using UnrealBuildTool;
public class BossAnimationEditor : ModuleRules
{
    public BossAnimationEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "UnrealEd", "AnimGraph", "AnimGraphRuntime", "BlueprintGraph", "KismetCompiler", "VeilBreak" });
    }
}

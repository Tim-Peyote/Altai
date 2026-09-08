using UnrealBuildTool;
public class AltaiCore: ModuleRules { public AltaiCore(ReadOnlyTargetRules Target):base(Target) { PCHUsage=PCHUsageMode.UseExplicitOrSharedPCHs; PublicDependencyModuleNames.AddRange(new string[] { "Core","CoreUObject","Engine" }); } }

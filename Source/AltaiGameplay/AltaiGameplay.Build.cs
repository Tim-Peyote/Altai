using UnrealBuildTool;
public class AltaiGameplay: ModuleRules { public AltaiGameplay(ReadOnlyTargetRules Target):base(Target) { PCHUsage=PCHUsageMode.UseExplicitOrSharedPCHs; PublicDependencyModuleNames.AddRange(new string[] { "Core","CoreUObject","Engine","AltaiCore","EnhancedInput","InputCore" }); } }

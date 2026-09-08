using UnrealBuildTool;
public class Altai: ModuleRules { public Altai(ReadOnlyTargetRules Target):base(Target) { PCHUsage=PCHUsageMode.UseExplicitOrSharedPCHs; PublicDependencyModuleNames.AddRange(new string[] { "Core","CoreUObject","Engine","AltaiCore","AltaiGameplay","AltaiPresentation" }); } }

using UnrealBuildTool;
public class AltaiPresentation: ModuleRules { public AltaiPresentation(ReadOnlyTargetRules Target):base(Target) { PCHUsage=PCHUsageMode.UseExplicitOrSharedPCHs; PublicDependencyModuleNames.AddRange(new string[] { "Core","CoreUObject","Engine","AltaiCore","UMG","Slate","SlateCore","InputCore" }); } }

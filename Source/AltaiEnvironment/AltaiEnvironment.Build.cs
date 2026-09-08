using UnrealBuildTool;
public class AltaiEnvironment : ModuleRules
{
 public AltaiEnvironment(ReadOnlyTargetRules Target) : base(Target)
 {
  PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
  PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "InputCore", "PhysicsCore", "Niagara", "AltaiPresentation", "AltaiGameplay", "UMG", "Slate", "SlateCore", "AnimGraphRuntime", "AnimationCore" });
 }
}

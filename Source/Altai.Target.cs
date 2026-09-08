using UnrealBuildTool;
public class AltaiTarget: TargetRules { public AltaiTarget(TargetInfo Target):base(Target) { Type=TargetType.Game; DefaultBuildSettings=BuildSettingsVersion.V7; IncludeOrderVersion=EngineIncludeOrderVersion.Latest; ExtraModuleNames.Add("Altai"); } }

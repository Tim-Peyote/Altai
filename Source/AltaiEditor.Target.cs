using UnrealBuildTool;
public class AltaiEditorTarget: TargetRules { public AltaiEditorTarget(TargetInfo Target):base(Target) { Type=TargetType.Editor; DefaultBuildSettings=BuildSettingsVersion.V7; IncludeOrderVersion=EngineIncludeOrderVersion.Latest; ExtraModuleNames.Add("Altai"); ExtraModuleNames.Add("AltaiEditor"); } }

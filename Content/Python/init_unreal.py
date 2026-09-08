# Editor-only, registered tooling. The game does not execute this module.
import unreal
from altai_ui_tools import AltaiUITools
unreal.ToolsetRegistry.register_toolset_class(AltaiUITools)
from altai_lab_tools import AltaiLabTools
unreal.ToolsetRegistry.register_toolset_class(AltaiLabTools)

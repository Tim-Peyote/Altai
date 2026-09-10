"""PIE smoke review of lab inventory and exclusive UI/input ownership."""
import unreal,json
from pathlib import Path
from altai_lab_tools import context
w,pc=context();checks={}
checks['lab_uses_shared_screens']=isinstance(pc,unreal.AltaiPlayerController)
initial_screen=str(pc.current_screen)
pc.go_back()
checks['starts_in_game']=pc.current_screen==unreal.AltaiScreenKind.NONE
pc.toggle_inventory()
checks['inventory_created']=pc.current_screen==unreal.AltaiScreenKind.INVENTORY
checks['paused_for_inventory']=unreal.GameplayStatics.is_game_paused(w)
checks['movement_blocked']=pc.is_move_input_ignored()
pc.toggle_developer_panel()
checks['no_overlapping_developer_panel']=pc.developer_panel is None
pc.go_back()
checks['movement_restored']=not pc.is_move_input_ignored() and not unreal.GameplayStatics.is_game_paused(w)
pc.toggle_developer_panel()
pc.toggle_inventory()
checks['no_overlapping_inventory']=pc.current_screen==unreal.AltaiScreenKind.NONE
pc.close_developer_panel()
checks['developer_restores_input']=not pc.is_move_input_ignored()
Path(unreal.Paths.project_saved_dir(),'lab_inventory_review.json').write_text(json.dumps({'passed':all(checks.values()),'initial_screen':initial_screen,'checks':checks},indent=2))
# Leave inventory open for keyboard and visual review.
pc.toggle_inventory()

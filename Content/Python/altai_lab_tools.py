"""Bounded lab-only inspection and repeatable weather/surface probes. Editor-only."""
import unreal,json,time
import toolset_registry

def context():
 w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
 if not w or 'L_CharacterLab' not in w.get_path_name():raise ValueError('Run L_CharacterLab in PIE first')
 pc=unreal.GameplayStatics.get_player_controller(w,0)
 if not isinstance(pc,unreal.AltaiLabController):raise ValueError('Expected lab-only controller')
 return w,pc

@unreal.uclass()
class AltaiLabTools(unreal.ToolsetDefinition):
 """Inspect and exercise only the natural character lab; cannot write profiles."""
 @toolset_registry.tool_call
 @staticmethod
 def inspect_lab() -> str:
  """Read lab pawn, surface response, weather state and bounded footprint count."""
  w,pc=context();p=unreal.GameplayStatics.get_player_pawn(w,0);s=pc.get_editor_property('surface_response');weather=pc.get_editor_property('weather')
  result={'pawn':p.get_path_name() if p else None,'position':str(p.get_actor_location()) if p else None,'weather':{},'surface':{}}
  if weather:
   for key in ['hour','preset_index','rain','snow','mist','wetness','snow_cover','cycle_time','automatic_weather']:result['weather'][key]=weather.get_editor_property(key)
  if s:
   for key in ['current_surface','speed_scale','step_count','stumble_count']:result['surface'][key]=str(s.get_editor_property(key))
  result['decals']=len([o for o in unreal.ObjectIterator(unreal.DecalComponent) if w.get_path_name().split('.')[0] in o.get_path_name()])
  if p:
   result['max_speed']=p.character_movement.max_walk_speed
   result['first_person']=p.get_editor_property('first_person')
   result['camera']=str(pc.player_camera_manager.get_camera_location())
   t=pc.get_editor_property('traversal')
   if t:result['traversal']={k:str(t.get_editor_property(k)) for k in ['climbing','can_climb','completed_climbs','ledge_point','hint']}
   result['movement_mode']=str(p.character_movement.movement_mode)
  return json.dumps(result)
 @toolset_registry.tool_call
 @staticmethod
 def set_lab_weather(preset: int, hour: float) -> str:
  """Set a repeatable weather preset and time in the lab PIE world only."""
  w,pc=context();rig=pc.get_editor_property('weather');rig.set_editor_property('automatic_weather',False);rig.set_editor_property('cycle_time',False);rig.select_preset(preset,True);rig.set_hour(hour)
  return AltaiLabTools.inspect_lab()
 @toolset_registry.tool_call
 @staticmethod
 def place_lab_player(x: float,y: float,z: float,yaw: float) -> str:
  """Teleport the lab pawn for surface probes; never opens or modifies saves."""
  w,pc=context();p=unreal.GameplayStatics.get_player_pawn(w,0);p.character_movement.stop_movement_immediately();p.set_actor_location(unreal.Vector(x,y,z),False,True);pc.set_control_rotation(unreal.Rotator(pitch=-15,yaw=yaw,roll=0))
  return AltaiLabTools.inspect_lab()
 @toolset_registry.tool_call
 @staticmethod
 def move_lab_player(seconds: float) -> str:
  """Apply brief real movement input on ticks in the lab, bounded to 0.2–4 seconds."""
  w,pc=context();p=unreal.GameplayStatics.get_player_pawn(w,0);duration=max(.2,min(seconds,4));deadline=time.monotonic()+duration;handle=[None]
  def tick(dt):
   if not unreal.SystemLibrary.is_valid(p) or time.monotonic()>=deadline:
    unreal.unregister_slate_post_tick_callback(handle[0]);return
   p.add_movement_input(unreal.MathLibrary.get_forward_vector(unreal.Rotator(pitch=0,yaw=pc.get_control_rotation().yaw,roll=0)),1,False)
  handle[0]=unreal.register_slate_post_tick_callback(tick)
  return json.dumps({'moving_seconds':duration})

 @toolset_registry.tool_call
 @staticmethod
 def set_lab_automation(clock: bool, weather: bool) -> str:
  """Toggle only lab time and weather automation for a repeatable smoke check."""
  w,pc=context();r=pc.get_editor_property('weather');r.set_editor_property('cycle_time',clock);r.set_editor_property('automatic_weather',weather)
  return AltaiLabTools.inspect_lab()
 @toolset_registry.tool_call
 @staticmethod
 def reset_lab() -> str:
  """Reload the current lab PIE map, restoring loose props and weather defaults."""
  w,pc=context();unreal.GameplayStatics.open_level(w,'L_CharacterLab');return 'Lab reset requested'

 @toolset_registry.tool_call
 @staticmethod
 def view_lab(first_person: bool,pitch: float,yaw: float) -> str:
  """Switch the lab camera and aim it for visual checks."""
  w,pc=context();p=unreal.GameplayStatics.get_player_pawn(w,0);p.set_first_person(first_person);pc.set_control_rotation(unreal.Rotator(pitch=max(-85,min(85,pitch)),yaw=yaw,roll=0))
  return AltaiLabTools.inspect_lab()
 @toolset_registry.tool_call
 @staticmethod
 def climb_lab(cancel: bool) -> str:
  """Exercise the actual lab mantle, or cancel and restore movement."""
  w,pc=context();t=pc.get_editor_property('traversal')
  if cancel:t.cancel_climb()
  else:t.try_climb()
  return AltaiLabTools.inspect_lab()

 @toolset_registry.tool_call
 @staticmethod
 def prepare_hand_probe(mass: int) -> str:
  """Place one existing lab physics stone and pawn on the clear test lane, PIE only."""
  w,pc=context();p=unreal.GameplayStatics.get_player_pawn(w,0);h=pc.get_editor_property('hands');h.release()
  target=None
  for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.StaticMeshActor):
   c=a.static_mesh_component
   if c.is_simulating_physics() and abs(c.get_mass()-mass)<.1:
    target=a
  if not target:raise ValueError('No matching 1/5/20 kg lab stone')
  for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.StaticMeshActor):
   if a!=target and a.static_mesh_component.is_simulating_physics():
    a.set_actor_location(unreal.Vector(5100,-4300+a.static_mesh_component.get_mass()*40,180),False,True)
  p.character_movement.stop_movement_immediately();p.set_actor_location(unreal.Vector(4700,-3100,160),False,True);p.set_actor_rotation(unreal.Rotator(pitch=0,yaw=0,roll=0),True)
  pc.set_control_rotation(unreal.Rotator(pitch=-50,yaw=0,roll=0))
  target.static_mesh_component.set_physics_linear_velocity(unreal.Vector(0,0,0))
  target.static_mesh_component.set_physics_angular_velocity_in_degrees(unreal.Vector(0,0,0))
  target.set_actor_location(unreal.Vector(4810,-3100,95),False,True)
  return target.get_path_name()
 @toolset_registry.tool_call
 @staticmethod
 def grab_lab(release: bool) -> str:
  """Use the production grab or release in lab PIE."""
  w,pc=context();h=pc.get_editor_property('hands')
  if release:h.release()
  else:h.try_grab()
  return AltaiLabTools.inspect_hands()
 @toolset_registry.tool_call
 @staticmethod
 def inspect_hands() -> str:
  """Read actual mass, bounded effort and evaluated wrist-to-contact errors."""
  w,pc=context();p=unreal.GameplayStatics.get_player_pawn(w,0);h=pc.get_editor_property('hands')
  r={k:str(h.get_editor_property(k)) for k in ['held_mass','two_hands','stamina','applied_force','position_error','hint']}
  held=h.get_editor_property('held');r['held']=held.get_path_name() if held else None
  if held:
   r['object_position']=str(held.get_world_location());r['angular_speed']=held.get_physics_angular_velocity_in_radians().length()
  goals=h.get_editor_property('contact_goals');weights=h.get_editor_property('contact_weights')
  r['contact_errors']={bone:float((p.mesh.get_socket_location(bone)-goals[i]).length()) for i,bone in enumerate(['hand_l','hand_r','foot_l','foot_r']) if weights[i]>.5}
  return json.dumps(r)

 @toolset_registry.tool_call
 @staticmethod
 def prepare_wall_probe(kind: str) -> str:
  """Stand at an authored climbing lane in PIE."""
  w,pc=context();p=unreal.GameplayStatics.get_player_pawn(w,0);wall=pc.get_editor_property('wall_climbing');wall.release_wall();wall.set_editor_property('input_from_player',False);pc.get_editor_property('hands').release()
  target=next((a for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.StaticMeshActor) if a.actor_has_tag(unreal.Name(kind)) and a.actor_has_tag("AltaiClimbable")),None)
  if not target:raise ValueError('Use RoughRock, SmoothRock or MossyRock')
  loc=target.get_actor_location()
  p.character_movement.stop_movement_immediately();p.set_actor_location(unreal.Vector(loc.x+125,loc.y,target.get_actor_bounds(False)[0].z-target.get_actor_bounds(False)[1].z+140),False,True)
  pc.set_control_rotation(unreal.Rotator(pitch=0,yaw=180,roll=0))
  return str(p.get_actor_location())
 @toolset_registry.tool_call
 @staticmethod
 def wall_lab(action: str) -> str:
  """Exercise the wall climbing state and inspect contact effort."""
  w,pc=context();wall=pc.get_editor_property('wall_climbing')
  if action=='attach':wall.attach_wall()
  elif action=='release':wall.release_wall()
  elif action=='up':wall.set_move_intent(unreal.Vector2D(0,1))
  elif action=='place':wall.place_contact()
  elif action=='next':wall.select_next_limb()
  elif action=='free':wall.release_selected_contact()
  elif action=='jump':wall.jump_off()
  elif action=='mantle':pc.get_editor_property('traversal').try_climb_from_wall()
  return json.dumps({k:str(wall.get_editor_property(k)) for k in ['attached','wetness','grip','stamina','selected_limb','slips','hint','limb_stamina','contact_active','support_load','moving_limb','contact_progress']})

 @toolset_registry.tool_call
 @staticmethod
 def hand_obstacle_probe(prepare: bool) -> str:
  """Exercise a held stone against the authored wall, inspecting real collision state."""
  w,pc=context();p=unreal.GameplayStatics.get_player_pawn(w,0);h=pc.get_editor_property('hands')
  a=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.StaticMeshActor) if a.static_mesh_component.is_simulating_physics() and abs(a.static_mesh_component.get_mass()-5)<.1)
  if prepare:
   pc.get_editor_property('wall_climbing').release_wall();h.release()
   p.character_movement.stop_movement_immediately();p.set_actor_location(unreal.Vector(3710,-3100,194),False,True);p.set_actor_rotation(unreal.Rotator(pitch=0,yaw=180,roll=0),True)
   pc.set_control_rotation(unreal.Rotator(pitch=-60,yaw=180,roll=0))
   a.static_mesh_component.set_physics_linear_velocity(unreal.Vector(0,0,0));a.static_mesh_component.set_physics_angular_velocity_in_degrees(unreal.Vector(0,0,0))
   a.set_actor_location(unreal.Vector(3635,-3100,121),False,True);h.try_grab()
  return json.dumps({'held':h.get_editor_property('held') is not None,'object_x':a.get_actor_location().x,'pawn_x':p.get_actor_location().x,'pawn_speed':p.get_velocity().length(),'pawn_response':str(a.static_mesh_component.get_collision_response_to_channel(unreal.CollisionChannel.ECC_PAWN))})
 @toolset_registry.tool_call
 @staticmethod
 def prepare_station_probe(name: str) -> str:
  """Stand at an authored carry station, using the prop's original position, not a synthetic pickup."""
  if name not in ['Loose_Stone_1kg','Loose_Stone_5kg','Loose_Stone_20kg','Carry_Bucket_Empty_2kg','Carry_Bucket_Ballast_12kg','Carry_Stick_08kg','Carry_Crate_18kg','Carry_Crate_40kg']:raise ValueError('Unknown carry station')
  w,pc=context();pc.get_editor_property('wall_climbing').release_wall();pc.get_editor_property('hands').release()
  a=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.StaticMeshActor) if a.get_actor_label()==name)
  p=unreal.GameplayStatics.get_player_pawn(w,0);p.character_movement.stop_movement_immediately();pos=a.get_actor_location();center=a.static_mesh_component.get_center_of_mass()
  p.set_actor_location(unreal.Vector(pos.x+118,pos.y,160),False,True);p.set_actor_rotation(unreal.Rotator(pitch=0,yaw=180,roll=0),True)
  pc.set_control_rotation(unreal.MathLibrary.find_look_at_rotation(p.get_actor_location()+unreal.Vector(0,0,55),center))
  return json.dumps({'name':name,'position':str(pos),'center':str(center)})
 @toolset_registry.tool_call
 @staticmethod
 def inspect_load() -> str:
  """Read physical carry load, character movement settings and authored station props."""
  w,pc=context();h=pc.get_editor_property('hands');p=unreal.GameplayStatics.get_player_pawn(w,0)
  r=json.loads(AltaiLabTools.inspect_hands())
  r.update({k:float(h.get_editor_property(k)) for k in ['body_mass','carry_speed_scale','carry_acceleration_scale','balance_demand']})
  r.update(speed=p.character_movement.max_walk_speed,acceleration=p.character_movement.max_acceleration,braking=p.character_movement.braking_deceleration_walking,velocity=p.get_velocity().length())
  r['props']=[{'name':a.get_actor_label(),'mass':a.static_mesh_component.get_mass(),'position':str(a.get_actor_location()),'speed':a.get_velocity().length()} for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.StaticMeshActor) if a.static_mesh_component.is_simulating_physics()]
  return json.dumps(r)
 @toolset_registry.tool_call
 @staticmethod
 def prepare_wall_height(kind: str, height_cm: float) -> str:
  """Prepare a bounded 0-14m contact probe on a marked climbing route."""
  if kind not in ['RoughRock','SmoothRock','MossyRock'] or not 0<=height_cm<=1400:raise ValueError('Outside test route')
  w,pc=context();wall=pc.get_editor_property('wall_climbing');wall.release_wall();wall.set_editor_property('input_from_player',False);pc.get_editor_property('hands').release()
  a=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.StaticMeshActor) if a.actor_has_tag('AltaiClimbable') and a.actor_has_tag(kind));pos=a.get_actor_location();center,ext=a.get_actor_bounds(False)
  p=unreal.GameplayStatics.get_player_pawn(w,0);p.character_movement.stop_movement_immediately();p.set_actor_location(unreal.Vector(pos.x+125,pos.y,center.z-ext.z+140+height_cm),False,True);p.set_actor_rotation(unreal.Rotator(pitch=0,yaw=180,roll=0),True);pc.set_control_rotation(unreal.Rotator(pitch=0,yaw=180,roll=0))
  attached=wall.attach_wall()
  return json.dumps({'attached':attached,'height_cm':height_cm,'wall_height':ext.z*2})
 @toolset_registry.tool_call
 @staticmethod
 def inspect_wall_contacts() -> str:
  """Read material grip for each contact and actual evaluated IK errors."""
  w,pc=context();wall=pc.get_editor_property('wall_climbing');r=json.loads(AltaiLabTools.wall_lab('inspect'))
  r['limb_grip']=list(wall.get_editor_property('limb_grip'));r['contacts']=json.loads(AltaiLabTools.inspect_hands())['contact_errors'];r['position']=str(unreal.GameplayStatics.get_player_pawn(w,0).get_actor_location());return json.dumps(r)
 @toolset_registry.tool_call
 @staticmethod
 def cycle_lab_body_mass() -> str:
  """Use the same lab-only body mass control as B; no profile or character asset edits."""
  w,pc=context();pc.cycle_body_mass();return str(unreal.GameplayStatics.get_player_pawn(w,0).character_movement.mass)

"""Per-frame production contact/transition regression; fresh L_CharacterLab PIE only."""
import unreal,json,time,traceback,math
from pathlib import Path
from altai_lab_tools import context,AltaiLabTools
w,pc=context();p=unreal.GameplayStatics.get_player_pawn(w,0);wall=pc.get_editor_property('wall_climbing');hands=pc.get_editor_property('hands');tr=pc.get_editor_property('traversal')
report={'cases':{},'frames':[]};phase='start';phase_frames=[];last=None
bones=['hand_l','hand_r','foot_l','foot_r'];roots=['upperarm_l','upperarm_r','thigh_l','thigh_r'];joints=['lowerarm_l','lowerarm_r','calf_l','calf_r']
def v(q):return [q.x,q.y,q.z]
def select(i):
 while wall.get_editor_property('selected_limb')!=i:wall.select_next_limb()
def aim(i,dz=0):
 select(i);target=p.get_actor_location()+p.get_actor_forward_vector()*44+p.get_actor_right_vector()*((1 if i%2 else -1)*(28 if i<2 else 22))+unreal.Vector(0,0,(45 if i<2 else -60)+dz)
 pc.set_control_rotation(unreal.MathLibrary.find_look_at_rotation(p.get_actor_location()+unreal.Vector(0,0,50),target));return wall.place_contact()
def case(name):
 global phase,phase_frames
 if phase_frames:report['frames'].append({'phase':phase,'samples':phase_frames})
 phase=name;phase_frames=[]
def snapshot():
 return {'attached':wall.get_editor_property('attached'),'active':list(wall.get_editor_property('contact_active')),'moving':wall.get_editor_property('moving_limb'),'progress':wall.get_editor_property('contact_progress'),'stamina':list(wall.get_editor_property('limb_stamina')),'load':list(wall.get_editor_property('support_load')),'position':v(p.get_actor_location()),'mode':str(p.character_movement.movement_mode),'climbing':tr.get_editor_property('climbing'),'completed':tr.get_editor_property('completed_climbs'),'contact_errors':[(p.mesh.get_socket_location(bones[i])-hands.get_editor_property('contact_goals')[i]).length() for i,a in enumerate(wall.get_editor_property('contact_active')) if a]}
def routine():
 AltaiLabTools.set_lab_weather(1,14);AltaiLabTools.prepare_wall_height('RoughRock',650);wall.set_editor_property('assisted_stepping',False)
 yield .8
 assert wall.get_editor_property('attached');report['reach_cm']=list(hands.get_editor_property('limb_reach'))
 case('manual_reach');assert not aim(0,180);assert aim(0,10);assert not wall.get_editor_property('contact_active')[0];assert wall.get_editor_property('moving_limb')==0
 yield .8
 assert wall.get_editor_property('contact_active')[0] and wall.get_editor_property('moving_limb')==-1
 assert any(0<s['progress']<1 for s in phase_frames);report['cases']['manual_reach']=snapshot()
 case('two_hands_hang')
 select(2);wall.release_selected_contact();select(3);wall.release_selected_contact();yield .8
 assert wall.get_editor_property('attached') and list(wall.get_editor_property('contact_active'))==[True,True,False,False]
 report['cases']['two_hands_hang']=snapshot()
 case('one_hand_hang');select(1);wall.release_selected_contact();before=wall.get_editor_property('limb_stamina')[0];yield 1
 assert wall.get_editor_property('attached');assert wall.get_editor_property('limb_stamina')[0]<before;report['cases']['one_hand_hang']=snapshot()
 case('body_mass_hanging');rates={}
 for mass in [60,120]:
  p.character_movement.mass=mass;before=wall.get_editor_property('limb_stamina')[0];t0=time.monotonic();yield .8
  rates[mass]=(before-wall.get_editor_property('limb_stamina')[0])/(time.monotonic()-t0)
 assert rates[120]>rates[60]*1.5,rates;p.character_movement.mass=100;report['cases']['body_mass_hanging']={'fatigue_per_second':rates}
 case('hand_and_opposite_foot');assert aim(3);yield .8
 assert list(wall.get_editor_property('contact_active'))==[True,False,False,True];report['cases']['hand_and_opposite_foot']=snapshot()
 case('restore_hand');assert aim(1);yield .8
 assert wall.get_editor_property('contact_active')[1];report['cases']['restore_hand']=snapshot()
 case('jump_off');before=p.get_actor_location();assert wall.jump_off();yield .15
 assert not wall.get_editor_property('attached') and not pc.is_move_input_ignored();assert p.get_actor_location().x>before.x+5;report['cases']['jump_off']=snapshot()
 case('loss_of_hands');AltaiLabTools.prepare_wall_height('RoughRock',650);yield .4
 select(0);wall.release_selected_contact();select(1);wall.release_selected_contact();yield .2
 assert not wall.get_editor_property('attached') and not pc.is_move_input_ignored();report['cases']['loss_of_hands']=snapshot()
 case('assisted_ascent');AltaiLabTools.prepare_wall_height('RoughRock',650);wall.set_editor_property('assisted_stepping',True);yield .5
 start=p.get_actor_location().z
 for i in range(120):
  wall.set_move_intent(unreal.Vector2D(0,1));yield .1
  assert wall.get_editor_property('attached'),'Unexpected fall during short ascent'
  if p.get_actor_location().z-start>=200:break
 gain=p.get_actor_location().z-start;assert gain>=200,('Ascent stuck',gain);wall.set_move_intent(unreal.Vector2D(0,0));report['cases']['assisted_ascent']={**snapshot(),'gain_cm':gain}
 wall.release_wall();yield .2
 # Each saved route must have a real, unobstructed landing at its visible lip.
 for name in ['Rough','Smooth','Moss']:
  case('actual_top_'+name)
  actor=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.StaticMeshActor) if a.get_actor_label()=='Climbing_Test_'+name)
  center,ext=actor.get_actor_bounds(False);p.character_movement.stop_movement_immediately();p.set_actor_location(unreal.Vector(center.x+ext.x+44,center.y,center.z+ext.z-60),False,True);pc.set_control_rotation(unreal.Rotator(pitch=0,yaw=180,roll=0));p.set_actor_rotation(unreal.Rotator(pitch=0,yaw=180,roll=0),True)
  assert wall.attach_wall();yield .6
  if name=='Rough':
   blocker=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.StaticMeshActor) if a.get_actor_label()=='Carry_Crate_40kg');saved=blocker.get_actor_transform();blocker.static_mesh_component.set_simulate_physics(False);blocker.set_actor_location(unreal.Vector(center.x-4,center.y,center.z+ext.z+125),False,True)
   assert not tr.try_climb_from_wall() and wall.get_editor_property('attached'),'Occupied landing accepted'
   report['cases']['occupied_landing']={'rejected':True,'hint':tr.get_editor_property('hint')};blocker.set_actor_transform(saved,False,True);blocker.static_mesh_component.set_simulate_physics(True)
  count=tr.get_editor_property('completed_climbs');assert tr.try_climb_from_wall(),tr.get_editor_property('hint');yield 2.3
  assert tr.get_editor_property('completed_climbs')==count+1 and not pc.is_move_input_ignored(),tr.get_editor_property('hint')
  report['cases']['actual_top_'+name]=snapshot()
 # Isolated ledge fixture: production traces and swept capsule, no change to saved level.
 fixture=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.StaticMeshActor) if a.get_actor_label()=='Climbing_Test_Rough')
 original=fixture.get_actor_transform();fixture.static_mesh_component.set_mobility(unreal.ComponentMobility.MOVABLE);fixture.set_actor_location(unreal.Vector(9000,0,400),False,True);fixture.set_actor_scale3d(unreal.Vector(2,6,8));fixture.set_actor_hidden_in_game(False)
 def at_lip():
  p.character_movement.stop_movement_immediately();p.set_actor_location(unreal.Vector(9144,0,740),False,True);pc.set_control_rotation(unreal.Rotator(pitch=0,yaw=180,roll=0));p.set_actor_rotation(unreal.Rotator(pitch=0,yaw=180,roll=0),True);assert wall.attach_wall()
 at_lip();yield .5
 case('mantle_finish');count=tr.get_editor_property('completed_climbs');assert tr.try_climb_from_wall();assert not hands.try_grab();yield 2.3
 assert tr.get_editor_property('completed_climbs')==count+1 and not tr.get_editor_property('climbing');assert not pc.is_move_input_ignored();assert p.get_actor_location().z>=890;report['cases']['mantle_finish']=snapshot()
 at_lip();yield .4
 case('mantle_cancel');assert tr.try_climb_from_wall();yield .25;tr.cancel_climb();yield .1
 assert not pc.is_move_input_ignored() and not tr.get_editor_property('climbing');report['cases']['mantle_cancel']=snapshot()
 fixture.set_actor_transform(original,False,True);fixture.set_actor_hidden_in_game(True);fixture.static_mesh_component.set_mobility(unreal.ComponentMobility.STATIC);case('complete');report['max_chain_error_cm']=max(abs(s['chain_lengths'][i]-report['reach_cm'][i]/.97) for phase in report['frames'] for s in phase['samples'] for i in range(4))
 report['max_support_error_cm']=max((max(c['contact_errors']) for c in report['cases'].values() if c.get('contact_errors')),default=0)
 assert report['max_support_error_cm']<5, ('Support slipped away from evaluated limb',report['max_support_error_cm'])
 assert report['max_chain_error_cm']<.2, ('Bones changed length during blending',report['max_chain_error_cm'])
 AltaiLabTools.place_lab_player(4700,-3100,160,0)
 report['passed']=True

gen=routine();deadline=0
# Baseline chain lengths are measured each frame against the reference-pose limb total.
def tick(dt):
 global deadline,last
 try:
  now=time.monotonic();s=snapshot();s['dt']=dt
  positions=[p.mesh.get_socket_location(b) for b in bones];s['ends']=[v(q) for q in positions]
  s['chain_lengths']=[(p.mesh.get_socket_location(roots[i])-p.mesh.get_socket_location(joints[i])).length()+(p.mesh.get_socket_location(joints[i])-positions[i]).length() for i in range(4)]
  phase_frames.append(s)
  if now>=deadline:deadline=now+next(gen)
 except StopIteration:
  unreal.unregister_slate_post_tick_callback(handle);Path(unreal.Paths.project_saved_dir(),'contact_motion_validation.json').write_text(json.dumps(report,indent=2));unreal.log('CONTACT_MOTION_VALIDATION_PASSED')
 except Exception:
  unreal.unregister_slate_post_tick_callback(handle);report['passed']=False;report['error']=traceback.format_exc();report['frames'].append({'phase':phase,'samples':phase_frames});Path(unreal.Paths.project_saved_dir(),'contact_motion_validation.json').write_text(json.dumps(report,indent=2));unreal.log_error(report['error'])
handle=unreal.register_slate_post_tick_callback(tick)

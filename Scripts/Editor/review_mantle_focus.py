"""Inspect the actual evaluated limbs across the mantle and lowering phases."""
import unreal,time,json,traceback
from pathlib import Path
from altai_lab_tools import context
w,pc=context();p=unreal.GameplayStatics.get_player_pawn(w,0);wall=pc.wall_climbing;t=pc.traversal;h=pc.hands
unreal.AltaiEditorLibrary.set_physics_test_mode(True);p.un_crouch();h.release();t.cancel_climb();wall.release_wall();wall.input_from_player=False
rock=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.StaticMeshActor) if a.get_actor_label()=='Climbing_Test_Rough');center,ext=rock.get_actor_bounds(False);top=center.z+ext.z
p.set_actor_location(unreal.Vector(center.x+ext.x+44,center.y,top-55),False,True);p.set_actor_rotation(unreal.Rotator(yaw=180),True);pc.set_control_rotation(unreal.Rotator(yaw=180));wall.attach_wall();p.set_first_person(False)
rows=[];result={};phase='settle';began=time.monotonic()
def wait(s):
 end=time.monotonic()+s
 while time.monotonic()<end:yield
def run():
 global phase
 yield from wait(1);phase='up';result['start']=t.try_climb_from_wall();pc.set_control_rotation(unreal.Rotator(pitch=-15,yaw=95,roll=0));yield from wait(3.5);result['top']=p.character_movement.is_moving_on_ground();pc.set_control_rotation(unreal.Rotator(yaw=0));phase='down';result['lower']=t.try_descend();pc.set_control_rotation(unreal.Rotator(pitch=-15,yaw=95,roll=0));yield from wait(4);result['hang']=wall.attached
g=run()
def finish(error=''):
 unreal.unregister_slate_post_tick_callback(handle);unreal.AltaiEditorLibrary.set_physics_test_mode(False)
 # IK blend weight is not a load flag: an airborne transferring foot also has weight 1.
 # Identify acquired contacts from a stationary target over several rendered frames.
 def planted(index,limb):
  sample=rows[index]
  previous=[s for s in rows[max(0,index-6):index] if s['phase']==sample['phase'] and sample['seconds']-s['seconds']>=.08]
  return bool(previous) and all(sum((a-b)**2 for a,b in zip(s['goals'][limb],sample['goals'][limb]))<.01 for s in previous)
 maximums={phase:[max((s['errors'][i] for j,s in enumerate(rows) if s['phase']==phase and s['mantling'] and s['progress']>.12 and s['weights'][i]>.99 and planted(j,i)),default=0) for i in range(4)] for phase in ['up','down']}
 moving_max=max((s['errors'][i] for s in rows if s['mantling'] and s['progress']>.12 for i in range(4) if s['weights'][i]>.99),default=0)
 clearance={b:min((s['bones'][b][2]-top for s in rows if s['mantling'] and s['bones'][b][0]<center.x+ext.x),default=999) for b in ['pelvis','calf_l','calf_r','foot_l','foot_r']}
 import sys
 sys.path.insert(0,str(Path(unreal.Paths.project_dir())/'Scripts/MCP'))
 from analyze_climbing_joints import analyze
 joint_review=analyze(rows)
 checks=[joint_review['passed'],all(result.values()),moving_max<15,all(v<5 for values in maximums.values() for v in values),clearance['pelvis']>20,clearance['calf_l']>4,clearance['calf_r']>4,clearance['foot_l']>4,clearance['foot_r']>4]
 Path(unreal.Paths.project_saved_dir(),'mantle_focus_review.json').write_text(json.dumps({'passed':all(checks) and not error,'checks':checks,'joint_review':joint_review,'max_loaded_error_cm':maximums,'max_moving_guide_error_cm':moving_max,'min_inside_clearance_cm':clearance,'error':error,'result':result,'samples':rows},indent=2));unreal.log('MANTLE_FOCUS_DONE '+error)
def tick(dt):
 try:
  next(g)
  rows.append({'seconds':time.monotonic(),'phase':phase,'progress':t.progress,'mantling':t.climbing,'attached':wall.attached,'offset':list(t.body_offset.to_tuple()),'rotations':{b:list(p.mesh.get_socket_rotation(b).quaternion().to_tuple()) for b in ['thigh_l','thigh_r','calf_l','calf_r','upperarm_l','upperarm_r','lowerarm_l','lowerarm_r','foot_l','foot_r']},'weights':list(h.contact_weights),'goals':[list(g.to_tuple()) for g in h.contact_goals],'errors':[(p.mesh.get_socket_location(b)-h.contact_goals[i]).length() for i,b in enumerate(['hand_l','hand_r','foot_l','foot_r'])],'bones':{b:list(p.mesh.get_socket_location(b).to_tuple()) for b in ['pelvis','upperarm_l','upperarm_r','hand_l','hand_r','foot_l','foot_r','calf_l','calf_r','thigh_l','thigh_r','lowerarm_l','lowerarm_r']}})
 except StopIteration:finish()
 except Exception:finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)

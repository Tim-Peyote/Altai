# Contact motion review — 8 September 2026

This is a playable procedural prototype in `L_CharacterLab`, using the existing locomotion animation plus the native post-process contact node. It is not a set of finished authored climbing/carry animations.

## Changes

- Rig-derived reach: upper/lower arm and leg lengths are measured in reference pose with skeletal component scale. Reach reserves 3% before full extension. The current mannequin gives approximately 53.37 cm per arm and 82.99 cm per leg.
- Fixed an actual blending defect: interpolating component-space joint positions shortened limbs during pickup/release. Joint rotations now blend while child positions are reconstructed from unchanged local bone offsets. Descendants, including twist and finger bones, follow the corrected parents.
- One contact transfer at a time. A moving limb stops carrying load, follows a smooth 0.28–0.65 second outward arc, then becomes support only at the destination. Manual and assisted stepping share this path. The last supporting hand cannot be moved without restoring another hand.
- Deliberate limb release and recovery: two-hand hanging, one-hand hanging, and hand-plus-foot support are distinct states. Freed limbs fade to the underlying relaxed pose over time; a renewed reach starts at the evaluated wrist/ankle. Support-dependent body pose lowers and shifts the body, while contact goals stay fixed. The first-person wall camera follows the evaluated neck height within capsule-safe limits.
- Support load and fatigue account for the character's mass, active supports, extension, surface grip, moisture and strong gripping. One-hand hanging consumes stamina faster; it is not a resting stance. Losing both hands causes a fall even with feet remaining.
- Wall jump-off restores movement and applies outward/upward velocity.
- Wall-to-top transition requires both hands, stamina, a reachable walkable top and room for the standing capsule. The movement is swept throughout; interruption restores falling and input. Picking up an object is disallowed during the mantle.
- Cliff art now terminates at each playable lip. Landing collision matches the top; crown rocks are set back so the standing capsule has clearance.
- Fixed moving-hand detachment: the hold drive now ticks after CharacterMovement, the lab skeletal mesh evaluates after physics, and the animation node samples current rigid-body grip points. Actual capsule displacement (including stairs) supplies the drive velocity, rather than ground velocity with its missing vertical step motion. The previous mesh tick group is restored when the hands component ends.
- Carrying retains the existing mass-dependent speed/acceleration/braking, brings heavier objects closer and lower while respecting prop depth, adds a smoothly blended body brace, preserves locomotion foot targets and smoothly rotates authored grips with character turns. Wrist orientation and finger curl fade through release.

## Controls in the lab

| Control | Action |
|---|---|
| V | First/third person |
| F | Take/release object |
| E | Attach, ground mantle, or pull up from a wall |
| WASD | Move body on wall; assisted contact changes |
| RMB / LMB | Select limb / reach to aimed point |
| Q | Free selected limb |
| Shift | Strong grip |
| Space | Jump away from wall |
| C | Release wall or interrupt mantle |
| B | Cycle body mass, 60 / 80 / 100 / 120 kg |

## Repeatable checks

`Scripts/Editor/validate_contact_motion.py` records evaluated joints per frame and verifies manual transfer, support loss/recovery, one-hand hanging, diagonal support, jump-off, losing both hands, actual assisted ascent, all three saved cliff tops, mantle completion and cancellation. It checks constant skeletal chain lengths through transitions. Results: `Saved/contact_motion_validation.json`. The final contact regression keeps chain-length error below 0.001 cm; the largest settled support-target error is approximately 0.026 cm after the tick-order correction. Occupied landings are rejected without dropping the wall. A separate `validate_continuous_climb.py` completed 1001.3 cm of real ascent in 135.8 seconds with four planned rests and sequential regrips. There are no upward teleports after attachment.

`Scripts/MCP/validate_expanded_props.py` exercises all eight prop stations, checks mass, wrist goal error, angular stability, walking speed restoration, first/third-person captures and refusal of the 40 kg crate. Results: `Saved/expanded_props_validation.json`.

`Scripts/Editor/validate_wall_fall.py` tests actual rainfall saturation, successive grip failures and restored input. `validate_load_course.py` exercises real walking over the beam with a 20 kg load, including steps and stumbles. It now compares wrists to live rigid-body grip points and fails above 8 cm. The corrected run measured a maximum 0.949 cm during walking (formerly about 59 cm), completed the loaded course in 4.11 seconds and retained the 20 kg object. Maximum walking speed remained 357.14 cm/s versus 500 cm/s empty.

## Remaining animation work

- Finger poses are an approximate procedural curl with authored wrist frames for the bucket, stick and crate. Individual phalanx/object collision, thumb opposition tailored to each object, and finger pressure are not solved.
- Pickup still has an interaction/pull-in range beyond the anatomical arm reach. The IK does not stretch the arm, but this is not a fully physical reach-and-close-hand acquisition.
- The mantle is a collision-tested procedural body path with contact release, not an authored chest-over-lip, palm-push and knee-placement animation. It should receive dedicated clips or a fuller control rig before final animation approval.
- Partial support uses an animated body offset and load/fatigue model, not a dynamic center-of-mass pendulum, shoulder dislocation model or whole-body force solver. No overhang/corner/component-to-component route transfer.
- Carrying uses the existing speed-driven gait plus a brace. Separate heavy-lift start, heavy-step, stagger, turn and drop clips remain to be authored.
- Mathematical interpolation cannot compensate for the lab's approximately 12–14 FPS on this Mac; final motion review also needs a stable frame rate.

## Visual evidence

Final evaluated poses: [one hand](Images/contact_one_hand.png), [diagonal support](Images/contact_diagonal_support.png), [first-person wall](Images/contact_first_person_wall.png), [20 kg stone](Images/expanded_Loose_Stone_20kg_first.png), [crate](Images/expanded_Carry_Crate_18kg_third.png).

The machine-readable result summary is `Saved/contact_motion_summary.json`.

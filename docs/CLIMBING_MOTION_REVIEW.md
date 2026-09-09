# Climbing motion review

> Superseded visual-quality assessment: see [CLIMBING_JOINT_SAFETY.md](CLIMBING_JOINT_SAFETY.md). Contact-target tests below did not detect joint inversion.

The lab now treats climbing as a sequence of loaded contacts. Entry catches with the right hand while the left reaches; movement can continue slowly during a single limb's transfer. The supporting limbs constrain body movement. Manual selection, resting a limb and regripping still work, including a right-hand/left-foot hang. Descending uses the same contact constraints with lower targets and a foot-first bias.

## Reference and design decisions

- [The Game Bakers: Cairn](https://www.thegamebakers.com/cairn/) is the reference for deliberate route choice and resource management. The implementation here is authored for Altai, not a reproduction of Cairn's proprietary solver.
- [REI: Climbing Techniques and Moves](https://www.rei.com/learn/expert-advice/climbing-techniques.html), with instructor Jay Parks, explains foot loading, hip position, straight-arm rests and keeping a planted foot stationary. These informed the body transfer, fixed supporting contacts and limb sequencing.
- [REI's filmed demonstration](https://www.youtube.com/watch?v=iazQou-36sE&t=146s) was viewed in the browser, including back stepping, lay-backing and the mantle section. The mantle is a push onto the lip followed by bringing a foot up and transferring weight over it. No footage or third-party animation assets were imported into the project.

## Changes

Manual placement casts from the actual camera in both views and still checks the selected limb’s reach. The old chest-origin ray could select a different point from the crosshair.

The step planner selects a trailing reachable limb before it becomes overextended, rather than waiting for the body to hit its reach limit and stopping entirely. A transfer includes a short unloading phase, an outward reach arc, contact and settling. The remaining supports constrain requested displacement to their reach spheres. Surface grip contributes to support load; load, extension, mass and wetness still affect fatigue and slips. Released limbs relax with an aligned wrist and open fingers instead of reverting abruptly to a walking hand pose.

The postprocess rig solves torso translation against loaded hands and feet before two-bone IK. It preserves bone lengths, adds a modest support-dependent hip turn and uses climbing-specific elbow and knee directions. Finger curl is reduced while reaching or resting and becomes a flatter palm for the final push.

Mantling is now a contact sequence: acquire the lip, lift the first foot, pull/push, bring the other foot onto the top, release the hands separately and stand. A compact capsule permits the crouched rock-over while retaining collision. The palms re-seat separately farther onto the top before the hips cross. Each foot first rises outside the wall, then crosses the lip. The torso leans over the palms, pelvis clearance is constrained, and the knee solver selects an above-surface bend once hip and ankle clear the edge. The path and full standing landing are checked before wall support is surrendered. A failed or cancelled transition releases the movement lock; capsule expansion is rejected where it would overlap geometry.

In first person the deliberate turn toward the wall is added smoothly to the view while preserving mouse input. View switching during traversal restores the correct movement/orientation settings afterward.

C near a climbable edge starts a reverse transition from the player's current position: turn toward the face, crouch, acquire the lip, move feet outside, lower and hand off to normal wall climbing. S then continues downward; reaching a walkable floor exits wall climbing. C while attached still lets go, and Space pushes off. The developer panel and contextual hints explain the new controls without changing the existing panel font size.

## Verification

- `Scripts/Editor/review_climbing_cycle.py`: asymmetric attachment, ascent, descent, one-hand/one-foot hang, manual restoration, mantle, grounded completion, reverse transition and descent after lowering.
- `Scripts/Editor/review_mantle_focus.py`: actual evaluated limb errors and pelvis/knee/ankle clearance throughout both transition directions, beyond successful capsule movement alone.
- `Scripts/Editor/review_climbing_boundaries.py`: dry/wet route behavior, blocked landing rejection, cancel cleanup, jump-off and missing edge rejection.
- `Scripts/Editor/review_climbing_aim.py`: all four limbs aimed at known surface points from both cameras.
- `Scripts/Editor/review_climbing_camera.py`: first-person lowering, turn direction, view switching and input restoration.
- `Scripts/Editor/review_ground_mantles.py`: 45, 100 and 160 cm obstacles and grounded completion.
- `Scripts/Editor/validate_continuous_climb.py`: 10 m ascent with normal regrips/rests, without upward teleports after setup.
- `Scripts/Editor/view_mantle_motion.py`: temporary slow-motion side-view review; time scale restores automatically.

This is a procedural climbing implementation on the existing character rig, not bespoke climbing mocap or a full-body rigid-body/tendon simulation. Authored collision faces and reachable route geometry remain important. Verification results are recorded in Saved and summarized below.

### Recorded results (2026-09-09)

The functional climbing cycle passed, including a one-hand/one-foot hang and manual restoration. Focused pose checks also inspect the actual evaluated skeleton, including the landing clearance; final measurements are recorded in `Saved/mantle_focus_review.json`. The camera-directed placement check passed all eight hand/foot/view combinations. First-person turn, view switching and restored input passed. Mantles on 45, 100 and 160 cm obstacles completed grounded, with the normal approximately 2.15 cm character floor clearance.

All three base route surfaces passed up/down movement checks. With real rain exposure, measured grip changed from 0.95 to 0.65 on rough rock, 0.65 to 0.25 on smooth rock and 0.80 to 0.18 on moss. Wet moss produced contact slips and a fall. Blocked landings preserved the wall attachment; cancellation, push-off and missing-edge rejection passed.

The continuous climb gained 1000.04 cm in 143.27 seconds with four rests and no upward repositioning after initial setup. The later first-person camera correction follows the neck's lateral/forward displacement as well as height, keeping the eye ahead of the torso when inspecting footholds; it does not change the movement planner.

Final mantle pose checks passed with maximum loaded hand error of 3.16 cm upward and 1.66 cm downward. For joints over the landing footprint, minimum pelvis clearance was 24.0 cm, knee clearance 28.3 cm, and ankle clearance 5.9 cm. Those are joint positions, not guarantees for every alternate character mesh or every surface shape. Both transition directions, camera switching and all three obstacle heights passed on the final build.

Side-view inspection caught and corrected a pelvis/knee intersection that contact-target checks alone had missed. The final ascent was inspected again at the crouched transfer. Slow-motion helpers now slow the world, so the controller's traversal and character animation share the same time scale, and restore it automatically. The current deep crouch remains an authored procedural pose; bespoke climbing animation would be the next visual-quality step.

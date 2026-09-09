# Anatomical grip review

The rotation control now means turning the held object with a fixed hand contact. It no longer spins the object through sliding palms. R preserves the wrist's accumulated pose across releases and repeated presses. Mouse Y flexes the wrist, mouse X deviates it, and the wheel pronates/supinates the forearm. Limits share a combined comfort envelope, reduce with load/two-hand support and account for the forearm's initial rotation. Reverse movement has no hidden accumulated input to unwind.

## Reference decisions

- [GRASP taxonomy, Feix et al.](https://www.csc.kth.se/grasp/taxonomyGRASP.pdf): differentiate power, precision and intermediate grasps, palm/pad opposition and thumb posture. Profiles distinguish cylinder wrapping, handle hooking, rod power grips and broad support. Our joint angles are authored rig settings, not values measured from this paper.
- [GRAB human-object motion capture, Taheri et al.](https://grab.is.tue.mpg.de/): grasping involves the body and multiple contacts; evaluate the elbow, forearm, wrist and object relationship together, in both views. Dataset motion assets were not imported or licensed into the game.
- [Wrist motion study](https://pubmed.ncbi.nlm.nih.gov/25097812/): wrist motion and forearm rotation are distinct; the study reports physiological ranges, not recommended loaded gameplay limits. The implementation uses conservative configurable control limits rather than claiming universal anatomical values.
- [Frictional: Evoking Presence](https://frictionalgames.com/2017-04-evoking-presence/): keep a continuous, understandable connection between input and character action. R now moves the hand-object unit and exposes its limits, rather than becoming an object viewer.
- [Fort Solis developer interview, Epic](https://www.unrealengine.com/developer-interviews/fort-solis-uses-unreal-engine-5-2-to-deliver-an-immersive-martian-experience): procedural object examination driven by player input, postprocess rig correction and body alignment. This is a more directly documented reference for this particular mechanic than assuming proprietary details about Tarkov or Kingdom Come.

## Rig changes

Fixed local flexion axes replace a changing world-space curl axis. Each phalanx blends toward an absolute authored pose from the reference skeleton, so idle finger animation does not add another uncontrolled curl. The thumb has a separate opposition setting. Profiles are stored as `AltaiGripProfile` asset user data on each test mesh; socket frames specify the wrist, finger direction and palm normal.

First-person forward eye offset is capped at 8 cm; the previous 20 cm offset made physically reachable loads fill the view. Heavy buckets are carried lower, with an overhead rim/handle hook rather than fingertips floating above the rim.

The elbow plane follows the grasp direction for loose props. A downward hook and an upright cylinder cannot share a fixed elbow pole. Forearm roll is solved before wrist swing, with limits and diagnostics. Bone lengths are preserved. A sustained impossible wrist pose releases an obstructed prop rather than allowing the wrist to continue turning indefinitely. Furniture keeps its supported torso reach and dedicated elbow setup.

The profiles and limits are not a full tendon/skin-contact simulation. Finger placement requires visual review for every authored prop, especially handle apertures and glove clearance. Unknown imported objects receive a bounded fallback and still need a reviewed profile for finished-quality use.

## Verification

`Scripts/Editor/review_anatomical_grips.py` checks seven prop meshes (including both empty and loaded buckets), both directions on all three anatomical axes, persistent limits across R re-entry, immediate reverse response, fixed local contact points, forearm bounds, held state and camera stability. Report: `Saved/anatomical_grips_review.json`.

The object manipulation and furniture suites remain the regression checks. Visual checks must include the small flask, handled cup, bucket rim and third-person arm pose. Build or wrist-position tests alone do not establish visual quality.

### Completed review — 2026-09-09

Mac Development build succeeded. The final rotation matrix passed 8 cases / 48 axis directions, with fixed contacts, forearm bounds, persistent limits and immediate reverse response. Maximum required wrist correction was 5.28 degrees; stationary camera displacement was zero. `Saved/anatomical_grips_final.json` records provenance: the full matrix plus a focused rod rerun after removing the forced 5-degree minimum forearm allowance.

The final manipulation regression passed all 14 checks across 9 cases, including a full 20 kg throw. That test exposed an over-deep two-hand windup which approached an elbow/forearm singularity; two-hand windup is now a short 3 cm back / 2 cm up movement. The single-hand windup is unchanged. Both furniture regressions passed all 9 fixtures each, standing and crouched.

Visual review in the running editor covered the flask and cup in first person, loaded bucket rim contact, and flask/bucket arm poses from the side in third person. A native mouse drag and release also released the held cup. These checks establish the current test-prop implementation, not finished bespoke animation or collision-perfect finger skin. The saved poses remain procedural and should be reviewed again with the final character, gloves and production props.

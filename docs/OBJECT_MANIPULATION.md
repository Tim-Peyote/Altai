# Physical object manipulation — 2026-09-09

## Controls in L_CharacterLab

- F: toggle persistent carry / release.
- Hold LMB: grab and move the hand with the mouse. Release LMB to let go with the object's actual physical velocity. Upward strokes move forward/up, horizontal strokes sweep sideways. The view stays steady while moving the hand.
- Hold RMB: build a throw, release RMB to complete the forward stroke and let go. View aiming remains available while charging. F cancels by releasing the object.
- Hold R + mouse: flex/deviate the wrist within the current grasp. R + wheel rotates the forearm. Limits persist across repeated presses and narrow with load.
- Wheel: move the carry target closer/farther, within arm reach.
- T: change one/two-handed grip. Loads above the strength-adjusted 6 kg one-hand limit remain two-handed.
- Ctrl+D: developer panel, including controls, current manipulation mode, last throw speed/energy/mass. Opening the panel cancels charge/mouse modes and retains the object. Font size is unchanged.

Furniture and climbing keep their contextual LMB/RMB behavior. F1 remains unbound by the lab.

## Implementation

Loose props remain simulated Chaos rigid bodies. A bounded, mass-aware critically damped force follows a reachable hand target; angular torque respects per-axis inertia. There is no attachment/teleport used to move or rotate a prop. Carry speed, acceleration, stamina and balance demand still depend on load versus body mass. Grip targets are projected into both arm reach volumes, rather than stretching the rig or leaving the prop beyond the hand.

R mode turns the object with fixed local hand contacts, bounded by an anatomical control envelope. The final orientation is retained when R is released. Sliding regrips were removed. See ANATOMICAL_GRIPS.md for the current rig and profile design.

Charged throws convert bounded work to speed using mass: sqrt(2*work/mass), including body mass, stamina and wetness. One/two-hand work budgets are 60/160 J plus a small base toss, with a 14 m/s requested speed ceiling. The outgoing velocity combines existing prop motion with the throw, under a speed cap, then uses a mass-scaled impulse. Release timing includes a brief forward stroke and fading arm follow-through. Gesture release applies no new impulse. CCD stays active during fast flight, pawn collision returns only after clearing the capsule, and original CCD/damping settings are restored when appropriate.

Hand targets follow the solved rigid-body pose after physics. Spine/brace, reach, finger curl and release blend procedurally. Small props receive a tighter finger curl than broad objects. This is not a full contact solver for each phalanx, nor a new mocap animation set.

## Test area

The camp's new throw table is at approximately X=4650, Y=-3200. It contains original test geometry: a 250 g flask, a 1 kg bottle and a 400 g handled cup. The cup uses a hollow compound collision shape. Wooden 1/3/6 kg tipping targets form a short lane beyond the table. Existing stones, buckets, stick and crates remain available for load comparisons. Authoring scripts are idempotent; source OBJ meshes are included.

## Design references

- [Frictional Games: Let's not forget about Physics](https://frictionalgames.com/2010-09-lets-not-forget-about-physics/): responsive physical control, mouse-to-world interaction and an explicit rotation mode.
- [Official Amnesia: A Machine for Pigs manual](https://shared.fastly.steamstatic.com/store_item_assets/steam/apps/239200/manuals/Manual.pdf?t=1727954562): hold-to-grab, separate rotation/throw input and carry-distance control.
- [Bare Mettle's Exanima description](https://store.steampowered.com/app/362490/Exanima/): momentum, forces and collisions as gameplay foundations.
- [Warhorse Weekly Torch, animator Karel Taufman interview](https://forum.kingdomcomerpg.com/t/warhorse-studios-weekly-torch/29593?page=5): prop-contact correction, transitions and viewpoint-specific animation review. This informs the review approach; it is not evidence that Kingdom Come uses this throw implementation.

## Reproducible verification

Run the editor helpers in a PIE L_CharacterLab session. They temporarily reposition existing PIE props and restore them on completion; they never save runtime test positions into the map.

- `Scripts/Editor/review_object_manipulation.py`: holds from 0.25–20 kg, one/two-hand limits, wrist contact, three anatomical rotation axes, charge strength, gesture velocity preservation, panel cancellation and view restoration. Report: `Saved/object_manipulation_review.json`.
- `Scripts/Editor/review_object_collision.py`: a fast small prop against a 2 cm wall and restoration of pawn collision. Report: `Saved/object_collision_review.json`.
- Existing standing/crouched furniture review scripts check doors, chest and six drawers after the shared grab/release refactor.

These scripts drive the same interaction functions as input but do not emulate a full human mouse/trackpad session. Multiplayer, arbitrary imported prop geometry, breakage/damage and bespoke mocap are outside this implementation.

Final AltaiEditor Mac Development build passed. The manipulation suite passed all 14 conditions over 9 cases. The 250 g flask at 14 m/s was stopped by the 2 cm wall, and pawn collision returned after clearance. Final structured results are in the two Saved reports above.
Furniture regression also passed all 9 standing and all 9 crouched fixtures on the final build.

The anatomical-grip revision was rebuilt and rechecked on 2026-09-09: all 14 manipulation conditions and 18 furniture cases passed again. Two-hand charge uses a shorter windup to preserve forearm alignment under a 20 kg load. See `ANATOMICAL_GRIPS.md` for the new rotation/contact matrix and visual review scope.

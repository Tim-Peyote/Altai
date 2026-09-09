# Furniture reach and camera review — 2026-09-09

## Final behavior

Furniture reach includes a bounded procedural body adjustment instead of checking arm length alone. The torso can move up to 25 cm horizontally and 50 cm downward; foot IK anchors the stance. The target is 80% arm reach. Bone lengths are preserved, held arm IK is capped at 97% extension during transitions, and unsupported targets still fail with a move-closer hint.

Elbow poles sit below and slightly outside the shoulder. Furniture sockets describe the handle contact; the wrist target sits 7 cm behind the finger direction and 2 cm outside the palm normal. Entry and release blend smoothly, with feet remaining anchored during pelvis recovery. The opening drive waits 0.35 seconds, and reach-loss checking allows 0.8 seconds for the initial body motion to settle.

First-person eyes use corrected capsule-relative heights (64 cm standing, 56 cm crouched) and at most 20 cm forward offset. A bounded reach offset follows the same smooth body adjustment rather than sampling idle neck motion. This deliberately moves the viewpoint during leaning and recovery; the camera remains steady during stationary manipulation. The character uses the full original mesh in both views.

The interaction component forces pose and bone refresh even when the body leaves the camera view and disables animation update-rate skipping while active. Its previous mesh tick settings are restored on component teardown. This prevents stale shoulder and wrist transforms during first-person manipulation.

## Verification

The earlier standing baseline failed all nine fixtures at the same 48 cm approach distance where crouching worked. Shoulder-to-handle distances were 69–93 cm. Baseline: Saved/furniture_standing_before.json.

Reproducible PIE scripts:
- Scripts/Editor/review_furniture_standing.py
- Scripts/Editor/review_furniture_camera.py (crouched cases)

Both scripts exercise the chest lid, two door leaves and six drawers, checking grab, mechanism travel, held wrist position, release/input restoration, and camera displacement during manipulation. Standing also records elbow extension. The tests use normal aim/TryGrab, scripted DragInteraction and Release, not raw mouse/trackpad input.

Visual checks cover first-person lower drawer grip and the complete pose from the side. Screenshots: Saved/furniture_standing_fp.png and Saved/furniture_standing_side.png.

The hand and body motion remains procedural. Finger curl is shared, not a separate contact-solving animation for every handle shape. Walking into a moving door, arbitrary approach angles and network play are outside this stationary regression matrix.

Final build succeeded (AltaiEditor Mac Development). Final PIE regression: 9/9 standing and 9/9 crouched cases passed. Aggregate measurements: Saved/furniture_reach_review_summary.json.

Subsequent anatomical-grip work moved the first-person forward eye limit from 20 cm to 8 cm and revised finger/forearm posing. See ANATOMICAL_GRIPS.md for the current behavior and validation.

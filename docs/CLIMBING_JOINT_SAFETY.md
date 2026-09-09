# Climbing joint safety and motion correction

This review supersedes the visual-quality conclusion in CLIMBING_MOTION_REVIEW.md. The earlier contact-distance checks did not test whether the rendered joints bent anatomically. They were insufficient: knee direction could reverse, the leg could cross the body's center, and the underlying locomotion animation could move the pelvis abruptly during traversal.

## References applied

- [Epic: Full Body IK](https://dev.epicgames.com/documentation/en-us/unreal-engine/control-rig-full-body-ik-in-unreal-engine): preferred angles, joint limits and a coordinated body solve.
- [Epic: Motion Warping](https://dev.epicgames.com/documentation/en-us/unreal-engine/motion-warping-in-unreal-engine): align authored motion to environmental targets. This change does not claim to import a motion-warped mocap montage; the current traversal uses explicit phase trajectories.
- [REI climbing moves, filmed mantle demonstration](https://www.youtube.com/watch?v=iazQou-36sE&t=146s): push the body over the hands before bringing the foot up and transferring weight. The footage was viewed; it was not imported as an animation asset.

## Implementation

FullBodyIK is enabled. The animation node uses Unreal's PBIK solver for a coordinated pelvis, spine and limb pose, followed by rigid two-segment contact correction in a shared anatomical hinge frame. Final hinge projection prevents an iterative solver's residual constraint error from becoming an inverted knee or elbow. Segment lengths remain fixed. The correction searches for knee positions that clear the lip, remain on the correct side of the body and respect hip limits. If a guide is unreachable, the limb keeps its joint limits instead of stretching or inverting.

Knee and elbow flexion bounds account for the actual reference bend of the mannequin. The elbow's reference pose is already bent, so adding a nominal 145 degrees to it would exceed the intended total flexion. Hip rotation has a bounded envelope. Forearm roll is limited in angle and speed, and hand orientation is limited relative to the forearm.

The node captures a stable entry pose for traversal and blends back to locomotion after release. This prevents the walk/fall state machine from injecting a different pelvis keyframe midway through a climb. Lean is distributed over the pelvis and spine instead of concentrating a large bend at one lumbar joint.

Mantling now includes intermediate wall footholds before the feet pass the lip. Body rise, first foot placement, crossing, second foot placement and standing are sequenced. The reverse follows the same support sequence backward. Hand acquisition/release has a longer window and applies its position blend once.

## Verification criteria

`review_mantle_focus.py` captures actual world-space bone positions and rotations every rendered frame through ascent and lowering. `analyze_climbing_joints.py` independently measures signed flexion from the upper/lower segment vectors and the upper bone's hinge axis, lateral departure from that plane, and frame-to-frame local rotation. It does not trust effector success as evidence of anatomical validity.

Stationary, acquired contacts are distinguished from moving guides by observed target motion over several frames. IK weight 1 is not a support flag: a foot in transit also needs full pose control. Acquired contacts must remain within 5 cm; moving guides may be displaced by collision/anatomical constraints and are reported separately. Pelvis, knees and ankles that pass inside the landing footprint must remain above its surface.

`review_climbing_cycle.py` applies the joint checks to actual ascent/descent, unilateral hanging, manual regrips and both top transitions. Camera, obstacle-height and object-handling regressions are separate checks. Final measurements are in Saved/mantle_focus_review.json and Saved/climbing_cycle_review.json.

The initial catch uses a 0.4-second eased pose influence. The cached entry pose freezes immediately; the contact influence ramps separately. The exit tail is driven by its own blend amount rather than the grounded/crouched foot IK weights, so crouching cannot keep an old climbing pose active indefinitely.

`review_climbing_entry.py` tests the initial catch after repositioning has settled. The long cycle excludes only the first 0.12 seconds of its artificial fixture teleport; actual attachment is covered by the separate test with no teleport in its sampled interval.

Lowering starts with a separate 0.5-second eased turn on the landing. Foot targets follow that turning stance and retain full support immediately; only after facing the wall does the descent trajectory begin. This prevents a half-blended straight leg from sinking through the landing or swapping left/right foot targets during the turn. Knee clearance also applies against the active wall plane during ordinary climbing, not only against the top edge during mantles.

## Recorded verification (2026-09-09)

Final native build: `AltaiEditor Mac Development` succeeded. The long climbing cycle passed, including 151.55 cm upward movement, 130.28 cm downward movement, a one-hand/one-foot hang, manual restoration, mantle, grounded completion, reverse lowering and continued descent.

The final focused transition review passed signed knee/elbow flexion, hinge-plane alignment and frame rotation checks. Observed knee flexion remained approximately 2–142.4 degrees. Acquired contact error was at most 4.32 cm in the sampled descent and under 0.51 cm in ascent. Inside the landing footprint, the sampled joint-center clearances remained above 50.5 cm for the pelvis, 10 cm for knees and 7.9 cm for ankles. These measurements concern the current mannequin and test geometry; the data includes moving-guide errors separately.

First-person turn and view switching, obstacles of 45/100/160 cm and a separate initial-attachment review passed. Side and oblique visual inspections covered the crouched transfer and reverse transition; ordinary wall movement was also inspected. No production-quality claim is inferred solely from a successful capsule move or target-distance check.

The existing object-manipulation regression also passed after the final climbing changes. PIE was stopped and temporary slow motion/test settings were restored.

# Furniture interaction station

Map: `/Game/Altai/Debug/Maps/L_CharacterLab`, Outliner folder `11_HandInteraction`.
The workshop deck is south of the camp workbench and beam, around X 4780, Y -5480.

## Controls

- Aim at a handle, hold **LMB**, move the mouse down to pull / up to push. Release LMB to let go.
- **F** toggles the same grip for accessibility; F again releases it.
- **Left Ctrl** crouches to reach low handles; **V** switches first/third person.
- Walk with the handle when opening it farther than arm reach. The grip releases if the arm cannot follow.
- Existing wall controls and F pickup of loose objects remain available.

## Fixtures

- Double wooden door: two independently simulated leaves, 24 / 30 kg, 0–105° hinges.
- Chest: separate 9 kg lid, 0–100° hinge, with the lock plate moving with the lid.
- Cabinet: six independent 3.6–6.6 kg drawers, 0–36 cm rails. Each drawer has an open five-box collision shell, so loose contents can rest inside.

`AAltaiArticulatedProp` is a reusable native actor with an optional shared anchor actor, a mesh for the moving part, hinge/rail axis, travel, mass, and damping. Constraints attach to the actual physical frame/cabinet. They disable collisions only between the moving part and its anchor. Other scene objects and the player remain obstacles.

Dragging applies a bounded force at the authored `Grip_One` socket. It does not teleport a component or play a binary opening animation. The hand supports the gravity torque of a lid within the same force budget. On release there is no hand drive: gravity, damping, contacts, and mechanical limits govern motion. Furniture mass is not treated as a carried load. Camera look is temporarily held while dragging and restored on release. First-person camera forward offset eases from 30 to 10 cm during furniture grip, leaving space to see the hand at close range.

The existing post-physics contact solver follows the solved rigid body. Crouching lowers the pelvis, bends the legs through foot IK, and adjusts the first-person camera to the neck. This is a procedural test pose; fingers use the existing approximate curl, not individual collision constraints or finished authored grasp clips.

## Sources

Models and 2K PBR maps from Poly Haven, CC0:

- [Treasure chest](https://polyhaven.com/a/treasure_chest)
- [Vintage wooden drawer](https://polyhaven.com/a/vintage_wooden_drawer_01)
- [Large castle door](https://polyhaven.com/a/large_castle_door)

Original glTF files, API manifests, checksums, and textures are retained in `SourceArt/Environment/InteractionAssets`. Prepared meshes preserve the source UVs and separate movable parts; axes and pivots are authored for UE centimeters. Assets are under `/Game/Altai/Environment/Interaction`.

## Reproduction and verification

- `Scripts/Assets/download_interaction_assets.py`: download and verify originals.
- `Scripts/Assets/prepare_interaction_meshes.py`: Blender conversion and part manifest.
- `Scripts/Editor/install_interaction_furniture.py`: import materials/meshes and assemble stations.
- `Scripts/Editor/clear_furniture_deck.py`: board deck and relocated trees.
- `Scripts/Editor/refine_furniture_colliders.py`: leaf colliders exclude decorative lock protrusions to prevent false interlocking at the seam.
- `Scripts/Editor/validate_furniture_joints.py`: physical full-travel opening, release, and closing.
- `Scripts/Editor/validate_furniture_hands.py`: normal aim/grab/drag/release, wrist error, restored camera and carry speed.
- `Scripts/Editor/validate_furniture_collision.py`: external blocker, drawer independence, loose contents.
- `Scripts/Editor/validate_load_course.py`: regression for the existing 20 kg carry course.

Run the editor helpers through `Scripts/MCP/furniture_console.py` in a fresh PIE session. JSON evidence is saved under `Saved/furniture_*_validation.json`. Restore background throttling with `finish_hand_tests.py` after automated physics runs. Resetting the lab restores the original closed fixtures.

## Recorded results

The native Development Editor build succeeded. All nine moving parts passed full travel and return tests. All nine passed the normal hand interaction path, with camera input restored and carry speed scale 1.0. An external crate stopped a drawer at 27.7 cm; removing it allowed 36.0 cm. A loose stone settled inside its open collision shell; other drawers drifted less than 0.01 cm. The existing 20 kg beam/stair carry regression passed with maximum live wrist error 1.85 cm.

The detailed reports in Saved contain measured values and timestamps from the latest runs. Screenshots in docs/Images show the actual editor/PIE view.

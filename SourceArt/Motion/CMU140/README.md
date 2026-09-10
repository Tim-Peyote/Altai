# Recovery motion source

CMU Graphics Lab Motion Capture Database, subject 140:
- `140_01.amc`: Get Up Face Down
- `140_08.amc`: Get Up From Ground Laying on Back
- `140.asf`: source skeleton

Source: https://mocap.cs.cmu.edu/search.php?subjectnumber=140
Usage statement: https://mocap.cs.cmu.edu/ — “This dataset of motions is free for all uses.”
Capture supported by NSF Grant #0196217. Credit: Carnegie Mellon University, Graphics Lab.
Downloaded 2026-09-10 from the database's `/subjects/140/` links.

The JSON files are generated retargeted motion, not executable scripts. The retargeter preserves segment lengths and constructs shared knee/elbow flexion frames. It resamples at 30 Hz, trims the end of each take, applies a floor correction and bounded two-bone support correction for the lower foot (while preserving the other foot’s larger reach), and adds an explicitly authored one-second extension from the captured crouch to standing. The extension is not original CMU capture. Wrist/finger motion is currently neutralized rather than copied from the source's finger channels.

Rebuild: run `Scripts/Editor/export_body_rig.py` in lab PIE; then `Scripts/MCP/retarget_body_motion.py` with numpy/scipy; stop PIE and run `Scripts/Editor/import_body_motion.py`. The resulting native animation assets are in `/Game/Altai/Player/Animations/`.

Polish: the Acclaim-to-Manny coordinate conversion includes a handedness reflection; angular axes use the determinant sign. Knee/elbow axes come from the source skeleton’s declared hinge, including straight-leg frames. Support IK preserves that hinge direction. Lumbar and thoracic motion is distributed over the destination spine. Axial shoulder/hip rotation and ankle rotation are bounded, capture jitter is filtered, and the authored standing tail relaxes the arms. `Scripts/MCP/review_recovery_motion.py` checks initial prone/supine orientation, continuity, twist, hinge direction, and a supine back-bridge regression.

A bounded palm-to-floor wrist correction is applied near flat support, with axial wrist twist limited to 15 degrees and total wrist deviation to 70 degrees before smoothing. It does not perform runtime environmental hand-placement planning.

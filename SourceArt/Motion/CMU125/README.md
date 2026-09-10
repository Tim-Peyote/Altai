# Swimming motion references and authored clips

CMU Graphics Lab subject 125, take 01 (Breast Stroke):
https://mocap.cs.cmu.edu/search.php?subjectnumber=125
Downloaded 2026-09-10. Credit: Carnegie Mellon University Graphics Lab.
CMU usage statement: https://mocap.cs.cmu.edu/ (free for all uses).

The ASF/AMC files are reference data. The direct retarget trial was rejected because of abrupt pose changes. The shipped JSON/native clips are authored on Manny with constrained limb targets and positive elbow/knee hinges. They are NOT underwater mocap. Swim England's pull/breathe/kick/glide sequence informs arm/leg timing:
https://www.swimming.org/masters/improving-your-breaststroke-technique/
https://www.swimming.org/justswim/tips-getting-breaststroke-right/

Rebuild with Scripts/MCP/author_swim_motion.py, validate with review_swim_motion.py, then import outside PIE using Scripts/Editor/import_swim_motion.py. SwimEasy is an authored shorter recovery stroke; it is not a named competition technique.

SwimDrown is an authored non-looping relaxation pose transition, not CMU drowning mocap. It gradually lowers the hands and relaxes the leg tuck without reversing limb hinges.

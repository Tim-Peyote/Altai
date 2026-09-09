import runpy
from pathlib import Path
runpy.run_path(str(Path(__file__).with_name('review_anatomical_grips.py')),init_globals={'ANATOMY_TEST_CASES':[(.8,'SM_FieldStick')],'ANATOMY_REPORT_NAME':'anatomical_rod_review.json'})

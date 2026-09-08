import runpy
from pathlib import Path
p=Path(__file__).parent
runpy.run_path(str(p/'add_lab_canopy.py'),run_name='__main__')
runpy.run_path(str(p/'finalize_lab.py'),run_name='__main__')

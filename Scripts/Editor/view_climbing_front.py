"""Change only the review camera; safe alongside a slow-motion callback."""
import unreal
from altai_lab_tools import context
front_review_world,front_review_controller=context()
front_review_controller.set_control_rotation(unreal.Rotator(pitch=-12,yaw=135,roll=0))

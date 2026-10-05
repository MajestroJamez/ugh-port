"""Where things are in the copter, shared by copter.py (the copter) and caveman.py (its pilot's pedalling).

Metres; the copter's origin is the bottom middle of its body (UGH_LOGIC_COPTER_BODY_*: 22 x 20 px, 1 px = 0.1 m),
X right, Z up, the front of the copter towards -Y (the camera). The game (UghCopterModel.h) places the parts by the same
numbers.
"""
import math

BODY_WIDTH, BODY_HEIGHT = 2.2, 2.0
# the cage stays this far in front of and behind the plane of the play (the rock's face is 0.6 m in front of it)
BODY_DEPTH = 0.45

# the people in the copter are this tall (UghFigurePlace::PersonHeight, walking as tall as a passenger's sprite): a
# model of another height is scaled to it, its actions reach the crank and the handles at their places here divided by that scale
PERSON_HEIGHT = 1.45

# the pilot sits here (the origin of his sitting actions), the passenger behind him, a little higher; each turned
# (degrees about Z, see turned) from facing the front: the pilot to the left (-X), the passenger to the right, so that
# the camera sees them sitting from a side
PILOT_SEAT = (-0.28, 0.0, 0.52)
PASSENGER_SEAT = (0.42, 0.2, 0.6)
PILOT_YAW, PASSENGER_YAW = -50.0, 50.0

# in the pilot's own frame (from his seat, facing -Y; turned by PILOT_YAW in the copter): the crank he pedals, its
# axle (across, X), the pedals' radius and how far each is from the middle of the axle; the handles he holds (the left
# one; the right one mirrored)
PEDAL_AXLE = (0.0, -0.32, -0.22)
PEDAL_RADIUS = 0.08
PEDAL_SPREAD = 0.11
GRIP = (0.2, -0.32, 0.34)

# the rotor turns about a vertical axis through this point
ROTOR_HUB = (0.0, 0.0, 1.87)

# the stone passenger that hangs below (16 x 11 px, its top 1 px below the body): the middle and half the size of the
# boulder (stone_passenger.py) the sling holds (copter.py)
HANGING_MIDDLE = (0.0, 0.0, -0.63)
HANGING_SEMI = (0.79, 0.42, 0.53)


def turned(offset, yaw):
    """`offset` (x, y, z) of a seat's own frame in the copter's frame: turned by `yaw` degrees about Z (X to Y)."""
    c, s = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
    return (offset[0] * c - offset[1] * s, offset[0] * s + offset[1] * c, offset[2])

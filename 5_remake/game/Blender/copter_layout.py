"""Where things are in the copter, shared by copter.py (the copter) and caveman.py (its pilot's pedalling).

Metres; the copter's origin is the bottom middle of its body (UGH_LOGIC_COPTER_BODY_*: 22 x 20 px, 1 px = 0.1 m),
X right, Z up, the front (where the pilot looks) towards -Y. The game (UghCopterModel.h) places the parts by the same
numbers.
"""

BODY_WIDTH, BODY_HEIGHT = 2.2, 2.0
# the cage stays this far in front of and behind the plane of the play (the rock's face is 0.6 m in front of it)
BODY_DEPTH = 0.45

# the pilot sits here (the origin of his sitting actions), the passenger behind him, a little higher
PILOT_SEAT = (-0.28, -0.08, 0.52)
PASSENGER_SEAT = (0.42, 0.2, 0.6)

# the crank the pilot pedals: its axle (across, X) from the pilot's seat, the pedals' radius and how far each is from
# the middle of the axle; the handles he holds, from his seat (the left one; the right one mirrored)
PEDAL_AXLE = (0.0, -0.2, -0.17)
PEDAL_RADIUS = 0.08
PEDAL_SPREAD = 0.11
GRIP = (0.22, -0.2, 0.3)

# the rotor turns about a vertical axis through this point
ROTOR_HUB = (0.0, 0.0, 1.87)

# the stone passenger that hangs below (16 x 11 px, its top 1 px below the body): the middle and half the size of the
# boulder (stone_passenger.py) the sling holds (copter.py)
HANGING_MIDDLE = (0.0, 0.0, -0.63)
HANGING_SEMI = (0.79, 0.42, 0.53)

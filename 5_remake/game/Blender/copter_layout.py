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
# (degrees about Z, see turned) from facing the front: the pilot nearly to the left (-X), seen pedalling almost from
# his side, the passenger to the right, so that the camera sees them sitting from a side
PILOT_SEAT = (-0.28, 0.07, 0.6)
PASSENGER_SEAT = (0.42, 0.15, 0.6)
PILOT_YAW, PASSENGER_YAW = -70.0, 50.0

# in the pilot's own frame (from his seat, facing -Y; turned by PILOT_YAW in the copter): the crank he pedals, its
# axle (across, X), the pedals' radius and how far each is from the middle of the axle; the handles he holds (the left
# one; the right one mirrored)
PEDAL_AXLE = (0.0, -0.34, -0.25)
PEDAL_RADIUS = 0.12
PEDAL_SPREAD = 0.11
GRIP = (0.22, -0.36, 0.36)

# the rotor turns about a vertical axis through this point
ROTOR_HUB = (0.0, 0.0, 1.87)

# The drive from the crank to the rotor: a chain from a chainring on the crank's axle (on the pilot's left, +X of his
# frame: the camera's side) up to a sprocket on a layshaft across the top of the cage, parallel to the axle; the
# layshaft's lantern pinion turns the crown wheel on the rotor's shaft. The chain's plane is square to the axle; the
# layshaft points at the rotor's axis, the pinion meshing with the crown at CROWN_RADIUS from it. The chainring has
# RATIO times the sprocket's teeth and the pinion as many pegs as the crown, so that the rotor turns RATIO times for a
# turn of the crank (UghRotorSpin::RotorTurnsPerPedal).
CHAINRING_SIDE = 0.32   # from the middle of the axle along it
CHAINRING_RADIUS, CHAINRING_TEETH = 0.18, 18
RATIO = 3
SPROCKET_RADIUS = CHAINRING_RADIUS / RATIO
LAYSHAFT_HEIGHT = 1.3
CROWN_RADIUS = 0.1   # and the pinion's
CROWN_PEGS = 10
# the chain's links (one mesh each, moved along the chain by the game): this far apart along it at most
CHAIN_PITCH = 2 * math.pi * CHAINRING_RADIUS / CHAINRING_TEETH

# the stone passenger that hangs below (16 x 11 px, its top 1 px below the body): the middle and half the size of the
# boulder (stone_passenger.py) the sling holds (copter.py)
HANGING_MIDDLE = (0.0, 0.0, -0.63)
HANGING_SEMI = (0.79, 0.42, 0.53)


def turned(offset, yaw):
    """`offset` (x, y, z) of a seat's own frame in the copter's frame: turned by `yaw` degrees about Z (X to Y)."""
    c, s = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
    return (offset[0] * c - offset[1] * s, offset[0] * s + offset[1] * c, offset[2])


def of_pilot(offset):
    """A point of the pilot's frame (from his seat) in the copter."""
    return tuple(a + b for a, b in zip(PILOT_SEAT, turned(offset, PILOT_YAW)))


def crank_axle():
    """The middle of the crank's axle in the copter."""
    return of_pilot(PEDAL_AXLE)


def chainring():
    """The chainring's middle in the copter."""
    return of_pilot((CHAINRING_SIDE, PEDAL_AXLE[1], PEDAL_AXLE[2]))


def sprocket():
    """The sprocket's middle in the copter: in the chain's plane, its layshaft (along the crank's axle) pointing at the
    rotor's axis, LAYSHAFT_HEIGHT high."""
    across = turned((1, 0, 0), PILOT_YAW)
    ring = chainring()
    out = ring[0] * across[0] + ring[1] * across[1]   # how far the chain's plane is from the rotor's axis
    return (across[0] * out, across[1] * out, LAYSHAFT_HEIGHT)

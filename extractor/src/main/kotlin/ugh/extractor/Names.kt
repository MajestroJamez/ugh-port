package ugh.extractor

/*
 * The names of the game's data for everything outside the DOS address space (UGD 1 data, UGR 1 replays): the one
 * table from the original's DGROUP offsets to names. The C++ logic and the semantic replays know only these names.
 */
object Names {
    /** Passenger kinds by descriptor (0x4a bytes each): three walking kinds, their swimming twins, the standing one. */
    val PASSENGER_KINDS = linkedMapOf(
        0x7720 to "kind1", 0x776a to "kind1-water",
        0x77b4 to "kind2", 0x77fe to "kind2-water",
        0x7848 to "kind3", 0x7892 to "kind3-water",
        0x78dc to "standing",
    )

    /** Enemy kinds by descriptor (0x3c bytes each); any other descriptor in a level list is a walker (113b:3b21). */
    const val FLYER = 0x7630
    const val WALKER = 0x766c
    const val BLOWER = 0x76a8
    const val TREE = 0x76e4
    val ENEMY_KINDS = linkedMapOf(FLYER to "flyer", WALKER to "walker", BLOWER to "blower", TREE to "tree")

    /** Bonus item kinds by descriptor (0x14 bytes each, 7970 .. 7a38): nine energy kinds, a life, the multiplier. */
    val BONUS_KINDS: Map<Int, String> = linkedMapOf<Int, String>().apply {
        for (i in 0 until 9) put(0x7970 + 0x14 * i, "energy${i + 1}")
        put(0x7a24, "life")
        put(0x7a38, "multiplier")
    }

    fun passengerKind(descriptor: Int) = PASSENGER_KINDS[descriptor] ?: error("unknown passenger kind ${descriptor.hex()}")
    fun enemyKind(descriptor: Int) = ENEMY_KINDS[descriptor] ?: "walker"
    fun bonusKind(descriptor: Int) = BONUS_KINDS[descriptor] ?: error("unknown bonus kind ${descriptor.hex()}")

    /** The animation slots of a passenger descriptor: offset -> name (with the kind's name before it). */
    val PASSENGER_ANIMATIONS = linkedMapOf(
        0x2e to "standing", 0x30 to "waving", 0x32 to "walkLeft", 0x34 to "walkRight", 0x36 to "comingOut", 0x38 to "goingIn",
    )

    /** The animation slots of an enemy descriptor that the enemies use: offset -> name. */
    val ENEMY_ANIMATIONS = linkedMapOf(
        0x22 to "left", 0x24 to "right", 0x26 to "chargeLeft", 0x28 to "chargeRight", 0x2a to "recoverLeft",
        0x2c to "recoverRight", 0x2e to "watchLeft", 0x30 to "watchRight", 0x36 to "stunnedLeft", 0x38 to "stunnedRight",
    )
}

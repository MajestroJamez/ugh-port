/*
 * UGH! game logic - C API of the C++ core (C++20, no dependencies).
 *
 * The interface is the semantic game state of the golden replays (format "UGR 0", see
 * verify/src/test/kotlin/ugh/verify/replay/ReplayWriter.kt): named fields such as "copter.0.xf" with the same
 * values and units as the original (positions in 1/32 px, handlers and data by their DGROUP offsets). Fields
 * the core does not know (not set and not computed yet) are left out of ugh_sim_fields. Inside, the state is
 * kept like the original keeps it (DGROUP at the original offsets).
 *
 * Implemented so far (plan step 7): new game, level start (113b:3d66 / 3976), level end and the whole play frame
 * (copter physics 113b:1095, passengers 1486, objects 2363, bonus items 2b7f, water, rain, keyboard, random
 * numbers). Not yet: the caption, setup and fade frames between the levels, game over (plan step 8).
 */
#ifndef UGH_SIM_H
#define UGH_SIM_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ugh_sim ugh_sim;

/** Loads the data exported by the extractor (assets/sim/ugh-sim.bin). NULL on failure, reason in err. */
ugh_sim* ugh_sim_create(const char* data_path, char* err, size_t err_size);
void ugh_sim_destroy(ugh_sim* sim);

/** Back to the program start: the initialized memory, every field unknown. */
void ugh_sim_reset(ugh_sim* sim);

/** Forgets every field of the replay state; what the replay does not hold (the raindrops, the keyboard handler) is kept. */
void ugh_sim_clear(ugh_sim* sim);

/** 1 if the core has the stage of the play frame (the B lines of the replays: passengers, objects, bonuses). */
int ugh_sim_has_stage(const char* stage);

/** Sets a field from its replay text. 1 = set, 0 = not a field of the core (ignored), -1 = bad value. */
int ugh_sim_set(ugh_sim* sim, const char* field, const char* value);

/** Calls back with what the core met and does not support since the last call (and forgets it). */
void ugh_sim_take_problems(ugh_sim* sim, void (*callback)(void* ctx, const char* problem), void* ctx);

/** Calls back with every known field of the state. */
void ugh_sim_fields(const ugh_sim* sim, void (*callback)(void* ctx, const char* field, const char* value), void* ctx);

/** A scancode delivered between frames (keyboard handler 113b:4567). */
void ugh_sim_key(ugh_sim* sim, int scancode);

/** 113b:3961: 3 lives, multiplier 1, score 0. */
void ugh_sim_new_game(ugh_sim* sim);

/** After a level (113b:0fa7): next level or one life less. */
enum { UGH_SIM_CONTINUE = 0, UGH_SIM_GAME_OVER = 1, UGH_SIM_ALL_LEVELS_DONE = 2 };
int ugh_sim_level_end(ugh_sim* sim);

/** Level setup up to the caption: state, level load, the caption's water row. */
void ugh_sim_level_start(ugh_sim* sim);

/** One frame of the level play (113b:0c7d .. 0fa4, without the retrace wait). */
void ugh_sim_play_frame(ugh_sim* sim);

#ifdef __cplusplus
}
#endif

#endif

/* Which player a newly connected gamepad plays (runtime_pad.c), from the playerN_controller settings. */
#ifndef DCP_PAD_SLOTS_H
#define DCP_PAD_SLOTS_H

#include <stdbool.h>
#include <string.h>

/* wanted[i]: player i+1's setting (empty for any gamepad, a "vendor:product" model, or "none"); taken[i]: player i+1
 * has a gamepad. The first free player naming this model, else the first free player naming none; -1 when none is free. */
static inline int pad_slot_for(const char (*wanted)[64], const bool *taken, int players, const char *model) {
    for (int i = 0; i < players; ++i)
        if (!taken[i] && !strcmp(wanted[i], model)) return i;
    for (int i = 0; i < players; ++i)
        if (!taken[i] && !*wanted[i]) return i;
    return -1;
}

#endif

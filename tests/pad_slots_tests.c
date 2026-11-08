/* Focused test of which player a new gamepad plays (src/pad_slots.h); tools/test.ps1 runs it. */
#include <stdio.h>

#include "pad_slots.h"

static int failures;
static void expect(int got, int want, const char *what) {
    if (got != want) {
        printf("FAIL %s: player index %d, expected %d\n", what, got, want);
        ++failures;
    }
}

int main(void) {
    const char automatic[4][64] = {"", "", "", ""};
    bool none_taken[4] = {false, false, false, false};
    expect(pad_slot_for(automatic, none_taken, 4, "054c:09cc"), 0, "automatic: first free player");

    bool first_taken[4] = {true, false, false, false};
    expect(pad_slot_for(automatic, first_taken, 4, "054c:09cc"), 1, "automatic: next free player");

    const char mapped[4][64] = {"", "045e:02ea", "054c:09cc", ""};
    expect(pad_slot_for(mapped, none_taken, 4, "054c:09cc"), 2, "a mapped controller takes its player");
    expect(pad_slot_for(mapped, none_taken, 4, "057e:2009"), 0, "an unmapped controller skips the mapped players");

    const char keyboard_first[4][64] = {"none", "", "054c:09cc", "054c:09cc"};
    expect(pad_slot_for(keyboard_first, none_taken, 4, "045e:02ea"), 1, "player 1 set to none takes no gamepad");
    bool third_taken[4] = {false, false, true, false};
    expect(pad_slot_for(keyboard_first, third_taken, 4, "054c:09cc"), 3, "a second identical controller takes the next player");

    const char all_mapped[4][64] = {"none", "054c:09cc", "054c:09cc", "054c:09cc"};
    bool all_taken[4] = {false, true, true, true};
    expect(pad_slot_for(all_mapped, all_taken, 4, "045e:02ea"), -1, "no free player");

    if (!failures) puts("pad slot tests passed");
    return failures ? 1 : 0;
}

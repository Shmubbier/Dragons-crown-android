/* On-screen touch controls for the Android build: one layout for input (src/runtime_pad.c) and drawing
 * (gpu/shim/dcp_overlay.cpp). Positions are fractions of the window's width and height; radii are fractions
 * of its height, so controls stay round on any aspect ratio.
 *
 * user/touch_layout.ini changes them, one line per control: "<id> = <x> <y> <size> <opacity> <enabled>" (size: times
 * the default radius; opacity 0-1; enabled 0 or 1). The app's layout editor writes it (TouchLayoutActivity.java, whose
 * default table must match this one). */
#ifndef DCP_TOUCH_LAYOUT_H
#define DCP_TOUCH_LAYOUT_H
#include <stdint.h>

enum DcpTouchShape { DCP_TOUCH_CROSS, DCP_TOUCH_CIRCLE, DCP_TOUCH_SQUARE, DCP_TOUCH_TRIANGLE, DCP_TOUCH_LABEL, DCP_TOUCH_ARROW,
                     DCP_TOUCH_GEAR, DCP_TOUCH_LEFT_STICK, DCP_TOUCH_RIGHT_STICK, DCP_TOUCH_PAD };

/* Not a pad button: opens the app's settings (DcpActivity), and stays on screen when the other controls are hidden. */
#define DCP_TOUCH_SETTINGS 0x80000000u
#define DCP_ANDROID_OPEN_SETTINGS 0x8001 /* SDL_SendAndroidMessage command, SDLActivity.onUnhandledMessage */

typedef struct {
    const char *id;       /* its name in touch_layout.ini */
    float x, y, r;        /* centre (fractions of width, height) and radius (fraction of height) */
    uint32_t button;      /* libScePad button bit (runtime_pad.c BTN_*); 0 for the sticks and the touchpad */
    int shape;            /* DcpTouchShape */
    const char *label;    /* DCP_TOUCH_LABEL text; DCP_TOUCH_ARROW: "u", "d", "l" or "r" */
    float opacity;        /* 0-1 */
    int enabled;
} DcpTouchButton;

/* The left stick floats: a finger that lands in the left half, below the top bar and away from any button, becomes
 * its centre; x, y place the hint ring. The right stick is fixed where it is placed. The touchpad has no place: any
 * other touch on the screen is a touch on the PS4 touchpad. */
static const DcpTouchButton dcp_touch_defaults[] = {
    {"cross", 0.885f, 0.80f, 0.085f, 0x4000, DCP_TOUCH_CROSS, 0, 1.0f, 1},
    {"circle", 0.955f, 0.62f, 0.085f, 0x2000, DCP_TOUCH_CIRCLE, 0, 1.0f, 1},
    {"square", 0.815f, 0.62f, 0.085f, 0x8000, DCP_TOUCH_SQUARE, 0, 1.0f, 1},
    {"triangle", 0.885f, 0.44f, 0.085f, 0x1000, DCP_TOUCH_TRIANGLE, 0, 1.0f, 1},
    {"l1", 0.055f, 0.10f, 0.065f, 0x0400, DCP_TOUCH_LABEL, "L1", 1.0f, 1},
    {"l2", 0.145f, 0.10f, 0.065f, 0x0100, DCP_TOUCH_LABEL, "L2", 1.0f, 1},
    {"r1", 0.945f, 0.10f, 0.065f, 0x0800, DCP_TOUCH_LABEL, "R1", 1.0f, 1},
    {"r2", 0.855f, 0.10f, 0.065f, 0x0200, DCP_TOUCH_LABEL, "R2", 1.0f, 1},
    {"back", 0.42f, 0.07f, 0.055f, 0x100000, DCP_TOUCH_LABEL, "Back", 1.0f, 1}, /* touchpad click (left side) */
    {"start", 0.58f, 0.07f, 0.055f, 0x0008, DCP_TOUCH_LABEL, "Start", 1.0f, 1}, /* Options */
    {"settings", 0.28f, 0.07f, 0.045f, DCP_TOUCH_SETTINGS, DCP_TOUCH_GEAR, 0, 1.0f, 1},
    {"up", 0.105f, 0.30f, 0.050f, 0x0010, DCP_TOUCH_ARROW, "u", 1.0f, 1},
    {"down", 0.105f, 0.50f, 0.050f, 0x0040, DCP_TOUCH_ARROW, "d", 1.0f, 1},
    {"left", 0.050f, 0.40f, 0.050f, 0x0080, DCP_TOUCH_ARROW, "l", 1.0f, 1},
    {"right", 0.160f, 0.40f, 0.050f, 0x0020, DCP_TOUCH_ARROW, "r", 1.0f, 1},
    {"left_stick", 0.26f, 0.70f, 0.14f, 0, DCP_TOUCH_LEFT_STICK, 0, 1.0f, 1},
    {"right_stick", 0.66f, 0.78f, 0.11f, 0, DCP_TOUCH_RIGHT_STICK, 0, 1.0f, 1},
    {"touchpad", 0.0f, 0.0f, 0.0f, 0, DCP_TOUCH_PAD, 0, 1.0f, 1},
};
#define DCP_TOUCH_BUTTON_COUNT (sizeof(dcp_touch_defaults) / sizeof(dcp_touch_defaults[0]))

#define DCP_TOUCH_STICK_MAX_X 0.50f   /* the left stick's area: left of this */
#define DCP_TOUCH_STICK_MIN_Y 0.25f   /* and below this */
#define DCP_TOUCH_HIT_SCALE 1.25f     /* buttons accept touches a little outside their drawn circle */

#ifdef __cplusplus
extern "C" {
#endif
/* Shared with the overlay (src/runtime_pad.c): the layout in use (DCP_TOUCH_BUTTON_COUNT entries), whether to draw the
 * controls, which buttons are held, and each stick's origin and knob in window fractions (active 0 when no finger holds
 * it; stick 0 is the left one). */
const DcpTouchButton *dcp_touch_layout(void);
int dcp_touch_controls_visible(void);
uint32_t dcp_touch_held_buttons(void);
int dcp_touch_stick(int stick, float *origin_x, float *origin_y, float *knob_x, float *knob_y);
#ifdef __cplusplus
}
#endif
#endif

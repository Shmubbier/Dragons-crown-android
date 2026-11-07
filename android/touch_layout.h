/* On-screen touch controls for the Android build: one layout for input (src/runtime_pad.c) and drawing
 * (gpu/shim/dcp_overlay.cpp). Positions are fractions of the window's width and height; radii are fractions
 * of its height, so controls stay round on any aspect ratio. */
#ifndef DCP_TOUCH_LAYOUT_H
#define DCP_TOUCH_LAYOUT_H
#include <stdint.h>

enum DcpTouchShape { DCP_TOUCH_CROSS, DCP_TOUCH_CIRCLE, DCP_TOUCH_SQUARE, DCP_TOUCH_TRIANGLE, DCP_TOUCH_LABEL, DCP_TOUCH_ARROW };

typedef struct {
    float x, y, r;        /* centre (fractions of width, height) and radius (fraction of height) */
    uint32_t button;      /* libScePad button bit (runtime_pad.c BTN_*) */
    int shape;            /* DcpTouchShape */
    const char *label;    /* DCP_TOUCH_LABEL text; DCP_TOUCH_ARROW: "u", "d", "l" or "r" */
} DcpTouchButton;

static const DcpTouchButton dcp_touch_buttons[] = {
    {0.885f, 0.80f, 0.085f, 0x4000, DCP_TOUCH_CROSS, 0},      /* Cross */
    {0.955f, 0.62f, 0.085f, 0x2000, DCP_TOUCH_CIRCLE, 0},     /* Circle */
    {0.815f, 0.62f, 0.085f, 0x8000, DCP_TOUCH_SQUARE, 0},     /* Square */
    {0.885f, 0.44f, 0.085f, 0x1000, DCP_TOUCH_TRIANGLE, 0},   /* Triangle */
    {0.055f, 0.10f, 0.065f, 0x0400, DCP_TOUCH_LABEL, "L1"},
    {0.145f, 0.10f, 0.065f, 0x0100, DCP_TOUCH_LABEL, "L2"},
    {0.945f, 0.10f, 0.065f, 0x0800, DCP_TOUCH_LABEL, "R1"},
    {0.855f, 0.10f, 0.065f, 0x0200, DCP_TOUCH_LABEL, "R2"},
    {0.42f, 0.07f, 0.055f, 0x100000, DCP_TOUCH_LABEL, "Back"}, /* touchpad click (left side) */
    {0.58f, 0.07f, 0.055f, 0x0008, DCP_TOUCH_LABEL, "Start"},  /* Options */
    {0.105f, 0.30f, 0.050f, 0x0010, DCP_TOUCH_ARROW, "u"},
    {0.105f, 0.50f, 0.050f, 0x0040, DCP_TOUCH_ARROW, "d"},
    {0.050f, 0.40f, 0.050f, 0x0080, DCP_TOUCH_ARROW, "l"},
    {0.160f, 0.40f, 0.050f, 0x0020, DCP_TOUCH_ARROW, "r"},
};
#define DCP_TOUCH_BUTTON_COUNT (sizeof(dcp_touch_buttons) / sizeof(dcp_touch_buttons[0]))

/* A finger that lands in this region away from any button becomes a floating left stick centred where it landed. */
#define DCP_TOUCH_STICK_MAX_X 0.50f
#define DCP_TOUCH_STICK_MIN_Y 0.25f
#define DCP_TOUCH_STICK_RADIUS 0.14f  /* full deflection, fraction of height */
#define DCP_TOUCH_HIT_SCALE 1.25f     /* buttons accept touches a little outside their drawn circle */

#ifdef __cplusplus
extern "C" {
#endif
/* Shared with the overlay (src/runtime_pad.c): whether to draw the controls, which buttons are held, and the
 * stick's origin and knob in window fractions (active 0 when no finger holds it). */
int dcp_touch_controls_visible(void);
uint32_t dcp_touch_held_buttons(void);
int dcp_touch_stick(float *origin_x, float *origin_y, float *knob_x, float *knob_y);
#ifdef __cplusplus
}
#endif
#endif

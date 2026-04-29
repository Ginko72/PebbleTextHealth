#include "geometry.h"

// Quarter-circle arc length: pi*r/2, approximated as r*355/226.
#define QUARTER_ARC(r) ((uint16_t)((uint32_t)(r) * 355 / 226))

// Point on a corner arc at Pebble TRIG_ANGLE `angle` from corner center.
static GPoint corner_arc_point(GPoint center, uint16_t r, int32_t angle) {
    return (GPoint) {
        .x = center.x + (int16_t)((int32_t)sin_lookup(angle) * r / TRIG_MAX_RATIO),
        .y = center.y - (int16_t)((int32_t)cos_lookup(angle) * r / TRIG_MAX_RATIO)
    };
}

// TRIG_ANGLE delta for distance `d` along a corner arc of radius `r`.
// d * TRIG_MAX_ANGLE * 113 / (710 * r)  [no overflow for d<=47, r>=4]
static int32_t arc_angle_delta(uint16_t d, uint16_t r) {
    return (int32_t)d * TRIG_MAX_ANGLE * 113 / ((int32_t)710 * r);
}


uint16_t rounded_rect_perimeter(GRect rect, uint16_t r) {
    uint16_t w = (uint16_t)rect.size.w;
    uint16_t h = (uint16_t)rect.size.h;
    uint16_t straights = 2 * (w - 2 * r) + 2 * (h - 2 * r);
    uint16_t corners   = (uint16_t)((uint32_t)4 * r * 355 / 226);
    return straights + corners;
}


GPoint rounded_rect_perimeter_point(GRect rect, uint16_t r,
                                    uint16_t perimeter_length,
                                    uint16_t distance) {
    (void)perimeter_length;  // used by callers for allocation; unused here

    int16_t x0 = rect.origin.x;
    int16_t y0 = rect.origin.y;
    int16_t w  = rect.size.w;
    int16_t h  = rect.size.h;
    int16_t cx = x0 + w / 2;

    uint16_t qa = QUARTER_ARC(r);
    // Straight segment lengths
    uint16_t top_half  = (uint16_t)(w / 2 - r);    // cx → right corner start
    uint16_t right_seg = (uint16_t)(h - 2 * r);
    uint16_t bot_seg   = (uint16_t)(w - 2 * r);
    uint16_t left_seg  = right_seg;

    // Cumulative segment end distances (clockwise from top-center)
    uint16_t d0 = top_half;
    uint16_t d1 = d0 + qa;
    uint16_t d2 = d1 + right_seg;
    uint16_t d3 = d2 + qa;
    uint16_t d4 = d3 + bot_seg;
    uint16_t d5 = d4 + qa;
    uint16_t d6 = d5 + left_seg;
    uint16_t d7 = d6 + qa;
    // d7 + top_half = full perimeter (top-left corner end → cx)

    // Corner arc centers
    GPoint tr_c = { x0 + w - r,   y0 + r         };  // top-right
    GPoint br_c = { x0 + w - r,   y0 + h - r     };  // bottom-right
    GPoint bl_c = { x0 + r,       y0 + h - r     };  // bottom-left
    GPoint tl_c = { x0 + r,       y0 + r         };  // top-left

    if (distance < d0) {
        // Top-right straight: (cx, y0) → (x0+w-r, y0)
        return (GPoint){ cx + (int16_t)distance, y0 };

    } else if (distance < d1) {
        // Top-right arc: angle 0 (north) → TRIG_MAX_ANGLE/4 (east)
        int32_t angle = arc_angle_delta((uint16_t)(distance - d0), r);
        return corner_arc_point(tr_c, r, angle);

    } else if (distance < d2) {
        // Right edge: (x0+w, y0+r) → (x0+w, y0+h-r)
        return (GPoint){ x0 + w, y0 + r + (int16_t)(distance - d1) };

    } else if (distance < d3) {
        // Bottom-right arc: TRIG_MAX_ANGLE/4 → TRIG_MAX_ANGLE/2
        int32_t angle = TRIG_MAX_ANGLE / 4 + arc_angle_delta((uint16_t)(distance - d2), r);
        return corner_arc_point(br_c, r, angle);

    } else if (distance < d4) {
        // Bottom edge: (x0+w-r, y0+h) → (x0+r, y0+h)
        return (GPoint){ x0 + w - r - (int16_t)(distance - d3), y0 + h };

    } else if (distance < d5) {
        // Bottom-left arc: TRIG_MAX_ANGLE/2 → 3*TRIG_MAX_ANGLE/4
        int32_t angle = TRIG_MAX_ANGLE / 2 + arc_angle_delta((uint16_t)(distance - d4), r);
        return corner_arc_point(bl_c, r, angle);

    } else if (distance < d6) {
        // Left edge: (x0, y0+h-r) → (x0, y0+r)
        return (GPoint){ x0, y0 + h - r - (int16_t)(distance - d5) };

    } else if (distance < d7) {
        // Top-left arc: 3*TRIG_MAX_ANGLE/4 → TRIG_MAX_ANGLE
        int32_t angle = 3 * TRIG_MAX_ANGLE / 4 + arc_angle_delta((uint16_t)(distance - d6), r);
        return corner_arc_point(tl_c, r, angle);

    } else {
        // Top-left straight: (x0+r, y0) → cx
        return (GPoint){ x0 + r + (int16_t)(distance - d7), y0 };
    }
}


uint16_t trig_angle_from_center(GPoint point, GPoint center) {
    int dx = point.x - center.x;
    int dy = point.y - center.y;  // positive = downward in screen coords

    // Pebble TRIG_ANGLE: 0 = north (up), TRIG_MAX_ANGLE/4 = east, clockwise.
    // sin(angle) ~ rightward component, cos(angle) ~ upward component (-dy).
    // Binary search for angle in the correct quadrant.
    int lo, hi;
    if      (dx >= 0 && dy <= 0) { lo = 0;                    hi = TRIG_MAX_ANGLE / 4; }
    else if (dx >= 0 && dy >  0) { lo = TRIG_MAX_ANGLE / 4;   hi = TRIG_MAX_ANGLE / 2; }
    else if (dx <  0 && dy >  0) { lo = TRIG_MAX_ANGLE / 2;   hi = 3 * TRIG_MAX_ANGLE / 4; }
    else                          { lo = 3 * TRIG_MAX_ANGLE / 4; hi = TRIG_MAX_ANGLE; }

    for (int i = 0; i < 20; i++) {
        int mid = (lo + hi) / 2;
        // sin(mid)*(-dy) > cos(mid)*dx  →  mid is too large
        if ((int32_t)sin_lookup(mid) * (-dy) > (int32_t)cos_lookup(mid) * dx) {
            hi = mid;
        } else {
            lo = mid;
        }
    }
    return (uint16_t)((lo + hi) / 2);
}

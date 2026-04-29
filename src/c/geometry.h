#pragma once
#include <pebble.h>

// Total perimeter of a rounded rectangle in integer pixels.
// Uses 355/113 integer approximation for pi.
uint16_t rounded_rect_perimeter(GRect rect, uint16_t corner_radius);

// GPoint at `distance` pixels clockwise along the perimeter, starting at
// top-center. `perimeter_length` must be the value returned by
// rounded_rect_perimeter() for the same rect and corner_radius.
GPoint rounded_rect_perimeter_point(GRect rect, uint16_t corner_radius,
                                    uint16_t perimeter_length,
                                    uint16_t distance);

// TRIG_ANGLE (0=12-o'clock, clockwise) of `point` as seen from `center`.
// Computed via binary search using sin_lookup/cos_lookup; no floating point.
uint16_t trig_angle_from_center(GPoint point, GPoint center);

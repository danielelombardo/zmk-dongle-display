/*
 *
 * Copyright (c) 2024 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>

#define CANVAS_WIDTH 68
#define CANVAS_HEIGHT 160

#define LVGL_BACKGROUND                                                        \
  IS_ENABLED(CONFIG_NICE_VIEW_WIDGET_INVERTED) ? lv_color_black()              \
                                               : lv_color_white()
#define LVGL_FOREGROUND                                                        \
  IS_ENABLED(CONFIG_NICE_VIEW_WIDGET_INVERTED) ? lv_color_white()              \
                                               : lv_color_black()



void draw_battery_status(lv_obj_t *canvas, const struct status_state *state);


int zmk_widget_battery_init(struct zmk_widget_battery *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_battery_obj(struct zmk_widget_battery *widget);

int zmk_widget_layer_init(struct zmk_widget_layer *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_layer_obj(struct zmk_widget_layer *widget);

int zmk_widget_modifiers_init(struct zmk_widget_modifiers *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_modifiers_obj(struct zmk_widget_modifiers *widget);

int zmk_widget_output_init(struct zmk_widget_output *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_output_obj(struct zmk_widget_output *widget);



int zmk_widget_screen_init(struct zmk_widget_screen *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_screen_obj(struct zmk_widget_screen *widget);

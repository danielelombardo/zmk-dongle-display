/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "custom_screen.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include "assets/space_mono_14.c"
#include "assets/fonts.h"

static struct zmk_widget_output output_widget;
static struct zmk_widget_layer layer_widget;
static struct zmk_widget_battery battery_widget;
static struct zmk_widget_modifiers modifiers_widget;


lv_style_t global_style;

lv_obj_t *zmk_display_screen() {
    lv_obj_t *screen;

    screen = lv_obj_create(NULL);

    lv_style_init(&global_style);
    lv_style_set_text_font(&global_style, &space_mono_14);
    lv_style_set_text_letter_space(&global_style, 1);
    lv_style_set_text_line_space(&global_style, 1);
    lv_obj_add_style(screen, &global_style, LV_PART_MAIN);
    
    zmk_widget_output_init(&output_widget, screen);
    lv_obj_align(zmk_widget_output_obj(&output_widget), LV_ALIGN_TOP_LEFT, 0, 0);

    zmk_widget_modifiers_init(&modifiers_widget, screen);
    lv_obj_align(zmk_widget_modifiers_obj(&modifiers_widget), LV_ALIGN_BOTTOM_LEFT, 0, 0);

    zmk_widget_layer_init(&layer_widget, screen);
    // lv_obj_align(zmk_widget_layer_obj(&layer_widget), LV_ALIGN_BOTTOM_LEFT, 2, -18);
    lv_obj_align_to(zmk_widget_layer_obj(&layer_widget), zmk_widget_bongo_cat_obj(&bongo_cat_widget), LV_ALIGN_BOTTOM_LEFT, 0, 5);

    zmk_widget_battery_init(&battery_widget, screen);
    lv_obj_align(zmk_widget_battery_obj(&battery_widget), LV_ALIGN_TOP_RIGHT, 0, 0);

    return screen;
}
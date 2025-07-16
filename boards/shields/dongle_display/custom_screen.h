/*
 *
 * Copyright (c) 2024 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>

struct zmk_widget_battery {
    sys_snode_t node;
    lv_obj_t *obj;
};

struct zmk_widget_layer {
    sys_snode_t node;
    lv_obj_t *obj;
};

struct zmk_widget_modifiers {
    sys_snode_t node;
    lv_obj_t *obj;
};

struct zmk_widget_output {
    sys_snode_t node;
    lv_obj_t *obj;
};

int zmk_widget_battery_init(struct zmk_widget_battery *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_battery_obj(struct zmk_widget_battery *widget);

int zmk_widget_layer_init(struct zmk_widget_layer *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_layer_obj(struct zmk_widget_layer *widget);

int zmk_widget_modifiers_init(struct zmk_widget_modifiers *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_modifiers_obj(struct zmk_widget_modifiers *widget);

int zmk_widget_output_init(struct zmk_widget_output *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_output_obj(struct zmk_widget_output *widget);

lv_obj_t *zmk_display_screen();
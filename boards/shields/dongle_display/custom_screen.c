/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);
 
#include <dt-bindings/zmk/modifiers.h>
#include <zephyr/bluetooth/services/bas.h>
#include <zephyr/kernel.h>
#include <zmk/battery.h>
#include <zmk/ble.h>
#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/keymap.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/hid.h>

#include "custom_screen.h"
#include "assets/space_mono_14.c"
#include "assets/fonts.h"

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

const uint8_t MODS_KEYS[] = { (MOD_LSFT | MOD_RSFT), (MOD_LCTL | MOD_RCTL), (MOD_LGUI | MOD_RGUI), (MOD_LALT | MOD_RALT) };
#if IS_ENABLED(CONFIG_DONGLE_DISPLAY_ROTATE)
    const char* const MODS_BITES[] = { "SHIFT\n", "CTRL\n", "GUI\n", "ALT"} ;
    const char* const MODS_EMPTY[] = { " \n", " \n", " \n", " "} ;
#else
    const char* const MODS_BITES[] = { "SHIFT ", "CTRL  ", "GUI   ", "ALT"} ;
    const char* const MODS_EMPTY[] =  { "      ", "      ", "      ", "   "} ;
#endif

struct status_state {
    char batteries [ZMK_SPLIT_BLE_PERIPHERAL_COUNT][9];
    char top_layer[12];
    uint8_t active_mods;
};

struct zmk_widget_screen {
  sys_snode_t node;
  lv_obj_t *obj;
  lv_color_t cbuf[CANVAS_HEIGHT * CANVAS_HEIGHT];
  struct status_state state;
};


// draw canvas
void init_label_dsc(lv_draw_label_dsc_t *label_dsc, lv_color_t color, const lv_font_t *font, lv_text_align_t align) {
    lv_draw_label_dsc_init(label_dsc);
    label_dsc->color = color;
    label_dsc->font = font;
    label_dsc->align = align;
}

static void draw_canvas(lv_obj_t *widget, lv_color_t cbuf[], const struct status_state *state) {
    
    char batteries_text[ZMK_SPLIT_BLE_PERIPHERAL_COUNT * 9] = {};
    for (int i = 0; i < ZMK_SPLIT_BLE_PERIPHERAL_COUNT; i++) {
        strcat(batteries_text, state->batteries[i]);
    }

    lv_obj_t *canvas = lv_obj_get_child(widget, 0);
    lv_draw_label_dsc_t layer_label;
    lv_draw_label_dsc_t batteries_label;
    lv_draw_label_dsc_t mods_label;

    init_label_dsc(&mods_label, LVGL_FOREGROUND, &space_mono_14, LV_TEXT_ALIGN_CENTER);
    
    if ( ROTATION ) {
        init_label_dsc(&layer_label, LVGL_FOREGROUND, &space_mono_14, LV_TEXT_ALIGN_CENTER);
        lv_canvas_draw_text(canvas, 0, 7, CANVAS_WIDTH, &layer_label, state->top_layer);
    
        init_label_dsc(&batteries_label, LVGL_FOREGROUND, &space_mono_14, LV_TEXT_ALIGN_CENTER);
        lv_canvas_draw_text(canvas, 0, 25, CANVAS_WIDTH, &batteries_label, batteries_text);

    } else {
        init_label_dsc(&layer_label, LVGL_FOREGROUND, &space_mono_14, LV_TEXT_ALIGN_RIGHT);
        lv_canvas_draw_text(canvas, 1, 2, CANVAS_WIDTH / 2 - 4, &layer_label, state->top_layer);
    
        init_label_dsc(&batteries_label, LVGL_FOREGROUND, &space_mono_14, LV_TEXT_ALIGN_LEFT);
        lv_canvas_draw_text(canvas, 1, CANVAS_WIDTH / 2 + 2, CANVAS_WIDTH / 2 - 4, &batteries_label, batteries_text);
    }

    char mods_text[33] = {};
    for (int i = 0; i < 4; i++) {
        strcat(mods_text, (state->active_mods & MODS_KEYS[i]) ? MODS_BITES[i] : MODS_EMPTY[i]);
    }
    lv_canvas_draw_text(canvas, 0, CANVAS_HEIGHT / 2, CANVAS_WIDTH, &mods_label, mods_text);

    if ( ROTATION + ORIENTATION ) {
        static lv_color_t cbuf_tmp[CANVAS_HEIGHT * CANVAS_HEIGHT];
        memcpy(cbuf_tmp, cbuf, sizeof(cbuf_tmp));
    
        lv_img_dsc_t img;
        img.data = (void *)cbuf_tmp;
        img.header.cf = LV_IMG_CF_TRUE_COLOR;
        img.header.w = CANVAS_HEIGHT;
        img.header.h = CANVAS_HEIGHT;
        // lv_canvas_fill_bg(canvas, LVGL_BACKGROUND, LV_OPA_COVER);
        lv_canvas_transform(canvas, &img, ROTATION + ORIENTATION, LV_IMG_ZOOM_NONE, 0, 0, CANVAS_HEIGHT / 2, CANVAS_HEIGHT / 2, false);
    }
}


// battery
struct battery {
    uint8_t source;
    uint8_t level;
};

static void set_battery(struct zmk_widget_screen *widget, struct battery state) {
    if ( state.source >= ZMK_SPLIT_BLE_PERIPHERAL_COUNT ) { return; }
    sprintf(widget->state.batteries[state.source], "%4u %% ", state.level);
    draw_canvas(widget->obj, widget->cbuf, &widget->state);
}

static void battery_update_cb(struct battery state) {
    struct zmk_widget_screen *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_battery(widget, state); }
}

static struct battery battery_get_state(const zmk_event_t *eh) {
    const struct zmk_peripheral_battery_state_changed *ev = as_zmk_peripheral_battery_state_changed(eh);
    return (struct battery){
        .source = ev->source,
        .level = ev->state_of_charge,
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_battery, struct battery, battery_update_cb, battery_get_state)
ZMK_SUBSCRIPTION(widget_battery, zmk_peripheral_battery_state_changed);


// top layer
struct layer {
    uint8_t index;
    const char *label;
};

static void set_layer(struct zmk_widget_screen *widget, struct layer state) {
    if (state.label == NULL) { sprintf(widget->state.top_layer, "%i", state.index); } 
    else { sprintf(widget->state.top_layer, "%s", state.label); }
}

static void layer_update_cb(uint8_t state) {
    struct zmk_widget_screen *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_layer(widget, state); }
}

static struct layer layer_get_state(const zmk_event_t *eh) {
    uint8_t index = zmk_keymap_highest_layer_active();
    return (struct layer){
        .index = index, 
        .label = zmk_keymap_layer_name(index)
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_layer, uint8_t, layer_update_cb, layer_get_state)
ZMK_SUBSCRIPTION(widget_layer, zmk_layer_state_changed);


// Modifiers
static void set_modifiers(struct zmk_widget_screen *widget, uint8_t mods) {
    if (widget->state.active_mods != mods) {
        widget->state.active_mods = mods;
        draw_canvas(widget->obj, widget->cbuf, &widget->state);
    }
}

void modifiers_update_cb(uint8_t state) {
    struct zmk_widget_screen *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_modifiers(widget, state); }
}

static uint8_t modifiers_get_state(const zmk_event_t *eh) {
    return zmk_hid_get_explicit_mods();
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_modifiers, uint8_t, modifiers_update_cb, modifiers_get_state)
ZMK_SUBSCRIPTION(widget_modifiers, zmk_keycode_state_changed);


// initialise

int zmk_widget_screen_init(struct zmk_widget_screen *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, CANVAS_HEIGHT, CANVAS_WIDTH);

    lv_obj_t *canvas = lv_canvas_create(widget->obj);
    lv_obj_align(canvas, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_canvas_set_buffer(canvas, widget->cbuf, CANVAS_HEIGHT, CANVAS_HEIGHT, LV_IMG_CF_TRUE_COLOR);
    // Fill the canvas background (optional, for visibility)
    lv_canvas_fill_bg(canvas, LVGL_BACKGROUND, LV_OPA_COVER);

    sys_slist_append(&widgets, &widget->node);
    widget_battery_init();
    widget_layer_init();
    widget_modifiers_init();

    return 0;
}

lv_obj_t *zmk_widget_screen_obj(struct zmk_widget_screen *widget) { return widget->obj; }

static struct zmk_widget_screen screen_widget;
lv_obj_t *zmk_display_screen() {
    lv_obj_t *screen;
    screen = lv_obj_create(NULL);

    zmk_widget_screen_init(&screen_widget, screen);
    lv_obj_align(zmk_widget_screen_obj(&screen_widget), LV_ALIGN_TOP_LEFT, 0, 0);

    return screen;
}
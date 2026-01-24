#include <zephyr/kernel.h>
#include <zephyr/bluetooth/services/bas.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/ble.h>
#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/split/central.h>
#include <zmk/events/modifiers_state_changed.h>
#include <zmk/keymap.h>
#include <zmk/hid.h>
#include <dt-bindings/zmk/modifiers.h>

#include <lvgl.h>


#ifndef ZMK_SPLIT_BLE_PERIPHERAL_COUNT
#  define ZMK_SPLIT_BLE_PERIPHERAL_COUNT 0
#endif

struct zmk_widget_status {
    lv_obj_t *obj;
    lv_obj_t *label;
};

static struct zmk_widget_status widget;

static void update_status_label(lv_obj_t *label) {
    // 1. Get Peripheral Batteries (from what has been sent to Central)
    char batteries[6*ZMK_SPLIT_BLE_PERIPHERAL_COUNT+6]; 
    int offset = 0;
    int written = 0;
    uint8_t level;
    for (int i = 0; i < ZMK_SPLIT_BLE_PERIPHERAL_COUNT; i++) {
        // snprintf returns the number of characters that *would* have been written
        int rc = zmk_split_central_get_peripheral_battery_level(i, &level);
        if (rc == 0) {
            written = snprintf(batteries + offset, sizeof(batteries) - offset, "%3d%% ", level);
        } else {
            written = snprintf(batteries + offset, sizeof(batteries) - offset, "..."); 
        }
        // Update the offset to point to the end of the new string
        offset += written;
        // Safety check: stop if we run out of batteries space
        if (offset >= sizeof(batteries)) break;
    }
    
    // 2. Get Layer Name
    uint8_t layer_index = zmk_keymap_highest_layer_active();
    const char *layer_name = zmk_keymap_layer_name(layer_index);
    char layer_label[sizeof(layer_name) + 5];
    if (layer_name != NULL) {
        snprintf(layer_label, sizeof(layer_name), "%s", layer_name);
    } else {
        snprintf(layer_label, sizeof(layer_index), "-%i-", layer_index);
    }

    // 3. Get Modifiers
    zmk_mod_flags_t mods = zmk_hid_get_explicit_mods();
    bool shift = mods & (MOD_LSFT | MOD_RSFT),
    bool ctrl  = mods & (MOD_LCTL | MOD_RCTL);
    bool alt   = mods & (MOD_LALT | MOD_RALT);
    bool gui   = mods & (MOD_LGUI | MOD_RGUI);

    // 4. Caps Status
    // bool caps_lock = zmk_hid_get_leds() & BIT(0);
    // bool caps_word = zmk_caps_word_active();

    // Format the single-line string
    lv_label_set_text_fmt(label, 
        "%s | %s \n"
        "%s%s%s%s%s",
        batteries,
        layer_label,
        (shift) ? "Shft " : "     ",
        (ctrl)  ? "Ctrl " : "     ",
        (gui)   ? "Cmd " : "    ", 
        (alt)   ? "Alt " : "    ",
        "on" // (caps_lock || caps_word) ? "CAPS" : ""
    );
}

static int status_event_handler(const zmk_event_t *eh) {
    update_status_label(widget.label);
    return 0;
}

// Register listeners for all relevant state changes
ZMK_LISTENER(widget_status, status_event_handler);
ZMK_SUBSCRIPTION(widget_status, zmk_layer_state_changed);
ZMK_SUBSCRIPTION(widget_status, zmk_peripheral_battery_state_changed);
ZMK_SUBSCRIPTION(widget_status, zmk_modifiers_state_changed);


static lv_style_t style_status;
static bool style_init = false;

void init_status_styles() {
    if (style_init) return;

    lv_style_init(&style_status);
    // Set text color (white for OLEDs, use lv_palette_main(LV_PALETTE_BLUE) for color)
    lv_style_set_text_color(&style_status, lv_color_white());
    // Set a small, clean font (ZMK usually includes montserrat_12 or montserrat_16)
    lv_style_set_text_font(&style_status, &lv_font_unscii_16);
    // Letter spacing for readability
    lv_style_set_text_letter_space(&style_status, 2);
    lv_style_set_text_line_space(&style_status, 0);
    style_init = true;
}

int zmk_widget_status_init(struct zmk_widget_status *w, lv_obj_t *parent) {
    init_status_styles();
    
    w->obj = lv_obj_create(parent);
    
    // 1. Style the Container (the "box" holding the text)
    lv_obj_set_size(w->obj, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(w->obj, 0, 0);                 // No padding
    lv_obj_set_style_bg_opa(w->obj, LV_OPA_TRANSP, 0);      // Transparent background
    lv_obj_set_style_border_width(w->obj, 0, 0);            // No border
    lv_obj_set_style_align(w->obj, LV_ALIGN_TOP_LEFT, 0);   // Stick to LEFT
    lv_obj_set_scrollbar_mode(w->obj, LV_SCROLLBAR_MODE_OFF);   // Remove scrollbars from the container
    // 2. Style the Label
    w->label = lv_label_create(w->obj);
    lv_obj_add_style(w->label, &style_status, 0);
    // Handle text overflow: 
    // LV_LABEL_LONG_SCROLL_CIRCULAR will scroll the text if it's too long  // LV_LABEL_LONG_DOT will add "..." at the end
    lv_label_set_long_mode(w->label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(w->label, LV_PCT(100));
    // Center the text for a balanced look on small screens
    lv_obj_set_style_text_align(w->label, LV_TEXT_ALIGN_CENTER, 0);

    update_status_label(w->label);
    return 0;
}
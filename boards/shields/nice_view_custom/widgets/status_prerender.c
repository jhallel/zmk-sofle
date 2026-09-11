#include <stdio.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/battery.h>
#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/ble.h>
#include <zmk/endpoints.h>
#include <zmk/keymap.h>
#include <zmk/usb.h>

#include "status.h"

LV_IMG_DECLARE(nerv_bg_1);
LV_IMG_DECLARE(nerv_bg_2);
LV_IMG_DECLARE(nerv_bg_3);

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct output_status_state {
    bool usb_selected;
    uint8_t profile_index;
    bool connected;
    bool bonded;
};
struct layer_status_state { uint8_t index; };

static const char *mode_name(uint8_t layer) {
    switch (layer) {
    case 0: return "NORMAL";
    case 1: return "M1";
    case 2: return "M2";
    case 3: return "MAINT";
    default: return "SPEC";
    }
}

static void solid(lv_obj_t *canvas, int x, int y, int w, int h, lv_color_t color) {
    lv_draw_rect_dsc_t d;
    init_rect_dsc(&d, color);
    lv_canvas_draw_rect(canvas, x, y, w, h, &d);
}

static uint64_t glyph5x7(char c) {
    switch (c) {
    case ' ': return 0x0ULL;
    case '%': return 0x6744458c0ULL;
    case '0': return 0x3a33ae62eULL; case '1': return 0x11842108eULL;
    case '2': return 0x3a211111fULL; case '3': return 0x78217043eULL;
    case '4': return 0x08ca97c42ULL; case '5': return 0x7e10f043eULL;
    case '6': return 0x3a10f462eULL; case '7': return 0x7c2222108ULL;
    case '8': return 0x3a317462eULL; case '9': return 0x3a317842eULL;
    case 'A': return 0x3a31fc631ULL; case 'B': return 0x7a31f463eULL;
    case 'C': return 0x3e108420fULL; case 'D': return 0x7a318c63eULL;
    case 'E': return 0x7e10f421fULL; case 'I': return 0x7c842109fULL;
    case 'L': return 0x42108421fULL; case 'M': return 0x4775ac631ULL;
    case 'N': return 0x47359c631ULL; case 'O': return 0x3a318c62eULL;
    case 'P': return 0x7a31f4210ULL; case 'R': return 0x7a31f5251ULL;
    case 'S': return 0x3e107043eULL; case 'T': return 0x7c8421084ULL;
    case 'U': return 0x46318c62eULL; case 'W': return 0x4631ad6aaULL;
    case 'Y': return 0x462a21084ULL;
    default: return 0x0ULL;
    }
}

static void pixel_text_adv(lv_obj_t *canvas, int x, int y, const char *s, int advance) {
    for (int i = 0; s[i] != '\0'; i++) {
        uint64_t bits = glyph5x7(s[i]);
        for (int row = 0; row < 7; row++) {
            for (int col = 0; col < 5; col++) {
                int bit = 34 - (row * 5 + col);
                if ((bits >> bit) & 1ULL) {
                    solid(canvas, x + col, y + row, 1, 1, LVGL_FOREGROUND);
                }
            }
        }
        x += advance;
    }
}

static void pixel_text(lv_obj_t *canvas, int x, int y, const char *s) {
    pixel_text_adv(canvas, x, y, s, 6);
}

static void draw_bg(lv_obj_t *canvas, const lv_img_dsc_t *bg) {
    lv_draw_rect_dsc_t clear;
    init_rect_dsc(&clear, LVGL_BACKGROUND);
    lv_canvas_draw_rect(canvas, 0, 0, CANVAS_SIZE, CANVAS_SIZE, &clear);
    lv_draw_img_dsc_t img_dsc;
    lv_draw_img_dsc_init(&img_dsc);
    lv_canvas_draw_img(canvas, 0, 0, bg, &img_dsc);
}

static void draw_top(lv_obj_t *widget, lv_color_t cbuf[]) {
    lv_obj_t *canvas = lv_obj_get_child(widget, 0);
    draw_bg(canvas, &nerv_bg_1);
    rotate_canvas(canvas, cbuf);
}

static void draw_middle(lv_obj_t *widget, lv_color_t cbuf[], const struct status_state *state) {
    lv_obj_t *canvas = lv_obj_get_child(widget, 1);
    draw_bg(canvas, &nerv_bg_2);

    int fill_w = (state->battery * 24) / 100;
    if (fill_w > 0) {
        solid(canvas, 4, 52, fill_w, 9, LVGL_FOREGROUND);
    }
    char pct[8];
    snprintf(pct, sizeof(pct), "%u%%", state->battery);
    pixel_text(canvas, 40, 51, pct);
    rotate_canvas(canvas, cbuf);
}

static void draw_bottom(lv_obj_t *widget, lv_color_t cbuf[], const struct status_state *state) {
    lv_obj_t *canvas = lv_obj_get_child(widget, 2);
    draw_bg(canvas, &nerv_bg_3);

    const char *link = state->usb_selected ? "USB" : (state->connected ? "BT" : "STBY");
    pixel_text(canvas, 28, 0, link);
    pixel_text_adv(canvas, 28, 10, mode_name(state->layer_index), 5);
    rotate_canvas(canvas, cbuf);
}

static void redraw_all(struct zmk_widget_status *widget) {
    draw_top(widget->obj, widget->cbuf);
    draw_middle(widget->obj, widget->cbuf2, &widget->state);
    draw_bottom(widget->obj, widget->cbuf3, &widget->state);
}

static void set_battery_status(struct zmk_widget_status *widget, struct battery_status_state state) {
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
    widget->state.charging = state.usb_present;
#endif
    widget->state.battery = state.level;
    draw_middle(widget->obj, widget->cbuf2, &widget->state);
}
static void battery_status_update_cb(struct battery_status_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_battery_status(widget, state); }
}
static struct battery_status_state battery_status_get_state(const zmk_event_t *eh) {
    ARG_UNUSED(eh);
    return (struct battery_status_state){
        .level = zmk_battery_state_of_charge(),
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
        .usb_present = zmk_usb_is_powered(),
#endif
    };
}
ZMK_DISPLAY_WIDGET_LISTENER(widget_battery_status, struct battery_status_state,
                            battery_status_update_cb, battery_status_get_state)
ZMK_SUBSCRIPTION(widget_battery_status, zmk_battery_state_changed);
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(widget_battery_status, zmk_usb_conn_state_changed);
#endif

static void set_output_status(struct zmk_widget_status *widget, const struct output_status_state *state) {
    widget->state.usb_selected = state->usb_selected;
    widget->state.profile_index = state->profile_index;
    widget->state.connected = state->connected;
    widget->state.bonded = state->bonded;
    draw_bottom(widget->obj, widget->cbuf3, &widget->state);
}
static void output_status_update_cb(struct output_status_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_output_status(widget, &state); }
}
static struct output_status_state output_status_get_state(const zmk_event_t *eh) {
    ARG_UNUSED(eh);
    struct zmk_endpoint_instance ep = zmk_endpoints_selected();
    return (struct output_status_state){
        .usb_selected = ep.transport == ZMK_TRANSPORT_USB,
        .profile_index = zmk_ble_active_profile_index(),
        .connected = zmk_ble_active_profile_is_connected(),
        .bonded = !zmk_ble_active_profile_is_open(),
    };
}
ZMK_DISPLAY_WIDGET_LISTENER(widget_output_status, struct output_status_state,
                            output_status_update_cb, output_status_get_state)
ZMK_SUBSCRIPTION(widget_output_status, zmk_endpoint_changed);
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(widget_output_status, zmk_usb_conn_state_changed);
#endif
#if defined(CONFIG_ZMK_BLE)
ZMK_SUBSCRIPTION(widget_output_status, zmk_ble_active_profile_changed);
#endif

static void set_layer_status(struct zmk_widget_status *widget, struct layer_status_state state) {
    widget->state.layer_index = state.index;
    draw_bottom(widget->obj, widget->cbuf3, &widget->state);
}
static void layer_status_update_cb(struct layer_status_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_layer_status(widget, state); }
}
static struct layer_status_state layer_status_get_state(const zmk_event_t *eh) {
    ARG_UNUSED(eh);
    return (struct layer_status_state){.index = zmk_keymap_highest_layer_active()};
}
ZMK_DISPLAY_WIDGET_LISTENER(widget_layer_status, struct layer_status_state,
                            layer_status_update_cb, layer_status_get_state)
ZMK_SUBSCRIPTION(widget_layer_status, zmk_layer_state_changed);

int zmk_widget_status_init(struct zmk_widget_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 160, 68);

    lv_obj_t *top = lv_canvas_create(widget->obj);
    lv_obj_align(top, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_canvas_set_buffer(top, widget->cbuf, CANVAS_SIZE, CANVAS_SIZE, LV_IMG_CF_TRUE_COLOR);

    lv_obj_t *middle = lv_canvas_create(widget->obj);
    lv_obj_align(middle, LV_ALIGN_TOP_LEFT, 24, 0);
    lv_canvas_set_buffer(middle, widget->cbuf2, CANVAS_SIZE, CANVAS_SIZE, LV_IMG_CF_TRUE_COLOR);

    lv_obj_t *bottom = lv_canvas_create(widget->obj);
    lv_obj_align(bottom, LV_ALIGN_TOP_LEFT, -44, 0);
    lv_canvas_set_buffer(bottom, widget->cbuf3, CANVAS_SIZE, CANVAS_SIZE, LV_IMG_CF_TRUE_COLOR);

    widget->state.battery = zmk_battery_state_of_charge();
    widget->state.layer_index = zmk_keymap_highest_layer_active();

    struct zmk_endpoint_instance ep = zmk_endpoints_selected();
    widget->state.usb_selected = ep.transport == ZMK_TRANSPORT_USB;
    widget->state.profile_index = zmk_ble_active_profile_index();
    widget->state.connected = zmk_ble_active_profile_is_connected();
    widget->state.bonded = !zmk_ble_active_profile_is_open();

    sys_slist_append(&widgets, &widget->node);
    redraw_all(widget);
    widget_battery_status_init();
    widget_output_status_init();
    widget_layer_status_init();
    return 0;
}

lv_obj_t *zmk_widget_status_obj(struct zmk_widget_status *widget) { return widget->obj; }

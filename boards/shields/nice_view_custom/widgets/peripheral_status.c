#include <zephyr/kernel.h>
#include <lvgl.h>
#include "peripheral_status.h"

#define DECL_EVA(n) LV_IMG_DECLARE(eva##n)
DECL_EVA(01); DECL_EVA(02); DECL_EVA(03); DECL_EVA(04); DECL_EVA(05); DECL_EVA(06);
DECL_EVA(07); DECL_EVA(08); DECL_EVA(09); DECL_EVA(10); DECL_EVA(11); DECL_EVA(12);
DECL_EVA(13); DECL_EVA(14); DECL_EVA(15); DECL_EVA(16); DECL_EVA(17); DECL_EVA(18);
DECL_EVA(19); DECL_EVA(20); DECL_EVA(21); DECL_EVA(22); DECL_EVA(23); DECL_EVA(24);
DECL_EVA(25); DECL_EVA(26); DECL_EVA(27); DECL_EVA(28); DECL_EVA(29); DECL_EVA(30);
DECL_EVA(31); DECL_EVA(32); DECL_EVA(33); DECL_EVA(34); DECL_EVA(35); DECL_EVA(36);
DECL_EVA(37); DECL_EVA(38); DECL_EVA(39); DECL_EVA(40); DECL_EVA(41); DECL_EVA(42);
DECL_EVA(43); DECL_EVA(44); DECL_EVA(45); DECL_EVA(46); DECL_EVA(47); DECL_EVA(48);
DECL_EVA(49); DECL_EVA(50); DECL_EVA(51); DECL_EVA(52); DECL_EVA(53); DECL_EVA(54);
DECL_EVA(55); DECL_EVA(56); DECL_EVA(57); DECL_EVA(58); DECL_EVA(59); DECL_EVA(60);
DECL_EVA(61); DECL_EVA(62); DECL_EVA(63); DECL_EVA(64); DECL_EVA(65); DECL_EVA(66);
DECL_EVA(67); DECL_EVA(68); DECL_EVA(69); DECL_EVA(70); DECL_EVA(71); DECL_EVA(72);
DECL_EVA(73); DECL_EVA(74); DECL_EVA(75);
#undef DECL_EVA

/*
 * Final 75-frame cinematic reel.
 * Frames 32..49: polished Evangelion-style EMERGENCY warning sequence.
 * Frames 50..73: 24-frame EVA-01 awakening.
 * Existing boot/Tokyo-3/facility/cast artwork is preserved unchanged.
 * Total loop duration: 35.04 seconds.
 */
static const lv_img_dsc_t *anim_imgs[] = {
    &eva01, &eva02, &eva03, &eva04, &eva05, &eva06, &eva07, &eva08,
    &eva09, &eva10, &eva11, &eva12, &eva13, &eva14, &eva15, &eva16,
    &eva17, &eva18, &eva19, &eva20, &eva21, &eva22, &eva23, &eva24,
    &eva25, &eva26, &eva27, &eva28, &eva29, &eva30, &eva31, &eva32,
    &eva33, &eva34, &eva35, &eva36, &eva37, &eva38, &eva39, &eva40,
    &eva41, &eva42, &eva43, &eva44, &eva45, &eva46, &eva47, &eva48,
    &eva49, &eva50, &eva51, &eva52, &eva53, &eva54, &eva55, &eva56,
    &eva57, &eva58, &eva59, &eva60, &eva61, &eva62, &eva63, &eva64,
    &eva65, &eva66, &eva67, &eva68, &eva69, &eva70, &eva71, &eva72,
    &eva73, &eva74, &eva75,
};

static const uint16_t anim_duration_ms[] = {
    1210, 990, 990, 990, 1210, 940, 940, 880, 940, 990,
    940, 940, 990, 990, 1040, 770, 770, 770, 770, 770,
    830, 770, 830, 770, 660, 720, 830, 720, 830, 880,
    990, 180, 180, 180, 180, 130, 130, 180, 180, 180,
    180, 120, 120, 180, 180, 260, 180, 100, 90, 180,
    180, 170, 170, 160, 160, 160, 160, 160, 160, 150,
    150, 150, 150, 150, 150, 150, 150, 160, 160, 170,
    180, 220, 280, 200, 220,
};

BUILD_ASSERT(ARRAY_SIZE(anim_imgs) == ARRAY_SIZE(anim_duration_ms),
             "EVA frame and timing tables must match");

static lv_obj_t *anim_art;
static uint8_t anim_index;

static void advance_animation(lv_timer_t *timer) {
    anim_index = (anim_index + 1U) % ARRAY_SIZE(anim_imgs);
    lv_img_set_src(anim_art, anim_imgs[anim_index]);
    lv_timer_set_period(timer, anim_duration_ms[anim_index]);
}

int zmk_widget_status_init(struct zmk_widget_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 160, 68);

    anim_index = 0U;
    anim_art = lv_img_create(widget->obj);
    lv_obj_align(anim_art, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_img_set_src(anim_art, anim_imgs[anim_index]);

    lv_timer_create(advance_animation, anim_duration_ms[anim_index], NULL);
    return 0;
}

lv_obj_t *zmk_widget_status_obj(struct zmk_widget_status *widget) { return widget->obj; }

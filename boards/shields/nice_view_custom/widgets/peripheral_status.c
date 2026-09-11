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
#undef DECL_EVA

/*
 * Expanded cinematic reel, still using CONFIG_CUSTOM_ANIMATION_SPEED as the
 * total loop duration (~19.58 s). The original character portraits and alert
 * frames are preserved; frames 33..60 add Tokyo-3, facility/entry/startup,
 * activation, and a twelve-pose EVA-01 awakening.
 *
 * 86 playback steps => ~228 ms/step at 19.58 s total.
 */
static const lv_img_dsc_t *anim_imgs[] = {
    /* Poster intro */
    &eva01,&eva01,&eva01,&eva01,&eva01,&eva01,

    /* New Tokyo-3 establishing shots */
    &eva33,&eva33,&eva34,&eva34,&eva35,&eva35,

    /* NERV facility */
    &eva36,&eva36,&eva37,&eva37,

    /* Original cast: Shinji, Rei, Asuka, Misato, Kaworu, Gendo, Ritsuko, Kaji */
    &eva03,&eva03,&eva05,&eva05,&eva07,&eva07,&eva09,&eva09,
    &eva11,&eva11,&eva13,&eva13,&eva15,&eva15,&eva17,&eva17,

    /* NERV / warning escalation */
    &eva18,&eva18,&eva18,
    &eva20,&eva20,&eva21,&eva21,&eva22,&eva22,&eva23,&eva23,&eva24,&eva24,

    /* New entry / launch */
    &eva38,&eva38,&eva39,&eva39,&eva40,&eva40,

    /* New startup */
    &eva41,&eva41,&eva42,&eva42,&eva43,&eva43,&eva44,&eva44,

    /* New activation */
    &eva45,&eva45,&eva46,&eva46,&eva47,&eva47,&eva48,&eva48,

    /* New EVA-01 awakening: dormant -> eye -> head -> jaw -> rise -> roar */
    &eva49,&eva50,&eva51,&eva52,&eva53,&eva54,&eva55,&eva56,
    &eva57,&eva58,&eva59,&eva60,&eva59,&eva60,

    /* Settle / loop */
    &eva18,&eva18,&eva02,&eva01,&eva01
};

int zmk_widget_status_init(struct zmk_widget_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 160, 68);

    lv_obj_t *art = lv_animimg_create(widget->obj);
    lv_obj_align(art, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_animimg_set_src(art, (const void **)anim_imgs, ARRAY_SIZE(anim_imgs));
    lv_animimg_set_duration(art, CONFIG_CUSTOM_ANIMATION_SPEED);
    lv_animimg_set_repeat_count(art, LV_ANIM_REPEAT_INFINITE);
    lv_animimg_start(art);
    return 0;
}

lv_obj_t *zmk_widget_status_obj(struct zmk_widget_status *widget) { return widget->obj; }

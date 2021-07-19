//  ────────────────────────────────────────────────────────────
//                     ╔╗  ╔╗ ╔══╗      ╔════╗
//                     ║╚╗╔╝║ ╚╣╠╝      ║╔╗╔╗║
//                     ╚╗╚╝╔╝  ║║  ╔══╗ ╚╝║║╚╝
//                      ╚╗╔╝   ║║  ║╔╗║   ║║
//                       ║║   ╔╣╠╗ ║╚╝║   ║║
//                       ╚╝   ╚══╝ ╚══╝   ╚╝
//    ╔╗╔═╗                    ╔╗                     ╔╗
//    ║║║╔╝                   ╔╝╚╗                    ║║
//    ║╚╝╝  ╔══╗ ╔══╗ ╔══╗  ╔╗╚╗╔╝  ╔══╗ ╔╗ ╔╗╔╗ ╔══╗ ║║  ╔══╗
//    ║╔╗║  ║║═╣ ║║═╣ ║╔╗║  ╠╣ ║║   ║ ═╣ ╠╣ ║╚╝║ ║╔╗║ ║║  ║║═╣
//    ║║║╚╗ ║║═╣ ║║═╣ ║╚╝║  ║║ ║╚╗  ╠═ ║ ║║ ║║║║ ║╚╝║ ║╚╗ ║║═╣
//    ╚╝╚═╝ ╚══╝ ╚══╝ ║╔═╝  ╚╝ ╚═╝  ╚══╝ ╚╝ ╚╩╩╝ ║╔═╝ ╚═╝ ╚══╝
//                    ║║                         ║║
//                    ╚╝                         ╚╝
//
//    Lead Maintainer: Roman Kutashenko <kutashenko@gmail.com>
//  ────────────────────────────────────────────────────────────

#ifndef VS_IOT_EVENT_GROUP_BITS_H
#define VS_IOT_EVENT_GROUP_BITS_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#define EVENT_BIT(NUM) ((vs_event_bits_t)1 << (vs_event_bits_t)NUM)

typedef uint32_t vs_event_bits_t;
typedef struct vs_event_group_bits_s {
    pthread_cond_t cond;
    pthread_mutex_t mtx;
    vs_event_bits_t event_flags;
} vs_event_group_bits_t;

#define VS_EVENT_GROUP_WAIT_INFINITE (-1)
vs_event_bits_t
vs_event_group_wait_bits(vs_event_group_bits_t *ev_group,
                         vs_event_bits_t bits_to_wait_for,
                         bool is_clear_on_exit,
                         bool is_wait_for_all,
                         int32_t timeout);

vs_event_bits_t
vs_event_group_clear_bits(vs_event_group_bits_t *ev_group, vs_event_bits_t bits_to_clear);

vs_event_bits_t
vs_event_group_set_bits(vs_event_group_bits_t *ev_group, vs_event_bits_t bits_to_set);

int
vs_event_group_init(vs_event_group_bits_t *ev_group);

int
vs_event_group_destroy(vs_event_group_bits_t *ev_group);
#endif // VS_IOT_EVENT_GROUP_BITS_H

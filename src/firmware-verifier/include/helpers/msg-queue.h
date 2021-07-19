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

#ifndef VS_IOT_MESSAGE_QUEUE_H
#define VS_IOT_MESSAGE_QUEUE_H

#include <virgil/iot/status_code/status_code.h>

typedef struct vs_msg_queue_ctx_s vs_msg_queue_ctx_t;

vs_msg_queue_ctx_t *
vs_msg_queue_init(size_t queue_sz, size_t num_adders, size_t num_getters);

vs_status_e
vs_msg_queue_push(vs_msg_queue_ctx_t *ctx, const void *info, const uint8_t *data, size_t data_sz);

vs_status_e
vs_msg_queue_pop(vs_msg_queue_ctx_t *ctx, const void **info, const uint8_t **data, size_t *data_sz);

bool
vs_msg_queue_data_present(vs_msg_queue_ctx_t *ctx);

bool
vs_msg_queue_is_full(vs_msg_queue_ctx_t *ctx);

void
vs_msg_queue_reset(vs_msg_queue_ctx_t *ctx);

void
vs_msg_queue_free(vs_msg_queue_ctx_t *ctx);

#endif // VS_IOT_MESSAGE_QUEUE_H

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

#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

#include <virgil/iot/macros/macros.h>
#include <virgil/iot/threadsafe/vs-lock.h>

/******************************************************************************/
vs_lock_ctx_t
vs_threadsafe_init_hal(void) {
    pthread_mutex_t *mtx;

    mtx = malloc(sizeof(pthread_mutex_t));
    CHECK_NOT_ZERO(mtx);

    CHECK(0 == pthread_mutex_init(mtx, NULL), "Error init mutex var %s (%d)", strerror(errno), errno);

    return mtx;

terminate:
    free(mtx);
    return NULL;
}

/******************************************************************************/
vs_status_e
vs_threadsafe_deinit_hal(vs_lock_ctx_t ctx) {
    CHECK_NOT_ZERO_RET(ctx, VS_CODE_ERR_NULLPTR_ARGUMENT);

    pthread_mutex_destroy((pthread_mutex_t *)ctx);

    free(ctx);

    return VS_CODE_OK;
}

/******************************************************************************/
vs_status_e
vs_threadsafe_lock_hal(vs_lock_ctx_t ctx) {
    CHECK_NOT_ZERO_RET(ctx, VS_CODE_ERR_NULLPTR_ARGUMENT);

    CHECK_RET(0 == pthread_mutex_lock((pthread_mutex_t *)ctx), VS_CODE_ERR_THREAD, "Can't take mutex");

    return VS_CODE_OK;
}

/******************************************************************************/
vs_status_e
vs_threadsafe_unlock_hal(vs_lock_ctx_t ctx) {
    CHECK_NOT_ZERO_RET(ctx, VS_CODE_ERR_NULLPTR_ARGUMENT);

    CHECK_RET(0 == pthread_mutex_unlock((pthread_mutex_t *)ctx), VS_CODE_ERR_THREAD, "Can't release mutex");

    return VS_CODE_OK;
}
/******************************************************************************/

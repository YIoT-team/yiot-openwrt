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

#include <errno.h>
#include <stdio.h>
#include <sys/time.h>

#include <virgil/iot/logger/logger.h>
#include <virgil/iot/macros/macros.h>

#include "common/helpers/event-group-bits.h"

/******************************************************************************/
vs_event_bits_t
vs_event_group_wait_bits(vs_event_group_bits_t *ev_group,
                         vs_event_bits_t bits_to_wait_for,
                         bool is_clear_on_exit,
                         bool is_wait_for_all,
                         int32_t timeout) {

    int res = 0;
    struct timeval now;
    struct timespec thread_sleep;
    vs_event_bits_t stat = 0;

    assert(ev_group);
    CHECK_NOT_ZERO_RET(ev_group, 0);

    gettimeofday(&now, 0);
    thread_sleep.tv_sec = now.tv_sec + timeout;
    thread_sleep.tv_nsec = now.tv_usec * 1000;

    if (0 != pthread_mutex_lock(&ev_group->mtx)) {
        assert(false);
        return 0;
    }

    stat = ev_group->event_flags & bits_to_wait_for;

    while ((is_wait_for_all ? bits_to_wait_for != stat : 0 == stat) && ETIMEDOUT != res) {
        if (timeout >= 0) {
            res = pthread_cond_timedwait(&ev_group->cond, &ev_group->mtx, &thread_sleep);
        } else {
            res = pthread_cond_wait(&ev_group->cond, &ev_group->mtx);
        }
        stat = ev_group->event_flags & bits_to_wait_for;
        CHECK_RET(ETIMEDOUT == res || 0 == res, 0, "Error while wait condition, %s (%d)\n", strerror(errno), errno);
    }


    if (is_clear_on_exit) {
        ev_group->event_flags &= ~bits_to_wait_for;
    }

    if (0 != pthread_mutex_unlock(&ev_group->mtx)) {
        VS_LOG_ERROR("pthread_mutex_unlock. errno, %s (%d)", strerror(errno), errno);
    }
    return stat;
}

/******************************************************************************/
vs_event_bits_t
vs_event_group_clear_bits(vs_event_group_bits_t *ev_group, vs_event_bits_t bits_to_clear) {
    vs_event_bits_t stat = 0;
    assert(ev_group);
    CHECK_NOT_ZERO_RET(ev_group, 0);

    if (0 != pthread_mutex_lock(&ev_group->mtx)) {
        assert(false);
        return 0;
    }

    stat = ev_group->event_flags;
    ev_group->event_flags &= ~bits_to_clear;

    if (0 != pthread_mutex_unlock(&ev_group->mtx)) {
        VS_LOG_ERROR("pthread_mutex_unlock. errno, %s (%d)", strerror(errno), errno);
    }

    return stat;
}

/******************************************************************************/
vs_event_bits_t
vs_event_group_set_bits(vs_event_group_bits_t *ev_group, vs_event_bits_t bits_to_set) {
    vs_event_bits_t stat = 0;
    assert(ev_group);
    CHECK_NOT_ZERO_RET(ev_group, 0);

    if (0 != pthread_mutex_lock(&ev_group->mtx)) {
        assert(false);
        return 0;
    }

    stat = ev_group->event_flags;
    ev_group->event_flags |= bits_to_set;

    if (0 != pthread_mutex_unlock(&ev_group->mtx)) {
        VS_LOG_ERROR("pthread_mutex_unlock. errno, %s (%d)", strerror(errno), errno);
        return stat;
    }

    if (0 != pthread_cond_broadcast(&ev_group->cond)) {
        VS_LOG_ERROR("pthread_cond_broadcast. errno, %s (%d)", strerror(errno), errno);
    }

    return stat;
}

/******************************************************************************/
int
vs_event_group_init(vs_event_group_bits_t *ev_group) {
    assert(ev_group);
    CHECK_NOT_ZERO_RET(ev_group, -1);

    ev_group->event_flags = 0;

    if (0 != pthread_cond_init(&ev_group->cond, NULL)) {
        VS_LOG_ERROR("Error init condition var %s (%d)", strerror(errno), errno);
        return -1;
    }

    if (0 != pthread_mutex_init(&ev_group->mtx, NULL)) {
        VS_LOG_ERROR("Error init mutex var %s (%d)", strerror(errno), errno);
        return -1;
    }
    return 0;
}

/******************************************************************************/
int
vs_event_group_destroy(vs_event_group_bits_t *ev_group) {
    assert(ev_group);
    CHECK_NOT_ZERO_RET(ev_group, -1);
    pthread_cond_destroy(&ev_group->cond);
    pthread_mutex_destroy(&ev_group->mtx);
    return 0;
}

/******************************************************************************/

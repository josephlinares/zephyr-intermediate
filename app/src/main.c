/*
 * Lecture 4 Task 1
 *
 * GOAL: Build a small event-driven system with zbus
 *
 *
 * ================================================================
 * TASKS
 * ================================================================
 *
 * - Create one zbus channel for sensor data
 * - Publish simulated sensor samples every 100 ms
 * - Add one listener for fast display updates
 * - Add one subscriber for slower logging
 *
 * ================================================================
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <stdbool.h>
#include <zephyr/zbus/zbus.h>

LOG_MODULE_REGISTER(homework, LOG_LEVEL_DBG);

#define STACK_SIZE 1024
#define SENSOR_ITERATIONS 6
#define SENSOR_PERIOD_MS  100
#define PRESSURE_THRS     15

/* ================================================================== */
/*  Shared channel message                                            */
/* ================================================================== */

struct sensor_data {
    int32_t pressure_psi;
    uint32_t timestamp_ms;
    uint8_t seq;
};

/* Forward declarations required before observer/channel definitions. */
static void valve_listener_cb(const struct zbus_channel *chan);

/* ================================================================== */
/*  Observers                                                         */
/* ================================================================== */

/* Critical task: Valve control in chamber */
ZBUS_LISTENER_DEFINE(valve_lis, valve_listener_cb);

/* Slow task: Display updates from message copies */
ZBUS_MSG_SUBSCRIBER_DEFINE(display_sub);

/* ================================================================== */
/*  Channel                                                           */
/* ================================================================== */

ZBUS_CHAN_DEFINE(sensor_chan, struct sensor_data,
                 NULL, NULL,
                 ZBUS_OBSERVERS(valve_lis, display_sub),
                 ZBUS_MSG_INIT(.pressure_psi = 0,
                               .timestamp_ms = 0,
                               .seq = 0));


/* ================================================================== */
/*  Listener - synchronous observer                                   */
/* ================================================================== */

static void valve_listener_cb(const struct zbus_channel *chan)
{
    const struct sensor_data *msg =
        (const struct sensor_data *)zbus_chan_const_msg(chan);

    /*
     * Listener runs in publisher context.
     * Keep it short. No blocking work here.
     */
    LOG_INF("[VALVE-LIS] thread=%s seq=%u pres=%d PSI",
            k_thread_name_get(k_current_get()),
            msg->seq,
            msg->pressure_psi);
    
    if(msg->pressure_psi > PRESSURE_THRS) {
        LOG_INF("[VALVE-LIS] Valve opened");
    }
}


/* ================================================================== */
/*  Publisher                                                         */
/* ================================================================== */

static void sensor_thread_fn(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

    k_thread_name_set(k_current_get(), "sensor");

    for (int i = 0; i < SENSOR_ITERATIONS; i++) {
        struct sensor_data data = {
            .pressure_psi = 12 + i,
            .timestamp_ms = k_uptime_get_32(),
            .seq = (uint8_t)i,
        };

        LOG_INF("[SENSOR] publish seq=%u pres=%d PSI",
                data.seq,
                data.pressure_psi);

        int ret = zbus_chan_pub(&sensor_chan, &data, K_MSEC(50));
        if (ret != 0) {
            LOG_WRN("[SENSOR] publish failed ret=%d", ret);
        }

        k_msleep(SENSOR_PERIOD_MS);
    }

    LOG_INF("[SENSOR] done");
}

/* ================================================================== */
/*  Message subscriber - logger                                       */
/* ================================================================== */

static void display_thread_fn(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

    k_thread_name_set(k_current_get(), "display");

    const struct zbus_channel *chan;
    int received = 0;

    while (received < SENSOR_ITERATIONS) {
        struct sensor_data msg;

        /*
         * Message subscribers receive a copy of the published message.
         * The slow display will not reread the latest channel value.
         */
        int ret = zbus_sub_wait_msg(&display_sub, &chan, &msg, K_MSEC(1500));
        if (ret != 0) {
            LOG_WRN("[DISPLAY-MSG] timeout ret=%d", ret);
            break;
        }

        received++;

        LOG_INF("[DISPLAY-MSG] thread=%s seq=%u pres=%d latency=%ums",
                k_thread_name_get(k_current_get()),
                msg.seq,
                msg.pressure_psi,
                k_uptime_get_32() - msg.timestamp_ms);

        /*
         * Slow display.
         * Message copies let it process old samples safely.
         */
        k_msleep(150);
    }

    LOG_INF("[DISPLAY-MSG] done received=%d", received);
}

/* ================================================================== */
/*  Threads                                                           */
/* ================================================================== */

K_THREAD_DEFINE(sensor_thread, STACK_SIZE, sensor_thread_fn,
                NULL, NULL, NULL, 5, 0, 0);

K_THREAD_DEFINE(display_thread, STACK_SIZE, display_thread_fn,
                NULL, NULL, NULL, 6, 0, 0);

/* ================================================================== */
/*  Main                                                              */
/* ================================================================== */

int main(void)
{
    LOG_INF("=== L4 Task: Zbus Pub-Sub ===");
    LOG_INF("sensor publishes every %dms", SENSOR_PERIOD_MS);
    LOG_INF("valve listener runs in publisher context");
    LOG_INF("display uses message subscriber copies");
    LOG_INF("valve threshold: %d PSI", PRESSURE_THRS);

    return 0;
}
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(demo, LOG_LEVEL_DBG);

#define STACK_SIZE 	1024
#define PRIO 		3
#define INCREMENTS 	1000000

volatile static uint32_t counter;

static struct k_sem done_sem;

K_MUTEX_DEFINE(increment_mutex);

void t_increment_fn(void *p1, void *p2, void *p3)
{
    const char *name = k_thread_name_get(k_current_get());

    for (int i = 0; i < INCREMENTS; i++) {
	k_mutex_lock(&increment_mutex, K_FOREVER);
	counter++;

	if (counter % 100000 == 0) {
	    LOG_INF("[%s] counted to %d", name, counter);
	}

	k_mutex_unlock(&increment_mutex);
    }

    // Release a semaphore, incrementing its count by 1
    k_sem_give(&done_sem);
    LOG_INF("[%s] finished", name);
}

K_THREAD_DEFINE(thread_a, STACK_SIZE, t_increment_fn,
                NULL, NULL, NULL, PRIO, 0, 0);
K_THREAD_DEFINE(thread_b, STACK_SIZE, t_increment_fn,
		NULL, NULL, NULL, PRIO, 0, 0);

int main(void)
{
    k_sem_init(&done_sem, 0, 2);

    int64_t time = k_uptime_get();
    int32_t expected_value = INCREMENTS * 2;

    LOG_INF("=== L2 Assignment: Mutex Protection ===");
    LOG_INF("Expected final value: %d", expected_value);

    // Stop thread and wait until the semaphore becomes available
    k_sem_take(&done_sem, K_FOREVER);
    k_sem_take(&done_sem, K_FOREVER);

    LOG_INF("Actual final value: %u", counter);

    if (counter == expected_value) {
        LOG_WRN("No race this run");
    }
    else {
        LOG_ERR("Race condition confirmed: lost %d updates",
                expected_value - counter);
    }

    LOG_INF("Execution time: %lld ms", k_uptime_delta(&time));

    return 0;
}

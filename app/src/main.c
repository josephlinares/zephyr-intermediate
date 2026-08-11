#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(demo, LOG_LEVEL_DBG);

#define STACK_SIZE 1024

#define PRIO_COOP (-1)
#define PRIO_HIGH 3
#define PRIO_MED  5
#define PRIO_LOW  7

#define COOP_ITER 5

void t_coop_fn(void *p1, void *p2, void *p3)
{
    while(1) {
	LOG_INF("T_COOP running");

	for(int i=0; i < COOP_ITER; i++) {
	    LOG_INF(" Iteration %d of %d", i, COOP_ITER);
	    k_busy_wait(50000);
	}

	LOG_INF("T_COOP yielding");
	k_yield();

	LOG_INF("T_COOP done");
	k_msleep(500);
    }
}

void t_high_fn(void *p1, void *p2, void *p3)
{
    while (1) {
	LOG_INF("T_HIGH running");
        k_msleep(100);
    }
}

void t_med_fn(void *p1, void *p2, void *p3)
{
    while (1) {
	LOG_INF("T_MED running");
        k_msleep(200);
    }
}

void t_low_fn(void *p1, void *p2, void *p3)
{
    while(1) {
        LOG_INF("T_LOW running");
	k_msleep(300);
    }
}

K_THREAD_DEFINE(thread_c, STACK_SIZE, t_coop_fn,
		NULL, NULL, NULL, PRIO_COOP, 0, 0);
K_THREAD_DEFINE(thread_h, STACK_SIZE, t_high_fn,
                NULL, NULL, NULL, PRIO_HIGH, 0, 0);
K_THREAD_DEFINE(thread_m, STACK_SIZE, t_med_fn,
                NULL, NULL, NULL, PRIO_MED, 0, 0);
K_THREAD_DEFINE(thread_l, STACK_SIZE, t_low_fn,
		NULL, NULL, NULL, PRIO_LOW, 0, 0);

int main(void)
{
    return 0;
}


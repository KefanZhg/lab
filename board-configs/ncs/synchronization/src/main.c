/*
 * Synchronization demo - nRF54L15 DK
 *
 * 演示 semaphore 和 mutex 在真实按键场景中的用法：
 *
 * Button ISR  --k_sem_give-->  worker_thread  --k_mutex_lock-->  led_state
 *                              (消抖 + toggle)                   (共享状态)
 *                                                                     |
 *                                                  reporter_thread <--+
 *                                                  (每 2s 打印状态)
 *
 * Semaphore：ISR 不能 sleep，用 sem give 通知线程有事件
 * Mutex：    worker 和 reporter 都访问 led_state，防止数据竞争
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(sw0),  gpios);
static const struct gpio_dt_spec led    = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

/* ── 共享状态（被 mutex 保护） ──────────────────────────────────────────── */

struct led_state {
	bool     on;
	uint32_t toggle_count;
};

static struct led_state led_state;
K_MUTEX_DEFINE(state_mutex);

/* ── Semaphore：ISR → worker_thread ────────────────────────────────────── */

K_SEM_DEFINE(button_sem, 0, 1);   /* 初始 count=0，上限 1 */

static struct gpio_callback button_cb;

static void button_isr(const struct device *dev,
		       struct gpio_callback *cb, uint32_t pins)
{
	/* ISR 只做一件事：通知 worker 有边沿事件 */
	k_sem_give(&button_sem);
}

/* ── worker_thread：消抖 + 更新 LED 状态 ───────────────────────────────── */

#define WORKER_STACK 1024
K_THREAD_STACK_DEFINE(worker_stack, WORKER_STACK);
static struct k_thread worker_thread;

static void worker_entry(void *a, void *b, void *c)
{
	while (1) {
		/* 等待 ISR 信号 */
		k_sem_take(&button_sem, K_FOREVER);

		/*
		 * 消抖：sleep 50ms 后读电平
		 * sem count 上限是 1，抖动期间多次 give 不会积累超过 1；
		 * sleep 后 reset 丢弃残余，确保只处理一次
		 */
		k_sleep(K_MSEC(50));
		k_sem_reset(&button_sem);

		int pin_state = gpio_pin_get_dt(&button);
		if (pin_state != 0) {
			/* active-low：1 = 释放，忽略 */
			continue;
		}

		/* 确认按下：用 mutex 保护共享状态 */
		k_mutex_lock(&state_mutex, K_FOREVER);
		led_state.on = !led_state.on;
		led_state.toggle_count++;
		uint32_t count = led_state.toggle_count;
		bool     on    = led_state.on;
		k_mutex_unlock(&state_mutex);

		gpio_pin_set_dt(&led, on ? 1 : 0);
		printk("[worker] toggle #%u -> LED %s\n", count, on ? "ON" : "OFF");
	}
}

/* ── reporter_thread：每 2s 打印一次状态 ───────────────────────────────── */

#define REPORTER_STACK 1024
K_THREAD_STACK_DEFINE(reporter_stack, REPORTER_STACK);
static struct k_thread reporter_thread;

static void reporter_entry(void *a, void *b, void *c)
{
	while (1) {
		k_sleep(K_SECONDS(2));

		k_mutex_lock(&state_mutex, K_FOREVER);
		bool     on    = led_state.on;
		uint32_t count = led_state.toggle_count;
		k_mutex_unlock(&state_mutex);

		printk("[reporter] LED=%s  total toggles=%u\n",
		       on ? "ON" : "OFF", count);
	}
}

/* ── main ───────────────────────────────────────────────────────────────── */

int main(void)
{
	printk("Sync demo: semaphore + mutex\n");
	printk("Press SW0 to toggle LED0\n");

	gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&button, GPIO_INPUT);
	gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_BOTH);
	gpio_init_callback(&button_cb, button_isr, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb);

	k_thread_create(&worker_thread, worker_stack, WORKER_STACK,
			worker_entry, NULL, NULL, NULL, 5, 0, K_NO_WAIT);
	k_thread_name_set(&worker_thread, "worker");

	k_thread_create(&reporter_thread, reporter_stack, REPORTER_STACK,
			reporter_entry, NULL, NULL, NULL, 7, 0, K_NO_WAIT);
	k_thread_name_set(&reporter_thread, "reporter");

	return 0;
}

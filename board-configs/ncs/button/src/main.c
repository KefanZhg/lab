/*
 * Button input demo - nRF54L15 DK
 *
 * 模式：GPIO 中断 + k_work_delayable 消抖
 *
 * 消抖原理：
 *   1. 按键边沿触发 ISR（rising + falling 双边沿）
 *   2. ISR 里只做一件事：k_work_reschedule 提交 50ms 后执行的 delayed work
 *      - 抖动期间多次触发 ISR，reschedule 会不断把执行时间推后
 *      - 抖动结束后 50ms，work handler 才真正执行一次
 *   3. work handler 读取当前 GPIO 电平，确认是真实的按下或释放
 *
 * 按键 SW0 控制 LED0 toggle（每次确认按下翻转一次）
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>

/* sw0 = button0 (P1.13, active low), led0 = LED0 (P2.9, active high) */
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
static const struct gpio_dt_spec led    = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

static struct gpio_callback button_cb;
static struct k_work_delayable debounce_work;

#define DEBOUNCE_MS 50

/* work handler：抖动平息后执行，读电平确认 */
static void debounce_handler(struct k_work *work)
{
	int state = gpio_pin_get_dt(&button);

	if (state == 0) {
		/* active low：0 = 按下 */
		printk("Button pressed (confirmed)\n");
		gpio_pin_toggle_dt(&led);
	} else {
		printk("Button released (confirmed)\n");
	}
}

/* GPIO ISR：仅负责重新调度 delayed work，不做任何逻辑 */
static void button_isr(const struct device *dev,
		       struct gpio_callback *cb,
		       uint32_t pins)
{
	/* reschedule 会重置倒计时；抖动期间反复触发只会推后执行时间 */
	k_work_reschedule(&debounce_work, K_MSEC(DEBOUNCE_MS));
}

int main(void)
{
	printk("Button debounce demo start\n");

	/* 配置 LED 输出 */
	gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);

	/* 配置按键输入，双边沿中断 */
	gpio_pin_configure_dt(&button, GPIO_INPUT);
	gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_BOTH);

	/* 注册 ISR */
	gpio_init_callback(&button_cb, button_isr, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb);

	/* 初始化 delayed work */
	k_work_init_delayable(&debounce_work, debounce_handler);

	printk("Press SW0 to toggle LED0\n");

	k_sleep(K_FOREVER);
	return 0;
}

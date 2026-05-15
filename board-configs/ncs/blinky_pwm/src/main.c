/*
 * 4-LED demo for nRF54L15 DK
 *
 * LED0 (P2.9, GPIO): thread    - software frequency modulation, mirrors LED1's sweep rate
 * LED1 (P1.10, PWM20): thread  - hardware frequency modulation, period sweeps 1s -> ~8ms -> 1s
 * LED2 (P2.7, GPIO): k_timer   - software amplitude modulation, 20ms period, duty cycle sweeps
 * LED3 (P1.14, PWM21): thread  - hardware amplitude modulation, 20ms period, duty cycle sweeps
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/gpio.h>

/* PWM specs: pwm-led0 -> LED1 (P1.10), pwm-led1 -> LED3 (P1.14) */
static const struct pwm_dt_spec pwm_led1 = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led0));
static const struct pwm_dt_spec pwm_led3 = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led1));

/* GPIO specs: led0 -> P2.9, led2 -> P2.7 */
static const struct gpio_dt_spec gpio_led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec gpio_led2 = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

#define FREQ_MAX_PERIOD_NS  PWM_SEC(1U)
#define FREQ_MIN_PERIOD_NS  (PWM_SEC(1U) / 128U)
#define AMP_PERIOD_NS       PWM_MSEC(20U)
#define AMP_STEPS           20u
#define AMP_STEP_MS         80u    /* full sweep: ~1.6s per direction */
#define FREQ_STEP_SLEEP_MS  4000u

/* LED0 reads this to mirror LED1's current period */
static volatile uint32_t shared_freq_half_period_ms = 500u;

/* ── LED1: hardware PWM frequency modulation ─────────────────────────── */

#define LED1_STACK 1024
K_THREAD_STACK_DEFINE(led1_stack, LED1_STACK);
static struct k_thread led1_thread;

static void led1_entry(void *a, void *b, void *c)
{
	uint32_t max_period = FREQ_MAX_PERIOD_NS;
	uint32_t period;
	uint8_t dir = 0;

	/* calibrate: find largest period the hardware accepts */
	while (pwm_set_dt(&pwm_led1, max_period, max_period / 2u)) {
		max_period /= 2u;
		if (max_period < 4u * FREQ_MIN_PERIOD_NS) {
			printk("LED1: PWM calibration failed\n");
			return;
		}
	}
	printk("LED1: freq-mod ready, max period %u ns\n", max_period);

	period = max_period;
	while (1) {
		pwm_set_dt(&pwm_led1, period, period / 2u);

		/* publish half-period in ms for LED0 to follow */
		uint32_t half_ms = period / 2u / 1000000u;
		shared_freq_half_period_ms = (half_ms < 4u) ? 4u : half_ms;

		period = dir ? (period * 2u) : (period / 2u);
		if (period > max_period) {
			period = max_period / 2u;
			dir = 0;
		} else if (period < FREQ_MIN_PERIOD_NS) {
			period = FREQ_MIN_PERIOD_NS * 2u;
			dir = 1;
		}
		k_sleep(K_MSEC(FREQ_STEP_SLEEP_MS));
	}
}

/* ── LED3: hardware PWM amplitude modulation ─────────────────────────── */

#define LED3_STACK 1024
K_THREAD_STACK_DEFINE(led3_stack, LED3_STACK);
static struct k_thread led3_thread;

static void led3_entry(void *a, void *b, void *c)
{
	const uint32_t step = AMP_PERIOD_NS / AMP_STEPS;
	uint32_t duty = 0;
	uint8_t dir = 0;

	printk("LED3: amp-mod ready, period %u ns, %u steps\n",
	       AMP_PERIOD_NS, AMP_STEPS);

	while (1) {
		pwm_set_dt(&pwm_led3, AMP_PERIOD_NS, duty);

		if (!dir) {
			duty += step;
			if (duty >= AMP_PERIOD_NS) {
				duty = AMP_PERIOD_NS;
				dir = 1;
			}
		} else {
			duty = (duty > step) ? duty - step : 0u;
			if (duty == 0u) {
				dir = 0;
			}
		}
		k_sleep(K_MSEC(AMP_STEP_MS));
	}
}

/* ── LED0: GPIO software frequency modulation (mirrors LED1) ─────────── */

#define LED0_STACK 1024
K_THREAD_STACK_DEFINE(led0_stack, LED0_STACK);
static struct k_thread led0_thread;

static void led0_entry(void *a, void *b, void *c)
{
	while (1) {
		uint32_t half_ms = shared_freq_half_period_ms;

		gpio_pin_toggle_dt(&gpio_led0);
		k_sleep(K_MSEC(half_ms));
	}
}

/* ── LED2: k_timer software amplitude modulation (mirrors LED3) ──────── */
/*
 * 用法：timer callback 在中断上下文运行，不能调用 k_sleep。
 * 用两阶段（phase=0: off→on，phase=1: on→off）重新 start timer，
 * 实现软件 PWM。每完成一个完整周期更新一次 duty。
 */

#define LED2_PERIOD_MS  20u
#define LED2_STEP_MS     1u
#define LED2_DUTY_EVERY  4u   /* 每 4 个周期更新一次 duty，约 80ms */

static struct k_timer led2_timer;
static uint32_t led2_on_ms;   /* 当前 duty 的 on 时间（ms） */
static uint8_t  led2_phase;   /* 0=即将开灯，1=即将关灯 */
static uint8_t  led2_dir;     /* 0=递增，1=递减 */
static uint32_t led2_pcnt;    /* 已完成的完整周期计数 */

static void led2_timer_cb(struct k_timer *t)
{
	if (led2_phase == 0) {
		/* 开灯，调度 on_ms 后关灯 */
		gpio_pin_set_dt(&gpio_led2, 1);
		led2_phase = 1;
		uint32_t next = (led2_on_ms > 0u) ? led2_on_ms : LED2_PERIOD_MS;
		k_timer_start(t, K_MSEC(next), K_NO_WAIT);
	} else {
		/* 关灯，调度 off_ms 后开灯；同时更新 duty */
		gpio_pin_set_dt(&gpio_led2, 0);
		led2_phase = 0;
		uint32_t off = (led2_on_ms < LED2_PERIOD_MS)
			? LED2_PERIOD_MS - led2_on_ms : LED2_PERIOD_MS;
		k_timer_start(t, K_MSEC(off), K_NO_WAIT);

		if (++led2_pcnt >= LED2_DUTY_EVERY) {
			led2_pcnt = 0u;
			if (!led2_dir) {
				led2_on_ms += LED2_STEP_MS;
				if (led2_on_ms >= LED2_PERIOD_MS) {
					led2_on_ms = LED2_PERIOD_MS;
					led2_dir = 1;
				}
			} else {
				led2_on_ms = (led2_on_ms > LED2_STEP_MS)
					? led2_on_ms - LED2_STEP_MS : 0u;
				if (led2_on_ms == 0u) {
					led2_dir = 0;
				}
			}
		}
	}
}

/* ── main ─────────────────────────────────────────────────────────────── */

int main(void)
{
	printk("4-LED demo start\n");

	if (!pwm_is_ready_dt(&pwm_led1)) {
		printk("Error: pwm_led1 (LED1) not ready\n");
		return 0;
	}
	if (!pwm_is_ready_dt(&pwm_led3)) {
		printk("Error: pwm_led3 (LED3) not ready\n");
		return 0;
	}
	if (!gpio_is_ready_dt(&gpio_led0) || !gpio_is_ready_dt(&gpio_led2)) {
		printk("Error: GPIO LEDs not ready\n");
		return 0;
	}

	gpio_pin_configure_dt(&gpio_led0, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&gpio_led2, GPIO_OUTPUT_INACTIVE);

	/* 启动三个线程 */
	k_thread_create(&led1_thread, led1_stack, LED1_STACK,
			led1_entry, NULL, NULL, NULL, 5, 0, K_NO_WAIT);
	k_thread_create(&led3_thread, led3_stack, LED3_STACK,
			led3_entry, NULL, NULL, NULL, 5, 0, K_NO_WAIT);
	k_thread_create(&led0_thread, led0_stack, LED0_STACK,
			led0_entry, NULL, NULL, NULL, 5, 0, K_NO_WAIT);

	/* LED2：软件定时器，立即触发第一次（phase=0，开灯） */
	k_timer_init(&led2_timer, led2_timer_cb, NULL);
	k_timer_start(&led2_timer, K_NO_WAIT, K_NO_WAIT);

	return 0;
}

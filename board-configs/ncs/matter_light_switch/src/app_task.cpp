/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"

#include "light_switch.h"

#include "app/matter_init.h"
#include "app/task_executor.h"
#include "board/board.h"
#include "clusters/identify.h"

#include <setup_payload/OnboardingCodesUtil.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;
using namespace ::chip::DeviceLayer;

namespace
{
constexpr uint32_t kDimmerTriggeredTimeout = 500;
constexpr uint32_t kDimmerInterval = 300;

/* EP1 = BTN2, EP2 = BTN3, EP3 = BTN4 */
struct ButtonCtx {
	k_timer dimmerTriggerTimer;
	k_timer dimmerTimer;
	EndpointId ep;
	bool wasDimmerTriggered;
};

static ButtonCtx sBtn[3];

Nrf::Matter::IdentifyCluster sIdentifyCluster(1 /* EP1 */);
} /* namespace */

static ButtonCtx *CtxFromTimer(k_timer *timer)
{
	for (auto &ctx : sBtn) {
		if (timer == &ctx.dimmerTriggerTimer || timer == &ctx.dimmerTimer) {
			return &ctx;
		}
	}
	return nullptr;
}

static void DimmerTriggerTimeoutCallback(k_timer *timer)
{
	ButtonCtx *ctx = CtxFromTimer(timer);
	if (!ctx) {
		return;
	}
	EndpointId ep = ctx->ep;
	Nrf::PostTask([ep]() {
		/* Find the ctx again inside the task (safe: sBtn is static) */
		ButtonCtx *c = nullptr;
		for (auto &b : sBtn) {
			if (b.ep == ep) { c = &b; break; }
		}
		if (!c) return;

		LOG_INF("Dimming started on EP%u...", ep);
		c->wasDimmerTriggered = true;
		LightSwitch::GetInstance().InitiateActionSwitch(ep, LightSwitch::Action::On);
		k_timer_start(&c->dimmerTimer, K_MSEC(kDimmerInterval), K_MSEC(kDimmerInterval));
		k_timer_stop(&c->dimmerTriggerTimer);
	});
}

static void DimmerTimeoutCallback(k_timer *timer)
{
	ButtonCtx *ctx = CtxFromTimer(timer);
	if (!ctx) {
		return;
	}
	EndpointId ep = ctx->ep;
	Nrf::PostTask([ep]() {
		LightSwitch::GetInstance().DimmerChangeBrightness(ep);
	});
}

static void HandleButtonRelease(ButtonCtx *ctx)
{
	EndpointId ep = ctx->ep;
	Nrf::PostTask([ep]() {
		ButtonCtx *c = nullptr;
		for (auto &b : sBtn) {
			if (b.ep == ep) { c = &b; break; }
		}
		if (!c) return;

		if (!c->wasDimmerTriggered) {
			LightSwitch::GetInstance().InitiateActionSwitch(ep, LightSwitch::Action::Toggle);
		}
		k_timer_stop(&c->dimmerTimer);
		k_timer_stop(&c->dimmerTriggerTimer);
		c->wasDimmerTriggered = false;
	});
}

void AppTask::ButtonEventHandler(Nrf::ButtonState state, Nrf::ButtonMask hasChanged)
{
	/* BTN2 → EP1 */
	if (DK_BTN2_MSK & hasChanged) {
		if (DK_BTN2_MSK & state) {
			LOG_INF("BTN2 pressed (EP1)");
			k_timer_start(&sBtn[0].dimmerTriggerTimer, K_MSEC(kDimmerTriggeredTimeout), K_NO_WAIT);
		} else {
			HandleButtonRelease(&sBtn[0]);
		}
	}

	/* BTN3 → EP2 */
	if (DK_BTN3_MSK & hasChanged) {
		if (DK_BTN3_MSK & state) {
			LOG_INF("BTN3 pressed (EP2)");
			k_timer_start(&sBtn[1].dimmerTriggerTimer, K_MSEC(kDimmerTriggeredTimeout), K_NO_WAIT);
		} else {
			HandleButtonRelease(&sBtn[1]);
		}
	}

	/* BTN4 → EP3 */
	if (DK_BTN4_MSK & hasChanged) {
		if (DK_BTN4_MSK & state) {
			LOG_INF("BTN4 pressed (EP3)");
			k_timer_start(&sBtn[2].dimmerTriggerTimer, K_MSEC(kDimmerTriggeredTimeout), K_NO_WAIT);
		} else {
			HandleButtonRelease(&sBtn[2]);
		}
	}
}

CHIP_ERROR AppTask::Init()
{
	/* Initialize Matter stack */
	ReturnErrorOnFailure(Nrf::Matter::PrepareServer(Nrf::Matter::InitData{ .mPostServerInitClbk = [] {
		LightSwitch::GetInstance().Init(1 /* EP1 */);
		return CHIP_NO_ERROR;
	} }));

	/* Initialize per-button contexts and timers */
	sBtn[0] = { .ep = 1, .wasDimmerTriggered = false };
	sBtn[1] = { .ep = 2, .wasDimmerTriggered = false };
	sBtn[2] = { .ep = 3, .wasDimmerTriggered = false };

	for (auto &ctx : sBtn) {
		k_timer_init(&ctx.dimmerTriggerTimer, DimmerTriggerTimeoutCallback, nullptr);
		k_timer_init(&ctx.dimmerTimer, DimmerTimeoutCallback, nullptr);
	}

	if (!Nrf::GetBoard().Init(ButtonEventHandler)) {
		LOG_ERR("User interface initialization failed.");
		return CHIP_ERROR_INCORRECT_STATE;
	}

	ReturnErrorOnFailure(Nrf::Matter::RegisterEventHandler(Nrf::Board::DefaultMatterEventHandler, 0));

	ReturnErrorOnFailure(sIdentifyCluster.Init());

	return Nrf::Matter::StartServer();
}

CHIP_ERROR AppTask::StartApp()
{
	ReturnErrorOnFailure(Init());

	while (true) {
		Nrf::DispatchNextTask();
	}

	return CHIP_NO_ERROR;
}

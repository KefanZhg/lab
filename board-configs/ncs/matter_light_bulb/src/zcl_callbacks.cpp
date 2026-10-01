/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <lib/support/logging/CHIPLogging.h>

#include "app_task.h"

#include <app-common/zap-generated/attributes/Accessors.h>
#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app/ConcreteAttributePath.h>

using namespace ::chip;
using namespace ::chip::app::Clusters;
using namespace ::chip::app::Clusters::OnOff;

static void SetLedForEndpoint(EndpointId ep, bool on)
{
	switch (ep) {
	case 2:
		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED3).Set(on);
		break;
	case 3:
		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED4).Set(on);
		break;
	default:
		break;
	}
}

void MatterPostAttributeChangeCallback(const chip::app::ConcreteAttributePath &attributePath, uint8_t type,
				       uint16_t size, uint8_t *value)
{
	ClusterId clusterId = attributePath.mClusterId;
	AttributeId attributeId = attributePath.mAttributeId;
	EndpointId ep = attributePath.mEndpointId;

	if (clusterId == OnOff::Id && attributeId == OnOff::Attributes::OnOff::Id) {
		ChipLogProgress(Zcl, "EP%u OnOff -> %" PRIu8, ep, *value);

		if (ep == 1) {
#if defined(CONFIG_PWM)
			AppTask::Instance().GetPWMDevice().InitiateAction(
				*value ? Nrf::PWMDevice::ON_ACTION : Nrf::PWMDevice::OFF_ACTION,
				static_cast<int32_t>(LightingActor::Remote), value);
#else
			Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Set(*value);
#endif
		} else {
			SetLedForEndpoint(ep, *value);
		}

	} else if (clusterId == LevelControl::Id && attributeId == LevelControl::Attributes::CurrentLevel::Id) {
		ChipLogProgress(Zcl, "EP%u LevelControl -> %" PRIu8, ep, *value);

		if (ep == 1) {
#if defined(CONFIG_PWM)
			if (AppTask::Instance().GetPWMDevice().IsTurnedOn()) {
				AppTask::Instance().GetPWMDevice().InitiateAction(
					Nrf::PWMDevice::LEVEL_ACTION,
					static_cast<int32_t>(LightingActor::Remote), value);
			}
#endif
		}
		/* EP2/EP3 are GPIO-only: LevelControl has no effect */
	}
}

void emberAfOnOffClusterInitCallback(EndpointId endpoint)
{
	Protocols::InteractionModel::Status status;
	bool storedValue;

	status = Attributes::OnOff::Get(endpoint, &storedValue);
	if (status != Protocols::InteractionModel::Status::Success) {
		return;
	}

	if (endpoint == 1) {
#if defined(CONFIG_PWM)
		AppTask::Instance().InitPWMDDevice();
		AppTask::Instance().GetPWMDevice().InitiateAction(
			storedValue ? Nrf::PWMDevice::ON_ACTION : Nrf::PWMDevice::OFF_ACTION,
			static_cast<int32_t>(LightingActor::Remote),
			reinterpret_cast<uint8_t *>(&storedValue));
#else
		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Set(storedValue);
#endif
	} else {
		SetLedForEndpoint(endpoint, storedValue);
	}

	AppTask::Instance().UpdateClusterState();
}

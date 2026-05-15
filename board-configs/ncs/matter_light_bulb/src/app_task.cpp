/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"

#ifdef CONFIG_AWS_IOT_INTEGRATION
#include "aws_iot_integration.h"
#endif

#include "app/matter_init.h"
#include "app/task_executor.h"

#if defined(CONFIG_PWM)
#include "pwm/pwm_device.h"
#endif

#include "clusters/identify.h"

#include <app-common/zap-generated/attributes/Accessors.h>
#include <app/persistence/AttributePersistenceProviderInstance.h>
#include <app/persistence/DefaultAttributePersistenceProvider.h>
#include <app/persistence/DeferredAttributePersistenceProvider.h>
#include <app/server/Server.h>
#include <setup_payload/OnboardingCodesUtil.h>
#include <lib/shell/Engine.h>
#include <access/AccessControl.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;
using namespace ::chip::DeviceLayer;

namespace
{
constexpr EndpointId kLightEndpointId = 1;
constexpr uint8_t kDefaultMinLevel = 0;
constexpr uint8_t kDefaultMaxLevel = 254;

Nrf::Matter::IdentifyCluster sIdentifyCluster(kLightEndpointId, true, []() {
	Nrf::PostTask([] { Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Set(false); });
#if defined(CONFIG_PWM)
	Nrf::PostTask([] { AppTask::Instance().GetPWMDevice().ApplyLevel(); });
#endif
});

#if defined(CONFIG_PWM)
const struct pwm_dt_spec sLightPwmDevice = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led1));
#endif

/* Define a custom attribute persister which makes actual write of the CurrentLevel attribute value
 * to the non-volatile storage only when it has remained constant for 5 seconds. This is to reduce
 * the flash wearout when the attribute changes frequently as a result of MoveToLevel command.
 * DeferredAttribute object describes a deferred attribute, but also holds a buffer with a value to
 * be written, so it must live so long as the DeferredAttributePersistenceProvider object.
 */
DeferredAttribute gCurrentLevelPersister(ConcreteAttributePath(kLightEndpointId, Clusters::LevelControl::Id,
							       Clusters::LevelControl::Attributes::CurrentLevel::Id));

/* Deferred persistence will be auto-initialized as soon as the default persistence is initialized */
DefaultAttributePersistenceProvider gSimpleAttributePersistence;
DeferredAttributePersistenceProvider gDeferredAttributePersister(gSimpleAttributePersistence,
								 Span<DeferredAttribute>(&gCurrentLevelPersister, 1),
								 System::Clock::Milliseconds32(5000));

#define APPLICATION_BUTTON_MASK DK_BTN2_MSK
} /* namespace */

void AppTask::LightingActionEventHandler(const LightingEvent &event)
{
#if defined(CONFIG_PWM)
	Nrf::PWMDevice::Action_t action = Nrf::PWMDevice::INVALID_ACTION;
	int32_t actor = 0;
	if (event.Actor == LightingActor::Button) {
		action = Instance().mPWMDevice.IsTurnedOn() ? Nrf::PWMDevice::OFF_ACTION : Nrf::PWMDevice::ON_ACTION;
		actor = static_cast<int32_t>(event.Actor);
	}

	if (action == Nrf::PWMDevice::INVALID_ACTION || !Instance().mPWMDevice.InitiateAction(action, actor, NULL)) {
		LOG_INF("An action could not be initiated.");
	}
#else
	Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Set(!Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).GetState());
#endif
}

void AppTask::ButtonEventHandler(Nrf::ButtonState state, Nrf::ButtonMask hasChanged)
{
	if ((APPLICATION_BUTTON_MASK & hasChanged) & state) {
		Nrf::PostTask([] {
			LightingEvent event;
			event.Actor = LightingActor::Button;
			LightingActionEventHandler(event);
		});
	}
}

#ifdef CONFIG_AWS_IOT_INTEGRATION
bool AppTask::AWSIntegrationCallback(struct aws_iot_integration_cb_data *data)
{
	LOG_INF("Attribute change requested from AWS IoT: %d", data->value);

	Protocols::InteractionModel::Status status;

	VerifyOrDie(data->error == 0);

	if (data->attribute_id == ATTRIBUTE_ID_ONOFF) {
		/* write the new on/off value */
		status = Clusters::OnOff::Attributes::OnOff::Set(kLightEndpointId, data->value);
		if (status != Protocols::InteractionModel::Status::Success) {
			LOG_ERR("Updating on/off cluster failed: %x", to_underlying(status));
			return false;
		}
	} else if (data->attribute_id == ATTRIBUTE_ID_LEVEL_CONTROL) {
		/* write the current level */
		status = Clusters::LevelControl::Attributes::CurrentLevel::Set(kLightEndpointId, data->value);

		if (status != Protocols::InteractionModel::Status::Success) {
			LOG_ERR("Updating level cluster failed: %x", to_underlying(status));
			return false;
		}
	}

	return true;
}
#endif /* CONFIG_AWS_IOT_INTEGRATION */

#if defined(CONFIG_PWM)
void AppTask::ActionInitiated(Nrf::PWMDevice::Action_t action, int32_t actor)
{
	if (action == Nrf::PWMDevice::ON_ACTION) {
		LOG_INF("Turn On Action has been initiated");
	} else if (action == Nrf::PWMDevice::OFF_ACTION) {
		LOG_INF("Turn Off Action has been initiated");
	} else if (action == Nrf::PWMDevice::LEVEL_ACTION) {
		LOG_INF("Level Action has been initiated");
	}
}

void AppTask::ActionCompleted(Nrf::PWMDevice::Action_t action, int32_t actor)
{
	if (action == Nrf::PWMDevice::ON_ACTION) {
		LOG_INF("Turn On Action has been completed");
	} else if (action == Nrf::PWMDevice::OFF_ACTION) {
		LOG_INF("Turn Off Action has been completed");
	} else if (action == Nrf::PWMDevice::LEVEL_ACTION) {
		LOG_INF("Level Action has been completed");
	}

	if (actor == static_cast<int32_t>(LightingActor::Button)) {
		Instance().UpdateClusterState();
	}
}
#endif /* CONFIG_PWM */

void AppTask::UpdateClusterState()
{
	SystemLayer().ScheduleLambda([this] {
#if defined(CONFIG_PWM)
		/* write the new on/off value */
		Protocols::InteractionModel::Status status =
			Clusters::OnOff::Attributes::OnOff::Set(kLightEndpointId, mPWMDevice.IsTurnedOn());
#else
		Protocols::InteractionModel::Status status = Clusters::OnOff::Attributes::OnOff::Set(
			kLightEndpointId, Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).GetState());
#endif
		if (status != Protocols::InteractionModel::Status::Success) {
			LOG_ERR("Updating on/off cluster failed: %x", to_underlying(status));
		}

#if defined(CONFIG_PWM)
		/* write the current level */
		status = Clusters::LevelControl::Attributes::CurrentLevel::Set(kLightEndpointId, mPWMDevice.GetLevel());
#else
		/* write the current level */
		if (Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).GetState()) {
			status = Clusters::LevelControl::Attributes::CurrentLevel::Set(kLightEndpointId, 100);
		} else {
			status = Clusters::LevelControl::Attributes::CurrentLevel::Set(kLightEndpointId, 0);
		}
#endif

		if (status != Protocols::InteractionModel::Status::Success) {
			LOG_ERR("Updating level cluster failed: %x", to_underlying(status));
		}
	});
}

void AppTask::InitPWMDDevice()
{
#if defined(CONFIG_PWM)
	/* Initialize lighting device (PWM) */
	uint8_t minLightLevel = kDefaultMinLevel;
	Clusters::LevelControl::Attributes::MinLevel::Get(kLightEndpointId, &minLightLevel);

	uint8_t maxLightLevel = kDefaultMaxLevel;
	Clusters::LevelControl::Attributes::MaxLevel::Get(kLightEndpointId, &maxLightLevel);

	Clusters::LevelControl::Attributes::CurrentLevel::TypeInfo::Type currentLevel;
	Clusters::LevelControl::Attributes::CurrentLevel::Get(kLightEndpointId, currentLevel);

	int ret =
		mPWMDevice.Init(&sLightPwmDevice, minLightLevel, maxLightLevel, currentLevel.ValueOr(kDefaultMaxLevel));
	if (ret != 0) {
		LOG_ERR("Failed to initialize PWD device.");
	}

	mPWMDevice.SetCallbacks(ActionInitiated, ActionCompleted);
#endif
}

// matter acl grant <nodeId_hex>  — add Operate ACL entry for a peer switch node
static CHIP_ERROR AclGrantCommandHandler(int argc, char **argv)
{
	using namespace chip::Shell;
	// argv[0] may be "grant" subword or the nodeId directly
	const char *nodeIdStr = nullptr;
	if (argc >= 2 && strcmp(argv[0], "grant") == 0) {
		nodeIdStr = argv[1];
	} else if (argc >= 1) {
		nodeIdStr = argv[0];
	} else {
		streamer_printf(streamer_get(), "Usage: acl grant <nodeId_hex>\r\n");
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

	chip::NodeId nodeId = (chip::NodeId)strtoull(nodeIdStr, nullptr, 0);

	chip::FabricIndex fabricIndex = chip::kUndefinedFabricIndex;
	for (const chip::FabricInfo & fabric : chip::Server::GetInstance().GetFabricTable()) {
		fabricIndex = fabric.GetFabricIndex();
		break;
	}
	if (fabricIndex == chip::kUndefinedFabricIndex) {
		streamer_printf(streamer_get(), "Error: not commissioned\r\n");
		return CHIP_ERROR_INCORRECT_STATE;
	}

	chip::Access::AccessControl::Entry entry;
	CHIP_ERROR err = chip::Access::GetAccessControl().PrepareEntry(entry);
	if (err != CHIP_NO_ERROR) {
		streamer_printf(streamer_get(), "PrepareEntry failed: %s\r\n", chip::ErrorStr(err));
		return err;
	}

	entry.SetFabricIndex(fabricIndex);
	entry.SetPrivilege(chip::Access::Privilege::kOperate);
	entry.SetAuthMode(chip::Access::AuthMode::kCase);
	entry.AddSubject(nullptr, nodeId);

	err = chip::Access::GetAccessControl().CreateEntry(nullptr, entry);
	if (err != CHIP_NO_ERROR) {
		streamer_printf(streamer_get(), "CreateEntry failed: %s\r\n", chip::ErrorStr(err));
		return err;
	}

	streamer_printf(streamer_get(), "ACL granted Operate to node 0x%llx on fabric %u\r\n",
			(unsigned long long)nodeId, fabricIndex);
	return CHIP_NO_ERROR;
}

static CHIP_ERROR NodeIdCommandHandler(int argc, char **argv)
{
	using namespace chip::Shell;
	bool found = false;
	for (const chip::FabricInfo & fabric : chip::Server::GetInstance().GetFabricTable()) {
		streamer_printf(streamer_get(), "Fabric %u  NodeId: 0x%llx\r\n",
				fabric.GetFabricIndex(),
				(unsigned long long)fabric.GetNodeId());
		found = true;
	}
	if (!found) {
		streamer_printf(streamer_get(), "Not commissioned yet\r\n");
	}
	return CHIP_NO_ERROR;
}

CHIP_ERROR AppTask::Init()
{
	/* Register shell commands */
	static const chip::Shell::shell_command_t sShellCmds[] = {
		{ &NodeIdCommandHandler, "nodeid", "Print this device's Matter node ID on each fabric" },
		{ &AclGrantCommandHandler, "acl", "Grant Operate ACL to a peer: acl grant <nodeId_hex>" },
	};
	chip::Shell::Engine::Root().RegisterCommands(sShellCmds, MATTER_ARRAY_SIZE(sShellCmds));

	/* Initialize Matter stack */
	ReturnErrorOnFailure(Nrf::Matter::PrepareServer(Nrf::Matter::InitData{ .mPostServerInitClbk = []() {
		app::SetAttributePersistenceProvider(&gDeferredAttributePersister);
		gSimpleAttributePersistence.Init(Nrf::Matter::GetPersistentStorageDelegate());
		return CHIP_NO_ERROR;
	} }));

	if (!Nrf::GetBoard().Init(ButtonEventHandler)) {
		LOG_ERR("User interface initialization failed.");
		return CHIP_ERROR_INCORRECT_STATE;
	}

	/* Register Matter event handler that controls the connectivity status LED based on the captured Matter network
	 * state. */
	ReturnErrorOnFailure(Nrf::Matter::RegisterEventHandler(Nrf::Board::DefaultMatterEventHandler, 0));

#ifdef CONFIG_AWS_IOT_INTEGRATION
	int retAws = aws_iot_integration_register_callback(AWSIntegrationCallback);
	if (retAws) {
		LOG_ERR("aws_iot_integration_register_callback() failed");
		return chip::System::MapErrorZephyr(retAws);
	}
#endif

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

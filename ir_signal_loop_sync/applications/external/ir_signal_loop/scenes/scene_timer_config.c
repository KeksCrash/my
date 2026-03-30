#include "../timed_remote.h"
#include "timed_remote_scene.h"

enum
{
	ITEM_TX_PIN,
	ITEM_OTG,
	ITEM_START,
};

static const char *tx_pin_text[] = {
	"Flipper",
	"2 (A7)",
	"Detect",
};

static const char *otg_text[] = {
	"OFF",
	"ON",
};

static void	build_items(TimedRemoteApp *);
static uint8_t	tx_pin_index(const TimedRemoteApp *);
static FuriHalInfraredTxPin tx_pin_from_index(uint8_t);

static uint8_t
tx_pin_index(const TimedRemoteApp *app)
{
	if (app->tx_pin == FuriHalInfraredTxPinInternal)
		return 0;
	if (app->tx_pin == FuriHalInfraredTxPinExtPA7)
		return 1;
	return 2;
}

static FuriHalInfraredTxPin
tx_pin_from_index(uint8_t index)
{
	if (index == 0)
		return FuriHalInfraredTxPinInternal;
	if (index == 1)
		return FuriHalInfraredTxPinExtPA7;
	return FuriHalInfraredTxPinMax;
}

static void
on_tx_pin_change(VariableItem *item)
{
	TimedRemoteApp *app;
	uint8_t index;

	app = variable_item_get_context(item);
	index = variable_item_get_current_value_index(item);

	app->tx_pin = tx_pin_from_index(index);
	variable_item_set_current_value_text(item, tx_pin_text[index]);
	view_dispatcher_send_custom_event(app->vd, EVENT_TX_PIN_CHANGED);
}

static void
on_otg_change(VariableItem *item)
{
	TimedRemoteApp *app;
	uint8_t index;

	app = variable_item_get_context(item);
	index = variable_item_get_current_value_index(item);
	app->is_otg_enabled = index == 1;
	variable_item_set_current_value_text(item, otg_text[index]);
	view_dispatcher_send_custom_event(app->vd, EVENT_OTG_CHANGED);
}

static void
on_enter(void *context, uint32_t index)
{
	TimedRemoteApp *app;

	app = context;
	if (index != ITEM_START)
		return;

	view_dispatcher_send_custom_event(app->vd, EVENT_RUN_START);
}

static void
build_items(TimedRemoteApp *app)
{
	VariableItem *item;
	uint8_t index;

	variable_item_list_reset(app->vlist);

	item = variable_item_list_add(
	    app->vlist,
	    "Signal Output",
	    COUNT_OF(tx_pin_text),
	    on_tx_pin_change,
	    app);
	index = tx_pin_index(app);
	variable_item_set_current_value_index(item, index);
	variable_item_set_current_value_text(item, tx_pin_text[index]);

	if (app->tx_pin < FuriHalInfraredTxPinMax) {
		item = variable_item_list_add(
		    app->vlist,
		    "5V on GPIO",
		    COUNT_OF(otg_text),
		    on_otg_change,
		    app);
		index = app->is_otg_enabled ? 1 : 0;
		variable_item_set_current_value_index(item, index);
		variable_item_set_current_value_text(item, otg_text[index]);
	} else {
		item = variable_item_list_add(app->vlist, "5V on GPIO", 1, NULL, app);
		variable_item_set_current_value_index(item, 0);
		variable_item_set_current_value_text(item, "Auto");
	}

	variable_item_list_add(app->vlist, ">> Start Loop <<", 0, NULL, NULL);
	variable_item_list_set_enter_callback(app->vlist, on_enter, app);
}

void
scene_cfg_enter(void *context)
{
	TimedRemoteApp *app;

	app = context;
	build_items(app);
	view_dispatcher_switch_to_view(app->vd, VIEW_LIST);
}

bool
scene_cfg_event(void *context, SceneManagerEvent event)
{
	TimedRemoteApp *app;

	app = context;
	if (event.type != SceneManagerEventTypeCustom)
		return false;

	if (event.event == EVENT_TX_PIN_CHANGED) {
		timed_remote_set_tx_pin(app, app->tx_pin);
		build_items(app);
		return true;
	}
	if (event.event == EVENT_OTG_CHANGED) {
		timed_remote_enable_otg(app, app->is_otg_enabled);
		timed_remote_set_tx_pin(app, app->tx_pin);
		return true;
	}
	if (event.event != EVENT_RUN_START)
		return false;

	app->paused = false;

	scene_manager_next_scene(app->sm, SCENE_RUN);
	return true;
}

void
scene_cfg_exit(void *context)
{
	TimedRemoteApp *app;

	app = context;
	variable_item_list_reset(app->vlist);
}

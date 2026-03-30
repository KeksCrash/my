#include <stdlib.h>
#include <string.h>

#include <power/power_service/power.h>

#include "scenes/timed_remote_scene.h"
#include "timed_remote.h"

extern const SceneManagerHandlers scene_handlers;

void
timed_remote_enable_otg(TimedRemoteApp *app, bool enable)
{
	Power *power;

	if (app == NULL)
		return;

	power = furi_record_open(RECORD_POWER);
	power_enable_otg(power, enable);
	furi_record_close(RECORD_POWER);

	app->is_otg_enabled = enable;
}

void
timed_remote_set_tx_pin(TimedRemoteApp *app, FuriHalInfraredTxPin tx_pin)
{
	FuriHalInfraredTxPin tx_pin_detected;

	if (app == NULL)
		return;

	if (tx_pin < FuriHalInfraredTxPinMax) {
		furi_hal_infrared_set_tx_output(tx_pin);
		if (tx_pin == FuriHalInfraredTxPinInternal) {
			timed_remote_enable_otg(app, false);
		} else {
			timed_remote_enable_otg(app, app->is_otg_enabled);
		}
	} else {
		tx_pin_detected = furi_hal_infrared_detect_tx_output();
		furi_hal_infrared_set_tx_output(tx_pin_detected);
		if (tx_pin_detected != FuriHalInfraredTxPinInternal) {
			timed_remote_enable_otg(app, true);
		} else {
			timed_remote_enable_otg(app, false);
		}
	}

	app->tx_pin = tx_pin;
}

static bool
nav_cb(void *context)
{
	TimedRemoteApp *app;

	app = context;
	return scene_manager_handle_back_event(app->sm);
}

static bool
evt_cb(void *context, uint32_t evt)
{
	TimedRemoteApp *app;

	app = context;
	return scene_manager_handle_custom_event(app->sm, evt);
}

bool
timed_remote_run_input_callback(InputEvent *event, void *context)
{
	TimedRemoteApp *app;

	app = context;
	if (event->key == InputKeyRight && event->type == InputTypeShort) {
		view_dispatcher_send_custom_event(app->vd, EVENT_RUN_TOGGLE_PAUSE);
		return true;
	}

	return false;
}

TimedRemoteApp *
timed_remote_app_alloc(void)
{
	TimedRemoteApp *app;

	app = malloc(sizeof(TimedRemoteApp));
	if (app == NULL)
		return NULL;
	memset(app, 0, sizeof(TimedRemoteApp));

	app->gui = furi_record_open(RECORD_GUI);
	app->dialogs = furi_record_open(RECORD_DIALOGS);

	app->vd = view_dispatcher_alloc();
	view_dispatcher_set_event_callback_context(app->vd, app);
	view_dispatcher_set_navigation_event_callback(app->vd, nav_cb);
	view_dispatcher_set_custom_event_callback(app->vd, evt_cb);
	view_dispatcher_attach_to_gui(
	    app->vd, app->gui, ViewDispatcherTypeFullscreen);

	app->sm = scene_manager_alloc(&scene_handlers, app);

	app->submenu = submenu_alloc();
	view_dispatcher_add_view(
	    app->vd, VIEW_MENU, submenu_get_view(app->submenu));

	app->vlist = variable_item_list_alloc();
	view_dispatcher_add_view(
	    app->vd, VIEW_LIST, variable_item_list_get_view(app->vlist));

	app->widget = widget_alloc();
	view_set_input_callback(
	    widget_get_view(app->widget),
	    timed_remote_run_input_callback);
	view_set_context(widget_get_view(app->widget), app);
	view_dispatcher_add_view(
	    app->vd, VIEW_RUN, widget_get_view(app->widget));

	app->popup = popup_alloc();
	view_dispatcher_add_view(
	    app->vd, VIEW_POP, popup_get_view(app->popup));

	app->tx_pin = FuriHalInfraredTxPinMax;
	app->is_otg_enabled = false;
	app->paused = false;
	app->run_tick_pending = false;
	timed_remote_set_tx_pin(app, app->tx_pin);

	return app;
}

void
timed_remote_app_free(TimedRemoteApp *app)
{
	if (app == NULL)
		return;

	if (app->timer != NULL) {
		furi_timer_stop(app->timer);
		furi_timer_free(app->timer);
	}

	if (app->ir != NULL)
		infrared_signal_free(app->ir);

	timed_remote_set_tx_pin(app, FuriHalInfraredTxPinInternal);
	timed_remote_enable_otg(app, false);

	view_dispatcher_remove_view(app->vd, VIEW_MENU);
	submenu_free(app->submenu);

	view_dispatcher_remove_view(app->vd, VIEW_LIST);
	variable_item_list_free(app->vlist);

	view_dispatcher_remove_view(app->vd, VIEW_RUN);
	widget_free(app->widget);

	view_dispatcher_remove_view(app->vd, VIEW_POP);
	popup_free(app->popup);

	scene_manager_free(app->sm);
	view_dispatcher_free(app->vd);
	furi_record_close(RECORD_DIALOGS);
	furi_record_close(RECORD_GUI);

	free(app);
}

int32_t
ir_signal_loop_app(void *p)
{
	TimedRemoteApp *app;

	UNUSED(p);

	app = timed_remote_app_alloc();
	if (app == NULL)
		return -1;

	scene_manager_next_scene(app->sm, SCENE_BROWSE);
	view_dispatcher_run(app->vd);
	timed_remote_app_free(app);

	return 0;
}

#include <stdio.h>

#include "../helpers/ir_helper.h"
#include "../timed_remote.h"
#include "timed_remote_scene.h"

static void	render_status(TimedRemoteApp *);

#define LOOP_TICK_MS 1U

static void
on_timer_tick(void *context)
{
	TimedRemoteApp *app;

	app = context;
	if (app->paused || app->run_tick_pending)
		return;

	app->run_tick_pending = true;
	view_dispatcher_send_custom_event(app->vd, EVENT_RUN_TICK);
}

static void
render_status(TimedRemoteApp *app)
{
	const char *state_text;

	state_text = app->paused ? "PAUSED" : "SENDING";

	widget_reset(app->widget);
	widget_add_string_element(
	    app->widget,
	    64,
	    2,
	    AlignCenter,
	    AlignTop,
	    FontSecondary,
	    "IR Signal Loop");
	widget_add_string_element(
	    app->widget,
	    64,
	    14,
	    AlignCenter,
	    AlignTop,
	    FontPrimary,
	    app->sig);
	widget_add_string_element(
	    app->widget,
	    64,
	    30,
	    AlignCenter,
	    AlignTop,
	    FontBigNumbers,
	    state_text);
	widget_add_string_element(
	    app->widget,
	    64,
	    50,
	    AlignCenter,
	    AlignTop,
	    FontSecondary,
	    "Right: Pause/Resume");
	widget_add_string_element(
	    app->widget,
	    64,
	    58,
	    AlignCenter,
	    AlignTop,
	    FontSecondary,
	    "Back: Stop");
}

void
scene_run_enter(void *context)
{
	TimedRemoteApp *app;

	app = context;
	timed_remote_set_tx_pin(app, app->tx_pin);
	app->run_tick_pending = false;
	render_status(app);
	view_dispatcher_switch_to_view(app->vd, VIEW_RUN);

	app->timer = furi_timer_alloc(on_timer_tick, FuriTimerTypePeriodic, app);
	if (app->timer == NULL)
		return;

	furi_timer_start(
	    app->timer,
	    (furi_kernel_get_tick_frequency() * LOOP_TICK_MS) / 1000U);
	view_dispatcher_send_custom_event(app->vd, EVENT_RUN_TICK);
}

bool
scene_run_event(void *context, SceneManagerEvent event)
{
	TimedRemoteApp *app;

	app = context;
	if (event.type == SceneManagerEventTypeBack) {
		scene_manager_search_and_switch_to_previous_scene(
		    app->sm,
		    SCENE_BROWSE);
		return true;
	}
	if (event.type != SceneManagerEventTypeCustom)
		return false;

	if (event.event == EVENT_RUN_TOGGLE_PAUSE) {
		app->paused = !app->paused;
		if (app->timer != NULL) {
			if (app->paused) {
				furi_timer_stop(app->timer);
				app->run_tick_pending = false;
			} else {
				app->run_tick_pending = false;
				furi_timer_start(
				    app->timer,
				    (furi_kernel_get_tick_frequency() * LOOP_TICK_MS) /
				        1000U);
				on_timer_tick(app);
			}
		}
		render_status(app);
		return true;
	}

	if (event.event != EVENT_RUN_TICK)
		return false;

	app->run_tick_pending = false;

	if (!app->paused && app->ir != NULL)
		ir_tx(app->ir);

	return true;
}

void
scene_run_exit(void *context)
{
	TimedRemoteApp *app;

	app = context;
	if (app->timer != NULL) {
		furi_timer_stop(app->timer);
		furi_timer_free(app->timer);
		app->timer = NULL;
	}
	app->run_tick_pending = false;
	widget_reset(app->widget);
}

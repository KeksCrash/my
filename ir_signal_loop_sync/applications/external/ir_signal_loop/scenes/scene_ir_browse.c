#include <stdio.h>

#include <dialogs/dialogs.h>
#include "../timed_remote.h"
#include "timed_remote_scene.h"

#define IR_DIRECTORY "/ext/infrared"

void
scene_browse_enter(void *context)
{
	TimedRemoteApp *app;
	DialogsFileBrowserOptions browser_options;
	FuriString *path;

	app = context;

	dialog_file_browser_set_basic_options(&browser_options, ".ir", NULL);
	browser_options.base_path = IR_DIRECTORY;
	browser_options.skip_assets = false;

	path = furi_string_alloc_set(IR_DIRECTORY);
	if (path == NULL) {
		view_dispatcher_stop(app->vd);
		return;
	}

	if (!dialog_file_browser_show(
		app->dialogs,
		path,
		path,
		&browser_options)) {
		furi_string_free(path);
		view_dispatcher_stop(app->vd);
		return;
	}

	snprintf(
	    app->file,
	    sizeof(app->file),
	    "%s",
	    furi_string_get_cstr(path));
	furi_string_free(path);

	view_dispatcher_send_custom_event(app->vd, EVENT_FILE_SELECTED);
}

bool
scene_browse_event(void *context, SceneManagerEvent event)
{
	TimedRemoteApp *app;

	app = context;
	if (event.type != SceneManagerEventTypeCustom)
		return false;
	if (event.event != EVENT_FILE_SELECTED)
		return false;

	scene_manager_next_scene(app->sm, SCENE_SELECT);
	return true;
}

void
scene_browse_exit(void *context)
{
	UNUSED(context);
}

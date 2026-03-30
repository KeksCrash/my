#pragma once

#include <furi.h>
#include <gui/gui.h>
#include <gui/modules/popup.h>
#include <gui/modules/submenu.h>
#include <gui/modules/variable_item_list.h>
#include <gui/modules/widget.h>
#include <gui/scene_manager.h>
#include <gui/view_dispatcher.h>
#include <dialogs/dialogs.h>
#include <furi_hal_infrared.h>
#include <lib/infrared/signal/infrared_signal.h>

typedef enum
{
	VIEW_MENU,
	VIEW_LIST,
	VIEW_RUN,
	VIEW_POP,
} ViewId;

#define SIGNAL_NAME_MAX_LEN 32
#define FILE_PATH_MAX_LEN 256

typedef struct
{
	Gui *gui;
	DialogsApp *dialogs;
	ViewDispatcher *vd;
	SceneManager *sm;

	Submenu *submenu;
	VariableItemList *vlist;
	Widget *widget;
	Popup *popup;

	InfraredSignal *ir;
	char sig[SIGNAL_NAME_MAX_LEN];
	char file[FILE_PATH_MAX_LEN];
	bool paused;
	bool run_tick_pending;
	FuriHalInfraredTxPin tx_pin;
	bool is_otg_enabled;

	FuriTimer *timer;
} TimedRemoteApp;

TimedRemoteApp *timed_remote_app_alloc(void);
void timed_remote_app_free(TimedRemoteApp *);
void timed_remote_set_tx_pin(TimedRemoteApp *, FuriHalInfraredTxPin);
void timed_remote_enable_otg(TimedRemoteApp *, bool);
bool timed_remote_run_input_callback(InputEvent *, void *);

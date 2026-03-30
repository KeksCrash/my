#pragma once

#include <gui/view.h>

typedef struct Hid Hid;
typedef struct HidKeynote HidKeynote;

typedef enum {
	HidKeynoteButtonUp,
	HidKeynoteButtonDown,
	HidKeynoteButtonLeft,
	HidKeynoteButtonRight,
	HidKeynoteButtonOk,
	HidKeynoteButtonBack,
	HidKeynoteButtonCount,
} HidKeynoteButton;

typedef enum {
	HidKeynoteActionArrowUp,
	HidKeynoteActionArrowDown,
	HidKeynoteActionArrowLeft,
	HidKeynoteActionArrowRight,
	HidKeynoteActionW,
	HidKeynoteActionA,
	HidKeynoteActionS,
	HidKeynoteActionD,
	HidKeynoteActionQ,
	HidKeynoteActionE,
	HidKeynoteActionR,
	HidKeynoteActionF,
	HidKeynoteActionT,
	HidKeynoteActionY,
	HidKeynoteActionU,
	HidKeynoteActionI,
	HidKeynoteActionO,
	HidKeynoteActionP,
	HidKeynoteActionG,
	HidKeynoteActionH,
	HidKeynoteActionJ,
	HidKeynoteActionK,
	HidKeynoteActionL,
	HidKeynoteActionZ,
	HidKeynoteActionX,
	HidKeynoteActionC,
	HidKeynoteActionV,
	HidKeynoteActionB,
	HidKeynoteActionN,
	HidKeynoteActionM,
	HidKeynoteActionSpace,
	HidKeynoteActionEnter,
	HidKeynoteActionTab,
	HidKeynoteActionEscape,
	HidKeynoteActionBackspace,
	HidKeynoteActionDelete,
	HidKeynoteActionMouseUp,
	HidKeynoteActionMouseDown,
	HidKeynoteActionMouseLeft,
	HidKeynoteActionMouseRight,
	HidKeynoteActionMouseScrollUp,
	HidKeynoteActionMouseScrollDown,
	HidKeynoteActionMouseLeftClick,
	HidKeynoteActionMouseRightClick,
	HidKeynoteActionBrowserBack,
	HidKeynoteActionCtrl,
	HidKeynoteActionAlt,
	HidKeynoteActionShift,
	HidKeynoteActionGui,
	HidKeynoteActionMediaPlayPause,
	HidKeynoteActionMediaPlay,
	HidKeynoteActionMediaPause,
	HidKeynoteActionMediaStop,
	HidKeynoteActionMediaNext,
	HidKeynoteActionMediaPrev,
	HidKeynoteActionMediaMute,
	HidKeynoteActionMediaVolUp,
	HidKeynoteActionMediaVolDown,
	HidKeynoteActionCtrlC,
	HidKeynoteActionCtrlV,
	HidKeynoteActionCtrlX,
	HidKeynoteActionCtrlA,
	HidKeynoteActionCtrlZ,
	HidKeynoteActionCtrlY,
	HidKeynoteActionCtrlS,
	HidKeynoteActionCtrlF,
	HidKeynoteActionCtrlW,
	HidKeynoteActionCtrlT,
	HidKeynoteActionCtrlShiftT,
	HidKeynoteActionAltF4,
	HidKeynoteActionAltTab,
	HidKeynoteActionCtrlAltDel,
	HidKeynoteActionCount,
} HidKeynoteAction;

typedef enum {
	HidKeynoteModeDefault,
	HidKeynoteModeConfigured,
} HidKeynoteMode;

HidKeynote* hid_keynote_alloc(Hid* bt_hid);

void hid_keynote_free(HidKeynote* hid_keynote);

View* hid_keynote_get_view(HidKeynote* hid_keynote);

void hid_keynote_set_connected_status(HidKeynote* hid_keynote, bool connected);

void hid_keynote_set_orientation(HidKeynote* hid_keynote, bool vertical);

const char* hid_keynote_action_get_name(HidKeynoteAction action);
const char* hid_keynote_action_get_short_name(HidKeynoteAction action);
HidKeynoteAction hid_keynote_get_default_action(HidKeynoteButton button);
HidKeynoteAction hid_keynote_get_action(HidKeynote* hid_keynote, HidKeynoteButton button);
void hid_keynote_set_action(
	HidKeynote* hid_keynote,
	HidKeynoteButton button,
	HidKeynoteAction action);
void hid_keynote_reset_actions(HidKeynote* hid_keynote);
void hid_keynote_sync_actions(HidKeynote* hid_keynote);
void hid_keynote_set_mode(HidKeynote* hid_keynote, HidKeynoteMode mode);

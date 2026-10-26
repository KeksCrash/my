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
	// tmux prefix-sequence combos (^B,key)
	HidKeynoteActionTmuxNewWin,
	HidKeynoteActionTmuxNextWin,
	HidKeynoteActionTmuxPrevWin,
	HidKeynoteActionTmuxSplitH,
	HidKeynoteActionTmuxSplitV,
	HidKeynoteActionTmuxPaneUp,
	HidKeynoteActionTmuxPaneDn,
	HidKeynoteActionTmuxPaneLt,
	HidKeynoteActionTmuxPaneRt,
	HidKeynoteActionTmuxDetach,
	HidKeynoteActionTmuxZoom,
	HidKeynoteActionTmuxCopyMode,
	HidKeynoteActionTmuxLastWin,
	HidKeynoteActionTmuxRename,
	// Symbol keys
	HidKeynoteActionMinus,
	HidKeynoteActionEquals,
	HidKeynoteActionOpenBracket,
	HidKeynoteActionCloseBracket,
	HidKeynoteActionBackslash,
	HidKeynoteActionSemicolon,
	HidKeynoteActionApostrophe,
	HidKeynoteActionGraveAccent,
	HidKeynoteActionComma,
	HidKeynoteActionDot,
	HidKeynoteActionSlash,
	// Function keys
	HidKeynoteActionF1,
	HidKeynoteActionF2,
	HidKeynoteActionF3,
	HidKeynoteActionF4,
	HidKeynoteActionF5,
	HidKeynoteActionF6,
	HidKeynoteActionF7,
	HidKeynoteActionF8,
	HidKeynoteActionF9,
	HidKeynoteActionF10,
	HidKeynoteActionF11,
	HidKeynoteActionF12,
	// Navigation keys
	HidKeynoteActionHome,
	HidKeynoteActionEnd,
	HidKeynoteActionPageUp,
	HidKeynoteActionPageDown,
	HidKeynoteActionInsert,
	// Extra keys (not on touch keyboard)
	HidKeynoteActionCapsLock,
	HidKeynoteActionPrintScreen,
	HidKeynoteActionScrollLock,
	HidKeynoteActionPause,
	HidKeynoteActionContextMenu,
	// User-defined macro (arbitrary key combos + typed text, see hid_keynote_play_macro())
	HidKeynoteActionCustomMacro,
	// BadKB Duckyscript from /ext/badkb/*.txt, executed via hid_ble2's active BLE profile
	HidKeynoteActionBadKbScript,
	HidKeynoteActionCount,
} HidKeynoteAction;

// Max length of one button's custom macro script (see hid_keynote_play_macro() for syntax).
#define KEYNOTE_MACRO_MAX_LEN 128

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

// Parses and immediately plays back a macro script (see hid_keynote.c for exact syntax:
// ";"-separated steps, each "MOD[+MOD...]+KEYNAME", "STRING <text>" or "DELAY <ms>").
// Blocking (like the existing HidKeynoteActionTypeMacroSeq) - returns once fully played.
void hid_keynote_play_macro(Hid* hid, const char* macro);

// Validates a macro script without playing it back - true if every step parses, used by
// the macro editor scene before saving so a typo doesn't silently become a no-op button.
bool hid_keynote_macro_is_valid(const char* macro);

// True if `action` is one of the modifier actions (Ctrl/Alt/Shift/GUI), with its HID
// modifier bit written to *out_mod_bit - used by the macro builder (see
// scenes/hid_scene_keynote_key_picker.c) to accumulate modifiers picked from the keyboard
// grid into the next non-modifier key picked, instead of treating each pick as its own step.
bool hid_keynote_action_is_modifier(HidKeynoteAction action, uint16_t* out_mod_bit);

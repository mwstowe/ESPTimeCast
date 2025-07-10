#ifndef SAVE_SETTINGS_STATE_H
#define SAVE_SETTINGS_STATE_H

// Enum to track the state of the settings save operation
enum SaveSettingsState {
  IDLE,
  MOUNT_FS,
  READ_CONFIG,
  UPDATE_CONFIG,
  WRITE_CONFIG,
  FINALIZE
};

// Global variables for state management
extern SaveSettingsState saveSettingsState;
extern bool settingsSavePending;

#endif // SAVE_SETTINGS_STATE_H

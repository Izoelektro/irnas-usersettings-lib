/** @file user_settings_types.h
 *
 * @brief Module for handling all IoT user settings
 *
 * @par
 * COPYRIGHT NOTICE: (c) 2022 Irnas.  All rights reserved.
 */

#ifndef USER_SETTINGS_TYPES_H
#define USER_SETTINGS_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/kernel.h>

/**
 * @brief Callback type to notify the application of a changed setting
 *
 * The callback is called from the same thread that updated the setting.
 *
 * The consumer can then get the value of the setting via user_settings_get_with_*()
 * and respond accordingly.
 *
 * @param[in] id The ID of the setting that was changed
 * @param[in] key The key of the setting that was changed
 */
typedef void (*user_settings_on_change_t)(uint32_t id, const char *key);

/**
 * @brief Type of user setting
 *
 * USER_SETTINGS_TYPE_STR should only be used for valid null terminated C strings. For arbitrary
 * byte arrays, use USER_SETTINGS_TYPE_BYTES.
 *
 */
enum user_setting_type {
	USER_SETTINGS_TYPE_BOOL = 0,

	USER_SETTINGS_TYPE_U8,
	USER_SETTINGS_TYPE_U16,
	USER_SETTINGS_TYPE_U32,
	USER_SETTINGS_TYPE_U64,

	USER_SETTINGS_TYPE_I8,
	USER_SETTINGS_TYPE_I16,
	USER_SETTINGS_TYPE_I32,
	USER_SETTINGS_TYPE_I64,

	USER_SETTINGS_TYPE_STR,

	USER_SETTINGS_TYPE_BYTES,

	USER_SETTINGS_TYPE_CRON_JOB
};

#ifdef __cplusplus
}
#endif

#endif /* USER_SETTINGS_TYPES_H */

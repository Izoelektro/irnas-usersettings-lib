/** @file user_settings.c
 *
 * @brief User settings
 *
 * @par
 * COPYRIGHT NOTICE: (c) 2022 Irnas. All rights reserved.
 */

#include <user_settings.h>

#include "user_settings_list.h"
#include <user_settings_types.h>

#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(user_settings, CONFIG_USER_SETTINGS_LOG_LEVEL);

#define INIT_ASSERT_TEXT "user_settings_init should be called before this function"
#define LOAD_ASSERT_TEXT "user_settings_load should be called before this function"

#define USER_SETTINGS_PREFIX	     "user"
#define USER_SETTINGS_DEFAULT_PREFIX "user_default"

/* External callback */
static user_settings_on_change_t prv_global_on_change_cb;

/* state of module */
static bool prv_is_inited;
static bool prv_is_loaded;

/* ------------- default settings values handlers -------------  */

/**
 * @brief This is called when we call settings_runtime_set on the default prefix
 */
static int prv_default_set_cb(const char *key, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	int rc;

	/* Check if key exists in the settings list */
	struct user_setting *setting = user_settings_list_get_by_key(key);
	if (!setting) {
		return -ENOENT;
	}

	/* Check if setting fits into the allocated space */
	if (len > setting->max_size) {
		return -EINVAL;
	}

	/* Read the settings from NVS */
	rc = read_cb(cb_arg, setting->default_data, setting->max_size);
	if (rc < 0) {
		LOG_ERR("read_cb, err: %d", rc);
		return rc;
	} else if (rc == 0) {
		LOG_ERR("read_cb, this key value pair was deleted");
		/* TODO: must we do something here? When does this even happen */
		return 0;
	}

	/* Remember actual data length */
	setting->default_data_len = rc;
	setting->default_is_set = true;

	return 0;
}

/**
 * @brief This is called when we call settings_runtime_set on the value prefix
 */
static int prv_value_set_cb(const char *key, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	int rc;

	/* Check if key exists in the settings list */
	struct user_setting *setting = user_settings_list_get_by_key(key);
	if (!setting) {
		return -ENOENT;
	}

	/* Check if setting fits into the allocated space */
	if (len > setting->max_size) {
		return -EINVAL;
	}

	/* Read the settings from NVS */
	rc = read_cb(cb_arg, setting->data, setting->max_size);
	if (rc < 0) {
		LOG_ERR("read_cb, err: %d", rc);
		return rc;
	} else if (rc == 0) {
		LOG_ERR("read_cb, this key value pair was deleted");
		/* TODO: must we do something here? When does this even happen */
		return 0;
	}

	/* Remember actual data length */
	setting->data_len = rc;
	setting->is_set = true;

	LOG_DBG("Setting %s was read", setting->key);

	/* Notify on change */
	if (prv_global_on_change_cb) {
		prv_global_on_change_cb(setting->id, setting->key);
	}

	if (setting->on_change_cb) {
		setting->on_change_cb(setting->id, setting->key);
	}

	return 0;
}

int user_settings_init(void)
{
	static struct settings_handler prv_default_sh = {
		.name = USER_SETTINGS_DEFAULT_PREFIX,
		.h_set = prv_default_set_cb,
	};

	static struct settings_handler prv_value_sh = {
		.name = USER_SETTINGS_PREFIX,
		.h_set = prv_value_set_cb,
	};

	__ASSERT(!prv_is_inited, "user_settings_init should only be called once");

	int err;

	user_settings_list_init();

	/* can be safely called multiple times from different modules */
	err = settings_subsys_init();
	if (err) {
		LOG_ERR("settings_subsys_init, err: %d", err);
		return -EIO;
	}

	/* register handler for default values */
	err = settings_register(&prv_default_sh);
	if (err) {
		LOG_ERR("settings_register, err: %d", err);
		return -EIO;
	}

	/* register handler for set values */
	err = settings_register(&prv_value_sh);
	if (err) {
		LOG_ERR("settings_register, err: %d", err);
		return -EIO;
	}

	prv_is_inited = true;

	return 0;
}

void user_settings_add(uint16_t id, const char *key, enum user_setting_type type)
{
	__ASSERT(prv_is_inited, INIT_ASSERT_TEXT);
	__ASSERT(type != USER_SETTINGS_TYPE_STR, "Use user_settings_add_sized for string type!");
	__ASSERT(type != USER_SETTINGS_TYPE_BYTES, "Use user_settings_add_sized for bytes type!");

	user_settings_list_add_fixed_size(id, key, type);
}

void user_settings_add_sized(uint16_t id, const char *key, enum user_setting_type type, size_t size)
{
	__ASSERT(prv_is_inited, INIT_ASSERT_TEXT);
	__ASSERT(type == USER_SETTINGS_TYPE_STR || type == USER_SETTINGS_TYPE_BYTES,
		 "This function only supports string and bytes types");

	user_settings_list_add_variable_size(id, key, type, size);
}

int user_settings_load(void)
{
	__ASSERT(prv_is_inited, INIT_ASSERT_TEXT);

	prv_is_loaded = true;

	int err;

	/* load all default values */
	err = settings_load_subtree(USER_SETTINGS_DEFAULT_PREFIX);
	if (err) {
		LOG_ERR("Failed loading user_settings_default subtree, err: %d", err);
		return -EIO;
	}

	/* load actual setting values */
	err = settings_load_subtree(USER_SETTINGS_PREFIX);
	if (err) {
		LOG_ERR("Failed loading user_settings values subtree, err: %d", err);
		return -EIO;
	}

	return 0;
}

static int prv_user_settings_set_default(struct user_setting *s, void *data, size_t len)
{
	__ASSERT(prv_is_loaded, LOAD_ASSERT_TEXT);

	int err;

	/* Check if new value is the same as existing value */
	if (len == s->default_data_len && memcmp(data, s->default_data, s->default_data_len) == 0) {
		LOG_DBG("Same default value as existing value.");
		return 0;
	}

	/* check if default already set */
	if (s->default_is_set) {
		LOG_ERR("Default already set for setting %s. Not setting new default. Clear NVS "
			"first if you wish to change the default.",
			s->key);
		return -EALREADY;
	}

	/* check space */
	if (len > s->max_size) {
		LOG_ERR("Default value size to large. Max size is %d", s->max_size);
		return -ENOMEM;
	}

	/* Use settings_runtime_set() so that prv_default_set_cb gets called, which will
	 * set the setting default value in the settings list.
	 * It will also set s->default_is_set
	 */
	char key_with_prefix[SETTINGS_MAX_NAME_LEN + 1] = {0};
	sprintf(key_with_prefix, USER_SETTINGS_DEFAULT_PREFIX "/%s", s->key);
	err = settings_runtime_set(key_with_prefix, data, len);
	if (err) {
		LOG_ERR("settings_runtime_set, err: %d", err);
		return -EIO;
	}

	/* Use settings_save_one() so that the default setting value is stored to NVS */
	err = settings_save_one(key_with_prefix, data, len);
	if (err) {
		LOG_ERR("settings_save, err: %d", err);
		return -EIO;
	}

	return 0;
}

int user_settings_set_default_with_key(char *key, void *data, size_t len)
{
	__ASSERT(prv_is_loaded, LOAD_ASSERT_TEXT);

	struct user_setting *s = user_settings_list_get_by_key(key);
	__ASSERT(s, "Key does not exists: %s", key);

	return prv_user_settings_set_default(s, data, len);
}

int user_settings_set_default_with_id(uint16_t id, void *data, size_t len)
{
	__ASSERT(prv_is_loaded, LOAD_ASSERT_TEXT);

	struct user_setting *s = user_settings_list_get_by_id(id);
	__ASSERT(s, "ID does not exists: %d", id);

	return prv_user_settings_set_default(s, data, len);
}

static int prv_user_settings_set(struct user_setting *s, void *data, size_t len)
{
	__ASSERT(prv_is_loaded, LOAD_ASSERT_TEXT);

	int err;

	/* check space */
	if (len > s->max_size) {
		LOG_ERR("Value size to large. Max size is %d", s->max_size);
		return -ENOMEM;
	}

	/* Check if value is the same */
	if (memcmp(s->data, data, len) == 0 && len == s->data_len) {
		LOG_DBG("Setting to same value.");
		return 0;
	}

	/* Use settings_runtime_set() so that prv_value_set_cb gets called, which will
	 * set the setting value in the settings list */
	char key_with_prefix[SETTINGS_MAX_NAME_LEN + 1] = {0};
	sprintf(key_with_prefix, USER_SETTINGS_PREFIX "/%s", s->key);
	err = settings_runtime_set(key_with_prefix, data, len);
	if (err) {
		LOG_ERR("settings_runtime_set, err: %d", err);
		return -EIO;
	}

	/* Use settings_save_one() so that the setting is stored to NVS */
	err = settings_save_one(key_with_prefix, data, len);
	if (err) {
		LOG_ERR("settings_save, err: %d", err);
		return -EIO;
	}

	return 0;
}

void user_settings_restore_defaults(void)
{
	__ASSERT(prv_is_loaded, LOAD_ASSERT_TEXT);

	/* by using prv_user_settings_set on each setting, the values will get stored and the
	 * on_change callbacks will be called correctly
	 */

	user_settings_list_iter_start();
	struct user_setting *setting;
	while ((setting = user_settings_list_iter_next()) != NULL) {
		if (setting->default_is_set) {
			prv_user_settings_set(setting, setting->default_data,
					      setting->default_data_len);
		}
	}
}

bool user_settings_exists_with_key(char *key)
{
	__ASSERT(prv_is_loaded, LOAD_ASSERT_TEXT);

	struct user_setting *s = user_settings_list_get_by_key(key);
	return s != NULL;
}

bool user_settings_exists_with_id(uint16_t id)
{
	__ASSERT(prv_is_loaded, LOAD_ASSERT_TEXT);

	struct user_setting *s = user_settings_list_get_by_id(id);
	return s != NULL;
}

int user_settings_set_with_key(char *key, void *data, size_t len)
{
	__ASSERT(prv_is_loaded, LOAD_ASSERT_TEXT);

	struct user_setting *s = user_settings_list_get_by_key(key);
	__ASSERT(s, "Key does not exists: %s", key);

	return prv_user_settings_set(s, data, len);
}

int user_settings_set_with_id(uint16_t id, void *data, size_t len)
{
	__ASSERT(prv_is_loaded, LOAD_ASSERT_TEXT);

	struct user_setting *s = user_settings_list_get_by_id(id);
	__ASSERT(s, "ID does not exists: %d", id);

	return prv_user_settings_set(s, data, len);
}

static void *prv_user_setting_get(struct user_setting *s, size_t *len)
{
	if (s->is_set) {
		if (len) {
			*len = s->data_len;
		}
		return s->data;
	}

	if (s->default_is_set) {
		if (len) {
			*len = s->default_data_len;
		}
		return s->default_data;
	}

	return NULL;
}

void *user_settings_get_with_key(char *key, size_t *len)
{
	__ASSERT(prv_is_loaded, LOAD_ASSERT_TEXT);

	struct user_setting *s = user_settings_list_get_by_key(key);
	__ASSERT(s, "Key does not exists: %s", key);

	return prv_user_setting_get(s, len);
}

void *user_settings_get_with_id(uint16_t id, size_t *len)
{
	__ASSERT(prv_is_loaded, LOAD_ASSERT_TEXT);

	struct user_setting *s = user_settings_list_get_by_id(id);
	__ASSERT(s, "ID does not exists: %d", id);

	return prv_user_setting_get(s, len);
}

void user_settings_set_global_on_change_cb(user_settings_on_change_t on_change_cb)
{
	__ASSERT(prv_is_inited, INIT_ASSERT_TEXT);

	prv_global_on_change_cb = on_change_cb;
}

void user_settings_set_on_change_cb_with_key(const char *key,
					     user_settings_on_change_t on_change_cb)
{
	__ASSERT(prv_is_inited, INIT_ASSERT_TEXT);

	struct user_setting *s = user_settings_list_get_by_key(key);
	__ASSERT(s, "Key does not exists: %s", key);

	s->on_change_cb = on_change_cb;
}

void user_settings_set_on_change_cb_with_id(uint16_t id, user_settings_on_change_t on_change_cb)
{
	__ASSERT(prv_is_inited, INIT_ASSERT_TEXT);

	struct user_setting *s = user_settings_list_get_by_id(id);
	__ASSERT(s, "ID does not exists: %d", id);

	s->on_change_cb = on_change_cb;
}

/* this is only false if no default exists and no value was set */
bool user_settings_is_set_with_key(char *key)
{
	__ASSERT(prv_is_loaded, LOAD_ASSERT_TEXT);

	struct user_setting *s = user_settings_list_get_by_key(key);
	__ASSERT(s, "Key does not exists: %s", key);

	return s->is_set;
}

/* this is only false if no default exists and no value was set */
bool user_settings_is_set_with_id(uint16_t id)
{
	__ASSERT(prv_is_loaded, LOAD_ASSERT_TEXT);

	struct user_setting *s = user_settings_list_get_by_id(id);
	__ASSERT(s, "Id does not exists: %d", id);

	return s->is_set;
}

bool user_settings_has_default_with_key(char *key)
{
	__ASSERT(prv_is_loaded, LOAD_ASSERT_TEXT);

	struct user_setting *s = user_settings_list_get_by_key(key);
	__ASSERT(s, "Key does not exists: %s", key);

	return s->default_is_set;
}

bool user_settings_has_default_with_id(uint16_t id)
{
	__ASSERT(prv_is_loaded, LOAD_ASSERT_TEXT);

	struct user_setting *s = user_settings_list_get_by_id(id);
	__ASSERT(s, "Id does not exists: %d", id);

	return s->default_is_set;
}

uint16_t user_settings_key_to_id(const char *key)
{
	__ASSERT(prv_is_loaded, LOAD_ASSERT_TEXT);

	struct user_setting *s = user_settings_list_get_by_key(key);
	__ASSERT(s, "Key does not exists: %s", key);

	return s->id;
}

const char *user_settings_id_to_key(uint16_t id)
{
	__ASSERT(prv_is_loaded, LOAD_ASSERT_TEXT);

	struct user_setting *s = user_settings_list_get_by_id(id);
	__ASSERT(s, "Id does not exists: %d", id);

	return s->key;
}

size_t user_settings_get_max_len_with_key(const char *key)
{
	__ASSERT(prv_is_loaded, LOAD_ASSERT_TEXT);

	struct user_setting *s = user_settings_list_get_by_key(key);
	__ASSERT(s, "Key does not exists: %s", key);

	return s->max_size;
}

size_t user_settings_get_max_len_with_id(uint16_t id)
{
	__ASSERT(prv_is_loaded, LOAD_ASSERT_TEXT);

	struct user_setting *s = user_settings_list_get_by_id(id);
	__ASSERT(s, "Id does not exists: %d", id);

	return s->max_size;
}

enum user_setting_type user_settings_get_type_with_key(const char *key)
{
	__ASSERT(prv_is_loaded, LOAD_ASSERT_TEXT);

	struct user_setting *s = user_settings_list_get_by_key(key);
	__ASSERT(s, "Key does not exists: %s", key);

	return s->type;
}

enum user_setting_type user_settings_get_type_with_id(uint16_t id)
{
	__ASSERT(prv_is_loaded, LOAD_ASSERT_TEXT);

	struct user_setting *s = user_settings_list_get_by_id(id);
	__ASSERT(s, "Id does not exists: %d", id);

	return s->type;
}

#include <user_settings.h>
#include <user_settings_list.h>

#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>

static void *user_settings_suite_setup(void)
{
	user_settings_init();

	/* add 4 common items */
	user_settings_add(1, "t1", USER_SETTINGS_TYPE_BOOL);
	user_settings_add(2, "t2", USER_SETTINGS_TYPE_U32);
	user_settings_add(3, "t3", USER_SETTINGS_TYPE_I8);
	user_settings_add_sized(4, "t4", USER_SETTINGS_TYPE_STR, 10);

	user_settings_load();

	return NULL;
}

void user_settings_suite_before_each(void *fixture)
{
	/* create default state for each setting */
	user_settings_set_global_on_change_cb(NULL);
	user_settings_set_on_change_cb_with_id(1, NULL);
	user_settings_set_on_change_cb_with_id(2, NULL);
	user_settings_set_on_change_cb_with_id(3, NULL);
	user_settings_set_on_change_cb_with_id(4, NULL);

	bool value1 = 0;
	user_settings_set_with_id(1, &value1, 1);
	uint32_t value2 = 0;
	user_settings_set_with_id(2, &value2, 4);
	int8_t value3 = 0;
	user_settings_set_with_id(3, &value3, 1);
	char value4[] = "";
	user_settings_set_with_id(4, &value4, strlen(value4) + 1);
}

ZTEST_SUITE(user_settings_suite, NULL, user_settings_suite_setup, user_settings_suite_before_each,
	    NULL, NULL);

ZTEST(user_settings_suite, test_settings_exist)
{
	zassert_equal(user_settings_exists_with_id(1), true, "Setting should exist");
	zassert_equal(user_settings_exists_with_id(2), true, "Setting should exist");
	zassert_equal(user_settings_exists_with_id(3), true, "Setting should exist");
	zassert_equal(user_settings_exists_with_id(4), true, "Setting should exist");
}

ZTEST(user_settings_suite, test_nonexistand_settings_dont_exist)
{
	zassert_equal(user_settings_exists_with_id(5), false, "Setting should not exist");
	zassert_equal(user_settings_exists_with_key("t0"), false, "Setting should not exist");
}

ZTEST(user_settings_suite, test_settings_key_id_conversion)
{
	zassert_equal(user_settings_key_to_id("t1"), 1, "key t1 should map to id 1");
	zassert_ok(strcmp(user_settings_id_to_key(2), "t2"), "id 2 should map to key \"t2\"");
}

ZTEST(user_settings_suite, test_settings_set)
{
	int err;

	bool value = true;
	size_t size;
	err = user_settings_set_with_id(1, &value, 1);
	zassert_ok(err, "set should not error here");

	bool *out_value = user_settings_get_with_id(1, &size);
	zassert_equal(*out_value, value, "What was set should be what was gotten");
	zassert_equal(size, 1, "size of bool setting should be 1");

	/* you can also get without size out param */
	bool *out_value2 = user_settings_get_with_id(1, NULL);
	zassert_equal(*out_value2, value, "What was set should be what was gotten");

	/* test set and get of string settings */
	char new_str[] = "banana";
	err = user_settings_set_with_id(4, &new_str, strlen(new_str) + 1);
	zassert_ok(err, "set should not error here");

	char *out_str = user_settings_get_with_id(4, &size);
	zassert_ok(strcmp(new_str, out_str), 0, "What was set should be what was gotten");
	zassert_equal(size, strlen(new_str) + 1, "size of str setting should be %d",
		      strlen(new_str) + 1);
}

ZTEST(user_settings_suite, test_settings_set_value_too_large)
{
	int err;

	/* fist set a good value */
	char new_str[] = "banana";
	err = user_settings_set_with_id(4, &new_str, strlen(new_str) + 1);

	/* try set a value that is to long */
	char new_str2[] = "bananarama";
	err = user_settings_set_with_id(4, &new_str2, strlen(new_str2) + 1);
	zassert_equal(err, -ENOMEM, "set should error here since new string is too long");

	/* setting should be old */
	char *out_str = user_settings_get_with_id(4, NULL);
	zassert_ok(strcmp(new_str, out_str), 0, "Setting not at value that was expected");
}

ZTEST(user_settings_suite, test_settings_default_value)
{
	/* set value */
	uint32_t value = 11;
	user_settings_set_with_id(2, &value, 4);

	/* set some default value */
	uint32_t default_value = 69;
	user_settings_set_default_with_id(2, &default_value, 4);

	/* Get default should return what was set above */
	size_t len_out;
	uint32_t val_get = *(uint32_t *)user_settings_get_default_with_id(2, &len_out);
	zassert_equal(len_out, sizeof(default_value), "default value should be %d",
		      sizeof(default_value));
	zassert_equal(val_get, default_value, "default value should be %d", default_value);

	/* value should be unchanged */
	uint32_t value_out;
	value_out = *(uint32_t *)user_settings_get_with_id(2, NULL);
	zassert_equal(value_out, value, "Value should be unchanged");
}

static uint32_t on_change_id_store;
static const char *on_change_key_store;
void on_change(uint32_t id, const char *key)
{
	on_change_id_store = id;
	on_change_key_store = key;
}

ZTEST(user_settings_suite, test_settings_on_change)
{
	user_settings_set_on_change_cb_with_id(2, on_change);

	uint32_t value = 1337;
	user_settings_set_with_id(2, &value, 4);

	zassert_equal(on_change_id_store, 2, "On change callback should have been called");
	zassert_ok(strcmp(on_change_key_store, "t2"), "On change callback should have been called");
}

ZTEST(user_settings_suite, test_settings_global_on_change)
{
	user_settings_set_global_on_change_cb(on_change);

	uint32_t value = 1337;
	user_settings_set_with_id(2, &value, 4);

	zassert_equal(on_change_id_store, 2, "global on change callback should have been called");
	zassert_ok(strcmp(on_change_key_store, "t2"),
		   "global on change callback should have been called");

	int8_t value2 = -1;
	user_settings_set_with_id(3, &value2, 1);
	zassert_equal(on_change_id_store, 3, "global on change callback should have been called");
	zassert_ok(strcmp(on_change_key_store, "t3"),
		   "global on change callback should have been called");
}

ZTEST(user_settings_suite, test_settings_get_max_len)
{
	/* Each user setting should return the correct max length */
	size_t len;

	len = user_settings_get_max_len_with_id(1);
	zassert_equal(len, 1, "Max length should be 1 for bool type");
	len = user_settings_get_max_len_with_id(2);
	zassert_equal(len, 4, "Max length should be 4 for u32 type");
	len = user_settings_get_max_len_with_id(3);
	zassert_equal(len, 1, "Max length should be 1 for i8 type");
	len = user_settings_get_max_len_with_id(4);
	zassert_equal(len, 10, "Max length should be 10 for string type");
}

ZTEST(user_settings_suite, test_settings_get_type)
{
	/* Each user setting should return the correct max length */
	enum user_setting_type type;

	type = user_settings_get_type_with_id(1);
	zassert_equal(type, USER_SETTINGS_TYPE_BOOL,
		      "Get type does not match the configured type for t1");
	type = user_settings_get_type_with_id(2);
	zassert_equal(type, USER_SETTINGS_TYPE_U32,
		      "Get type does not match the configured type for t2");
	type = user_settings_get_type_with_id(3);
	zassert_equal(type, USER_SETTINGS_TYPE_I8,
		      "Get type does not match the configured type for t3");
	type = user_settings_get_type_with_id(4);
	zassert_equal(type, USER_SETTINGS_TYPE_STR,
		      "Get type does not match the configured type for t4");
}

/*
 * NOT TESTED:
 *
 * - assertions when getting/setting nonexistant settings
 */
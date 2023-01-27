# User Settings

The user_settings lib is written to be agnostic to the list of settings used by an application.
All settings have a numeric ID, string key, type, value and default value (of the selected type).
The supported types can be seen in [user_settings_types.h](./user-settings/include/user_settings_types.h). Please see the [basic sample](./samples/basic/) for usage.

## Setup

1. To your `west.yml` add the irnas remote to the `remotes` section:

   ```yaml
    - name: irnas
      url-base: https://github.com/irnas
   ```

2. Then in the `projects` section add at the bottom:

    ```yaml
    - name: irnas-usersettings-lib
      repo-path: irnas-usersettings-lib
      path: irnas/irnas-usersettings-lib
      remote: irnas
      revision: v1.0.0

## Usage

To enable, please set `CONFIG_USER_SETTINGS=y` and fulfil the prerequisites.
If `CONFIG_USER_SETTINGS_SHELL` is enabled, settings can also be listed, got and set via the shell.
Type `usettings --help` to see the available commands.

User settings must first be initialized and then ALL the settings must be provided by calls to `user_settings_add()` or `user_settings_add_sized()`. Calls to those two functions will allocate into a private heap. If a setting can not be allocated, the library will assert. You are then expected to increase `CONFIG_USER_SETTINGS_HEAP_SIZE` and try again.
After adding all settings, on_change callbacks can be registered.
Then the settings should be loaded by calling `user_settings_load()`.

```c
user_settings_init();

user_settings_add(1, "t1", USER_SETTINGS_TYPE_BOOL);
user_settings_add(2, "t2", USER_SETTINGS_TYPE_U8);
// ... (add more settings)

user_settings_load();
```

When calling `user_settings_load()`, each setting will be loaded with its value from NVS. If no value was (ever) set, then a default value will be loaded.

The default value can be set by calling `user_settings_set_default_with_*()`. Keep in mind that a default value can only be set once for each setting. To set a new default, NVS must be cleared first.

Settings can then be get and set from within the application code - for that, the settings key/id and type are expected to be known by the caller.

---

During development, you probably want to hardcode all your setting
values by calling `user_settings_set_with_*` for each setting during initialization and thus have all settings with a valid value.

In a production device, settings should be provisioned into flash using `user_settings_set_default_with_*`.
When the application later boots, the defaults will be loaded, and your can assume that all settings have a valid value.

If no default and no value exist for a setting, getting its value will return a pointer to zeroed memory. In such cases, it is prudent
to check if a setting has a valid value by using `user_settings_is_set_with_*()`.

To check whether a setting has a default value, `user_settings_has_default_with_*()` can be used.
For this reason, it is suggested that each setting should have a value set (via its default value or directly) during application initialization, so that later code can assume all settings have valid values and can be read directly.

## Bluetooth Service

A user setting bluetooth service can be enabled by setting `CONFIG_USER_SETTINGS_BT_SERVICE=y`. See the [bluetooth service sample](./samples/bluetooth_service) for details.
The service uses the user settings binary protocol under the hood. For the protocol definition, see [here](./libraray/protocol/binary/README.md)

## Development Setup

If not already set up, install west and other required tools.
Follow the steps [here](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/gs_installing.html)
from [Install the required tools](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/gs_installing.html#install-the-required-tools)
up to (including) [Install west](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/gs_installing.html#install-the-required-tools).

Then follow these steps:

```bash
west init -m https://github.com/IRNAS/irnas-usersettings-lib irnas-usersettings-lib
cd irnas-usersettings-lib
west update
# remember to source zephyr env
source zephyr/zephyr-env.sh
```

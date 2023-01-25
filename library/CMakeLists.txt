
zephyr_include_directories(include)

zephyr_library()
add_subdirectory_ifdef(CONFIG_USER_SETTINGS user_settings)

# Binary Protocol

The binary protocol is a byte-based encoding of the user settings protocol.
All numbers are encoded as little endian.

The binary protocol supports the following commands:

- GET - get a short setting description
- GET_FULL - get a full setting description
- LIST - get a short setting description for each settings
- LIST_FULL - get a full setting description for each settings
- SET - set a setting value
- SET_DEFAULT - set a setting default value
- RESTORE - set all settings to their default values

## GET (0x01)

A valid get command is encoded as [1 byte command (0x01), 2 byte setting ID].
For example, to get the settings with id 13, the command is `010D00`.

A response to a valid get command is:
[2 byte setting id, setting key as a null terminated string, 1 byte [setting type](../../include/user_settings_types.h), 1 byte value length (LEN), LEN bytes value].

If the value of the settings is not set, LEN will be zero and no value is encoded.

Some Examples:

- a u8 setting with ID 7 , key `s7` and value 7 is encoded as: `0700733700010107`
- a u8 setting with ID 7 , key `s7` and no value is encoded as: `07007337000100`

## GET FULL (0x02)

A valid get full command is encoded as [1 byte command (0x02), 2 byte setting ID].
For example, to get the settings with id 13, the command is `020D00`.

A response to a valid get command is the same as for GET with the following additional fields: [..., 1 byte default length (DEFAULT_LEN), DEFAULT_LEN bytes default value, 1 byte maximum setting length].

If the default value of the settings is not set, DEFAULT_LEN will be zero and no default value is encoded.

Some Examples:

- a u8 setting with ID 7 , key `s7`, value 7, default value 15 and max length 1 is encoded as: `0700733700010107010F01`
- a u8 setting with ID 7 , key `s7`, value 7, no default value and max length 1 is encoded as: `07007337000101070001`
- a u8 setting with ID 7 , key `s7`, no value, no default value and max length 1 is encoded as: `070073370001000001`

## LIST (0x03)

A valid list command is encoded as `03`.

Each setting is encoded separately as specified in the GET command.

## LIST FULL (0x04)

A valid list command is encoded as `04`.

Each setting is encoded separately as specified in the GET FULL command.

## SET (0x05)

A valid set command is encoded as [1 byte command (0x05), 2 byte setting ID, 1 byte value length, LEN bytes value].
For example, to set setting with id 13 to the value 7 (the setting is type u8), the command is `050D000107`.

## SET DEFAULT (0x06)

A valid set default command is encoded as [1 byte command (0x06), 2 byte setting ID, 1 byte default value length, LEN bytes default value].
For example, to set the default value of setting with id 13 to 10 (the setting is type u8), the command is `060D00010A`.

## RESTORE (0x07)

A valid restore command is encoded as `07`.

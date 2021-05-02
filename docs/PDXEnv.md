# PDXEnv Library Documentation

This library is designed to spoof/emulate various elements of an operating environment.

Think of it as a sort of, catch-all for various environment options/quirks.

The library reads from an ini file referenced by the environment variable "PDXENV". If unset, the library will default to looking for a "pdxenv.ini" file within the current working directory (cwd).

Options available depend upon OS:

## Linux

- A "username" option in the "global" section that allows the library to spoof the executing username.
- A 'uid_uid#' section that refers to a particular user generally returned by get_uid_data() that allows configuration of the user's password, gid, uid, home directory, default shell, etc.
- An 'osinfo' section that allows spoofing of uname details such as distribution, architecture, etc.

Example:
```
[global]
username=user

[uid_1000]
pw_name=user_1000
pw_passwd=nono
pw_gid=0
pw_uid=0
pw_gecos=""
pw_dir=/home/user
pw_shell=/bin/bash

[osinfo]
sysname = "Linux"
nodename = "system"
release = "2.6.32-5-686"
version = "#1 SMP Mon Jan 16 16:04:25 UTC 2012"
machine = "i686"
domainname = "GNU/Linux"

```

## Windows

- A "username" option in the "global" section that allows the library to spoof the executing username.
- "drive_map_[letter]" sections that will spoof calls to determine drive type (FIXED/CDROM), filesystem type, volume label, device serials, etc.

Example:
```
[global]
username=user

[drive_map_c]
type=3
serial=8675309
label=APP
filesystem=NTFS

[drive_map_d]
type=5
serial=1234567
label=MYCOOLCDAPP
filesystem=CDFS
```

Refer to the pdxenv.ini file in "examples" for more information.
# PDXFS Library Documentation

This library is designed to perform filesystem redirection with varying levels of read/write isolation and fallback.

Some of the notable features:

- Home directory sanitization. That is, any requests to a user path will be redirected to a generic path for the purpose of portability.
Example:
```
/home/me => PDXROOT/home/user
C:\Users\me => PDXROOT\c\Users\user
```

- Alternate Data Streams are supported for both directories and files regardless of filesystem. ADS are now denoted as the following:
```
C:\path\to\file:myad => C:\path\to\file_myad.ADS
```


There are a few environment variables:
- PDXFS_IGNORE: A semicolon-delimited list of any directory paths you wish to ignore and bypass from redirection.
- PDXFS_ROOT: An absolute path to an isolated root. This will form the basis for every path redirection.
- PDXFS_MODE: An operation mode switch (default mode 1) for the level of isolation.
  - Mode 1: Reads will attempt to read from the redirected root first and fallback to the real path if it does not exist. All writes will be written to the redirected root.
  - Mode 2: Reads will attempt to read from the redirected root first and fallback to the real path if it does not exist. Writes will write outside of the redirected root if they exist.
  - Mode 3: Reads will only read from the redirected root. Writes will write outside of the redirected root if they exist.
  - Mode 4: Reads will only read from the redirected root. All writes will be written to the redirected root.




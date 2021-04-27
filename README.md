# Process Isolation and Environment Emulation Libraries (PIEE)

This collection of loadable modules comprise a framework for isolating a process and/or emulating another operating environment.

Aspects are divided as follows:

- piee_env: Handles username, home directory, os information, etc.

- piee_reg: Handles Windows registry isolation and redirection.

- piee_fs: Handles Filesystem isolation and redirection.

- piee_proc: Handles process spawning and legacy isolation.

Ideally, these modules would be loaded in at process start either via loader, payload delivery (like a bootstrap shim), or dynamically linked.

In addition, I have included an example bootstrap library (Windows) that looks at VXBOOT32 or VXBOOT64 for a semicolon-delimited list of libraries to load on process start.

Notes about piee_fs:
- Will bypass any path that includes paths in the semicolon-delimited envar PIEE_FS_IGNORE
- Will rebase every path to the root directory specified in evar PIEE_FS_ROOT
- Will 'sanitize' any user paths from a home directory to a generic 'user' path for persistence compatibility.
- Operates with various isolation modes defined by PIEE_FS_MODE:
    -> Mode 1 (Default): Read operations are allowed to fallback outside of isolation if they don't exist within, but all writes are redirected.
    -> Mode 2: Both read and write operations are allowed to fallback outside of isolation if they don't exist within.
    -> Mode 3: All read operations will be redirected. This includes drivers and system components. Write operations will fallback outside of isolation if they don't exist within.
    -> Mode 4: Full isolation - all reads and writes are redirected.
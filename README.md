# Process Isolation and Environment Emulation Libraries (PIEE)

This collection of loadable modules comprise a framework for isolating a process and/or emulating another operating environment.

Aspects are divided as follows:

- piee_env: Handles username, home directory, os information, etc.

- piee_reg: Handles Windows registry isolation and redirection.

- piee_fs: Handles Filesystem isolation and redirection.

- piee_proc: Handles process spawning and legacy isolation.

Ideally, these modules would be loaded in at process start either via loader, payload delivery (like a bootstrap shim), or dynamically linked.

In addition, I have included an example bootstrap library (Windows) that looks at VXBOOT32 or VXBOOT64 for a semicolon-delimited list of libraries to load on process start.
# PDXProc Library Documentation

This library is designed to ensure that a target process and all future processes in the lineage are injected with the same set of desired libraries.

The mechanism for this varies by platform:

Linux
-----
This library is given as a preload option on parent execution. It does this by hooking the execve syscall
to construct a modified envp[] and pass it to the new process. At that time, it will revert any envp changes.

There are a few environment variables that it references:
- PDXPL: This is a colon-delimited list of absolute library paths to inject into a process and all children of that process, with the first entry being the absolute path to pdxproc.

- PDXOPL: This is a colon-delimited list of the original libraries that the process was preloaded with. Anything in this envar will be appended to our desired libraries (PDXPL) and set as the advertised "LD_PRELOAD" at the end of the load process.

- LD_PRELOAD: Use this as the entry-point for injection of this library.

Note: In the event that a process will spawn a child that differs from the parent architecture, multiple 
sets of preloaded libraries can be specified (e.g. PDXPL=/path/to/pdxproc64.so;/path/to/pdxproc32.so)

Example Usage: 
```
PDXPL=/path/to/pdxproc.so;/path/to/library_1.so LD_PRELOAD=/path/to/pdxproc.so /bin/ls
```

Windows
-------
On Windows, this library operates differently due to the fact that processes are created in the parent address space.

On load, this library reads from a semicolon-delimited environment variable and loads all specified library paths into the process address space. It then hooks NtCreateProcess to ensure that any legacies have an altered startup procedure which starts the target in a suspended state where it will then retrieve the architecture type and call a separate, architecture-dependent process to perform APC Injection and subject the legacy to this same process.

This separate process is a dll loader by the name of dropkick. It can be used to arbitrarily inject libraries into a process either at spawn or while running (dependent upon how it's called). Due to the fact that this type of injection requires the parent to be the same architecture as the target, there are both dropkick32.exe and dropkick64.exe binaries. 

Usage follows using x64 as an example:
```
    // To start a new process from the executable's path and inject a given library, type:
        > dropkick64.exe start C:\path\to\pdxproc64.dll C:\path\to\app.exe [ARGS]
    // If you need to start a new process with a specific path:
        > dropkick64.exe start_in C:\path\to\pdxproc64.dll C:\path\to\app.exe [ARGS]
    // If the process is running / suspended, use the following command:	
	    > dropkick64.exe inject C:\path\to\pdxproc64.dll [PID as int] [TID as int] [LEAVE_SUSPENDED = 0 or 1]
```

There are a couple of environment variables used:
- PDXPL: This is a semicolon-delimited list of absolute paths to libraries that should be injected on process load.
  
*Note*: In the event that a process will spawn a child that differs from the parent architecture, multiple 
sets of preloaded libraries can be specified:
```
set PDXPL=C:\path\to\library_1_32.dll;C:\path\to\library_1_64.dll;C:\path\to\library_2_64.dll
```

- PDXPROC: This is a specified directory path to the location of the pdxproc libraries and loaders. A full, architecture-dependent path will be constructed as needed (e.g. C:\path\to\pdxproc\loader\dropkick64.exe)

Example Usage: 
```
set PDXPL=C:\path\to\library_1_64.dll
set PDXPROC=C:\path\to\pdxproc\loader
dropkick64.exe start c:\path\to\pdxproc64.dll C:\path\to\process.exe args
```







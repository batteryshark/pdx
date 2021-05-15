# PDXReg Library Documentation

This library is designed to intercept read/write requests to the Windows registry and redirect those operations to a specified ini file... like the good ol' days!

- The library will also write any changes to the ini file, not only for key/names that have been specified, but established paths as well. 
```
[Example]
A write call to "\registry\machine\software\infogrames interactive\somenewkeyname" to our ini file above will write the appropriate value to our ini file. This allows for a semblance of registry persistence as needed.
```

- Paths used are internal paths that don't specify hives directly.
```
[Example]
HKLM -> \registry\machine
HKCU/HKU -> \registry\user
```

- We also ignore wow6432node translation - all keys are referenced as they would be on their native architecture.

- In addition, any keys that specify a particular SID (such as keys that were created for a specific user) are anonymized for portability by default. 

```
[Example]
HKU\S-1-5-21-1431320325-1297723084-2957387153-1001\Console -> \registry\user\Console
```

## Environment Variables
- PDXREG: A full path to an ini file that specifies keys/data. If unset, the library will default to the current working directory to find a file named "pdxreg.ini". 
- PDXREG_AKP: Defines if keypaths should be anonymized. Keypaths WILL be anonymized if this is left unset. Use this if you need to modify legit SID subkeys of another user while using this library.
## Registry INI Format

The ini file has the following format:
```
[\registry\machine\software\infogrames interactive\dig dug deeper\main directory]
// The type refers to REG_SZ
type= 1 
data="c:\app\"

[\registry\machine\software\infogrames interactive\dig dug deeper\somebin]
// The type refers to REG_BINARY
type= 3
// A REG_BINARY entry is a hexstring of the bytes
data=0011223344556677

[\registry\machine\software\infogrames interactive\dig dug deeper\someint]
// The type refers to REG_DWORD
type= 4
// A REG_DWORD entry is the literal value
data=1337
```

Refer to the pdxreg.ini in the "examples" directory for more information.



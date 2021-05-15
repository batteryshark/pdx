#pragma once
#include "../common/ntmin/ntdll.h"

#define MAXIMUM_ALLOWED_BYPASS 0x8675309E

typedef enum _KEY_SET_INFORMATION_CLASS {
  KeyWriteTimeInformation,
  KeyWow64FlagsInformation,
  KeyControlFlagsInformation,
  KeySetVirtualizationInformation,
  KeySetDebugInformation,
  KeySetHandleTagsInformation,
  KeySetLayerInformation,
  MaxKeySetInfoClass
} KEY_SET_INFORMATION_CLASS;

typedef struct _KEY_VALUE_ENTRY {
  PUNICODE_STRING ValueName;
  ULONG           DataLength;
  ULONG           DataOffset;
  ULONG           Type;
} KEY_VALUE_ENTRY, *PKEY_VALUE_ENTRY;



void handler_nt_close_registry_key(void* hKey);
int handler_nt_flush_key(void* hKey);
int handler_nt_delete_value_key(void* hKey, PUNICODE_STRING ValueName);
int handler_nt_delete_key(void* hKey);
int handler_nt_query_mvkey(void* KeyHandle, PKEY_VALUE_ENTRY ValueEntries, ULONG EntryCount, PVOID ValueBuffer, PULONG BufferLength, PULONG RequiredBufferLength,PNTSTATUS status);
int handler_nt_enumerate_key(void* KeyHandle, ULONG Index, KEY_INFORMATION_CLASS KeyInformationClass, PVOID KeyInformation, ULONG Length, PULONG ResultLength,PNTSTATUS status);
int handler_nt_set_key_info(void* KeyHandle,KEY_SET_INFORMATION_CLASS KeySetInformationClass, PVOID KeySetInformation, ULONG KeySetInformationLength);
int handler_nt_set_value_key(HANDLE KeyHandle, PUNICODE_STRING ValueName, ULONG TitleIndex, ULONG Type, PVOID Data, ULONG DataSize);
int handler_nt_redirect_registry_key(POBJECT_ATTRIBUTES ObjectAttributes, PHANDLE KeyHandle);
int handler_nt_query_key(HANDLE KeyHandle, KEY_INFORMATION_CLASS KeyInformationClass, PVOID KeyInformation, ULONG Length, PULONG ResultLength,PNTSTATUS status);
int handler_nt_query_value_key(HANDLE KeyHandle, PUNICODE_STRING ValueName, KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass, PVOID KeyValueInformation, ULONG Length, PULONG ResultLength, PNTSTATUS status);

// Compatibility Stuff
int is_elevated_process();
void set_is_elevated();
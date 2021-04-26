#pragma once
#define GENERIC_READ_BYPASS 0x8675309E

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


BOOL nt_redirect_registry_key(POBJECT_ATTRIBUTES ObjectAttributes, BOOL is_read, BOOL is_write,PHANDLE KeyHandle);
BOOL nt_set_value_key(HANDLE KeyHandle, PUNICODE_STRING ValueName, ULONG TitleIndex, ULONG Type, PVOID Data, ULONG DataSize);
BOOL nt_query_mvkey(HANDLE KeyHandle, PKEY_VALUE_ENTRY ValueEntries, ULONG EntryCount, PVOID ValueBuffer, PULONG BufferLength, PULONG RequiredBufferLength,PNTSTATUS status);
BOOL nt_enumerate_key(HANDLE KeyHandle, ULONG Index, KEY_INFORMATION_CLASS KeyInformationClass, PVOID KeyInformation, ULONG Length, PULONG ResultLength,PNTSTATUS status);
BOOL nt_delete_value_key(HANDLE KeyHandle, PUNICODE_STRING ValueName);
BOOL nt_query_value_key(HANDLE KeyHandle, PUNICODE_STRING ValueName, KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass, PVOID KeyValueInformation, ULONG Length, PULONG ResultLength, PNTSTATUS status);
BOOL nt_set_key_info(HANDLE KeyHandle,KEY_SET_INFORMATION_CLASS KeySetInformationClass, PVOID KeySetInformation, ULONG KeySetInformationLength);
BOOL nt_delete_key(HANDLE KeyHandle);
BOOL nt_query_key(HANDLE KeyHandle, KEY_INFORMATION_CLASS KeyInformationClass, PVOID KeyInformation, ULONG Length, PULONG ResultLength,PNTSTATUS status);
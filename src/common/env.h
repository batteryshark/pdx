#pragma once
int get_envar(char* key, char* value, unsigned int value_len);
int get_socket_address(char* socket_name, char* socket_path);
int get_logging_enabled(void);
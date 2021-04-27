#pragma once

int HexToBin(const char* s, unsigned char* buff, int length);
void BinToHex(const unsigned char* buff, int length, char* output, int outLength);
void to_lowercase(char* in);
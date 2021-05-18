#include "strutils.h"

unsigned char HexChar(char c) {
	if ('0' <= c && c <= '9') return (unsigned char)(c - '0');
	if ('A' <= c && c <= 'F') return (unsigned char)(c - 'A' + 10);
	if ('a' <= c && c <= 'f') return (unsigned char)(c - 'a' + 10);
	return 0xFF;
}

int HexToBin(const char* s, unsigned char* buff, int length) {
	int result;
	if (!s || !buff || length <= 0) return -1;

	for (result = 0; *s; ++result)
	{
		unsigned char msn = HexChar(*s++);
		if (msn == 0xFF) return -1;
		unsigned char lsn = HexChar(*s++);
		if (lsn == 0xFF) return -1;
		unsigned char bin = (msn << 4) + lsn;

		if (length-- <= 0) return -1;
		*buff++ = bin;
	}
	return result;
}

void BinToHex(const unsigned char* buff, int length, char* output, int outLength) {
	char binHex[] = "0123456789ABCDEF";

	if (!output || outLength < 4) return;
	*output = '\0';

	if (!buff || length <= 0 || outLength <= 2 * length){return;}

	for (; length > 0; --length, outLength -= 2)
	{
		unsigned char byte = *buff++;

		*output++ = binHex[(byte >> 4) & 0x0F];
		*output++ = binHex[byte & 0x0F];
	}
	if (outLength-- <= 0) return;
	*output++ = '\0';
}

#ifndef lower
int lower(int argument){
    if (argument >= 'A' && argument <= 'Z')
        return argument + 'a' - 'A';
    else
        return argument;
}
#endif
void to_lowercase(char* in){
    for (int i = 0; in[i]; i++) {
        in[i] = lower(in[i]);
    }
}
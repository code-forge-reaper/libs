#pragma once
#include <stdio.h>
#include <assert.h>

inline const char* read_file(const char* filename) {
	FILE* file = fopen(filename, "rb");
	fseek(file, 0, SEEK_END);
	long length = ftell(file);
	fseek(file, 0, SEEK_SET);
	char* buffer = new char[length + 1];
	long read = fread(buffer, 1, length, file);
	assert(read == length);
	fclose(file);
	buffer[length] = 0;
	return buffer;
}

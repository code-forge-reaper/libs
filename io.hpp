#pragma once
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

const char *read_file(const char *filename) {
  FILE *file = fopen(filename, "rb");
  fseek(file, 0, SEEK_END);
  long length = ftell(file);
  fseek(file, 0, SEEK_SET);
  char *buffer = (char *)malloc(length + 1);
  long read = fread(buffer, 1, length, file);
  assert(read == length);
  fclose(file);
  buffer[length] = 0;
  return buffer;
}

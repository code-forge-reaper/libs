#pragma once
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
// there might be some memory leaks involving sb_append, but that's a risk we
// can take here

typedef struct {
  char *data;
  size_t len;
  size_t cap;
} String_Builder;
String_Builder *sb_create(size_t initial_size);
void sb_destroy(String_Builder *sb);
void sb_append(String_Builder *sb, const char *str);
void sb_append_many(String_Builder *sb, const char *first, ...);
void sb_appendf(String_Builder *sb, const char *fmt, ...);
void sb_reset(String_Builder *sb);
#define sb_append_many_end(...) sb_append_many(__VA_ARGS__, NULL)
// better than having the end user remember how to get the string themselfs
#define sb_to_string(sb) sb->data

#ifdef CREATE_STRING_BUILDER
String_Builder *sb_create(size_t initial_size) {
  String_Builder *sb = malloc(sizeof(*sb));
  sb->data = malloc(initial_size);
  sb->len = 0;
  sb->cap = initial_size;
  sb->data[0] = '\0';
  return sb;
}

void sb_append_many(String_Builder *sb, const char *first, ...) {
  va_list args;
  sb_append(sb, first);
  va_start(args, first);

  const char *str;
  while ((str = va_arg(args, const char *)) != NULL) {
    sb_append(sb, str);
  }

  va_end(args);
}

void sb_destroy(String_Builder *sb) {
  if (!sb)
    return;
  free(sb->data);
  free(sb);
}

void sb_appendf(String_Builder *sb, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int needed = vsnprintf(NULL, 0, fmt, args);
  va_end(args);

  if (needed <= 0)
    return;

  if (sb->len + needed + 1 > sb->cap) {
    while (sb->len + needed + 1 > sb->cap)
      sb->cap *= 2;
    sb->data = realloc(sb->data, sb->cap);
  }

  va_start(args, fmt);
  //vsprintf(sb->data + sb->len, fmt, args);
  vsnprintf(sb->data + sb->len, sb->cap - sb->len, fmt, args);
  va_end(args);
  sb->len += needed;
}
void sb_append(String_Builder *sb, const char *str) {
  if (!str)
    return;

  size_t add = strlen(str);
  if (sb->len + add + 1 > sb->cap) {
    // double until it fits
    while (sb->len + add + 1 > sb->cap)
      sb->cap *= 2;
    char *new_data = realloc(sb->data, sb->cap);
    if (!new_data) {
      perror("realloc");
      return; // don't assign NULL back
    }
    sb->data = new_data;
  }
  memcpy(sb->data + sb->len, str, add + 1);
  sb->len += add;
  sb->data[sb->len] = '\0';
}

void sb_reset(String_Builder *sb) {
  sb->len = 0;
  sb->data[0] = '\0';
}
#ifdef STRING_BUILDER_TEST
int main() {
  String_Builder *sb = sb_create(32);
  sb_append(sb, "Hello, ");
  sb_append(sb, "world!");
  sb_appendf(sb, " Value = %d", 42);
  printf("Result: %s\n", sb_to_string(sb));
  sb_destroy(sb);
  return 0;
}
#endif

#endif // CREATE_STRING_BUILDER

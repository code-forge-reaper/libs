#define CREATE_STRING_BUILDER
#include <stdio.h>
#include "../string_builder.h"

int main(){
	String_Builder* sb = sb_create(1024);
	int x = 35;
	sb_append(sb, "hello, x is = ");
	sb_appendf(sb, "%i", x);
	printf("%s\n", sb_to_string(sb));
	sb_destroy(sb);
	return 0;
}


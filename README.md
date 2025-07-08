# libs

this is a collection of useful libraries

## dependencies
None, depending on what you use,

> `lua_ffi.hpp`, `lua_math.hpp` and `lua_math.cpp` require lua to be able to be used, as the name suggests

## license
MIT

## using the libraries designed to be used in C

## files and how to use

#### `mime.h`
```c
#define CREATE_MIME_PARSER
#include "../mime.h"
int main(){
	read_mimetypes("mime.types");
	const char* mime = get_mime_from_extension("txt");
	printf("%s\n", mime);
	free_mimetypes();
	return 0;
}
```

#### `string_builder.h`
```c
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
```

#### `tokenizer.h` (for real use, see [this](https://github.com/code-forge-reaper/simple-assembly-language))
```c
#define CREATE_TOKENIZER
#define TK_KEYWORDS_LIST {\
  "reg","push","pop","add","sub","mult","div",\
  "eq","lt","gt","print","set","get","load","save",\
  "jmp","jz","jnz","label","concat","cast","dup",\
  "read", "call", "loadShared"\
}

#include "../../libs/tokenizer.h"

...


static char*readFile(const char*fn);
int main(int argc,char**argv){
    if(argc!=2){fprintf(stderr,"Usage: %s file.sal\n",argv[0]);return 1;}
    char*code=readFile(argv[1]); if(!code){perror("readFile");return 1;}

    initSalt();

    size_t tn; Token*t=tk_tokenize(code,argv[1],&tn);
    int pc; Instr*prog=parse(t,(int)tn,&pc); if(!prog)return 1;
    resolve_labels(prog,pc);
    run(prog,pc);
    free_prog(prog,pc);
    free(code); tk_free_tokens(t,tn);
    return 0;
}


static char*readFile(const char*fn){
    FILE*f=fopen(fn,"rb"); if(!f)return NULL;
    fseek(f,0,SEEK_END); size_t s=ftell(f); rewind(f);
    char*b=malloc(s+1); fread(b,1,s,f); fclose(f); b[s]='\0';
    return b;
}
```

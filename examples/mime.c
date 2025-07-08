#define CREATE_MIME_PARSER
#include "../mime.h"
int main(){
	read_mimetypes("mime.types");
	const char* mime = get_mime_from_extension("txt");
	printf("%s\n", mime);
	return 0;
}


#pragma once
// Call once at startup.  Path can be "/etc/mime.types" or your own file.
void read_mimetypes(const char *path);

// Given an extension (without the leading dot), returns the matching
// mime type, or "application/octet-stream" if unknown.
const char *get_mime_from_extension(const char *ext);

// Frees all internal data.  Call on shutdown.
void free_mimetypes(void);
#ifdef CREATE_MIME_PARSER
// mime.c
#define _GNU_SOURCE   // for getline()
#include "mime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct MimeEntry {
    char              *mimetype;
    char             **exts;
    size_t             ext_count;
    struct MimeEntry  *next;
} MimeEntry;

// head of our list
static MimeEntry *g_mime_list = NULL;

// safe malloc
static void *xmalloc(size_t sz) {
    void *p = malloc(sz);
    if (!p) { perror("malloc"); exit(1); }
    return p;
}

// safe strdup
static char *xstrdup(const char *s) {
    char *p = strdup(s);
    if (!p) { perror("strdup"); exit(1); }
    return p;
}

void read_mimetypes(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        perror(path);
        return;
    }

    char *line = NULL;
    size_t  len  = 0;

    while (getline(&line, &len, f) != -1) {
        // trim leading whitespace
        char *p = line;
        while (*p && isspace((unsigned char)*p)) p++;

        // skip blank or comment
        if (*p == '\0' || *p == '#') continue;

        // tokenize
        char *save;
        char *mime = strtok_r(p, " \t\r\n", &save);
        if (!mime) continue;

        // build entry
        MimeEntry *ent = xmalloc(sizeof *ent);
        ent->mimetype = xstrdup(mime);
        ent->exts     = NULL;
        ent->ext_count= 0;
        ent->next     = g_mime_list;
        g_mime_list   = ent;

        // gather extensions
        char *ext;
        while ((ext = strtok_r(NULL, " \t\r\n", &save)) != NULL) {
            // skip stray comments mid‑line
            if (ext[0] == '#') break;
            ent->exts = realloc(ent->exts, (ent->ext_count + 1) * sizeof *ent->exts);
            if (!ent->exts) { perror("realloc"); exit(1); }
            ent->exts[ent->ext_count++] = xstrdup(ext);
            #ifdef DUMP_MIME_ON_LOAD
            //printf("%s: %s\n", mime, ext);
            printf("filename.%s = %s\n", ext, mime);
            #endif
        }
    }

    free(line);
    fclose(f);
}

const char *get_mime_from_extension(const char *ext) {
    if (!ext) return "application/octet-stream";
    for (MimeEntry *e = g_mime_list; e; e = e->next) {
        for (size_t i = 0; i < e->ext_count; i++) {
            //printf("%s == %s(%b)\n", ext, e->exts[i], strcasecmp(ext, e->exts[i]) == 0);
            if (strcasecmp(ext, e->exts[i]) == 0){
                //printf("%s: %s\n", e->mimetype, e->exts[i]);
                return e->mimetype;
            }
        }
    }
    return "application/octet-stream";
}

void free_mimetypes(void) {
    while (g_mime_list) {
        MimeEntry *e = g_mime_list;
        g_mime_list = e->next;

        free(e->mimetype);
        for (size_t i = 0; i < e->ext_count; i++)
            free(e->exts[i]);
        free(e->exts);
        free(e);
    }
}
#endif // CREATE_MIME_PARSER

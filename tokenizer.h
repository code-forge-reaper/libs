#pragma once


#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

/* ─── Public Token API ───────────────────────────────────────────────────── */

typedef enum TokenType{
    TOK_STR,
    TOK_INT,
    TOK_FLOAT,
    TOK_ID,
    TOK_KEYWORD,
    TOK_CHAR,
    TOK_PP,
    TOK_ATTR,
    TOK_ELLIPSIS,
    TOK_PUNCT,
    TOK_OP,
    TOK_MAX // this is just so we know how big this enum is, not actualy used
} TokenType;

typedef struct {
    TokenType type; /* exact type */
    char *value;    /* exact text */
    int   line;     /* 1‑based */
    int   column;   /* 0‑based */
    char *file;     /* duplicated filename */
} Token;

/**
 * Tokenize a NUL‑terminated source string.
 * @param source     input text
 * @param filename   filename for error messages and Token.file
 * @param out_count  out‑param: number of tokens returned
 * @return           malloc’d Token array (must call tk_free_tokens)
 */
Token *tk_tokenize(const char *source, const char *filename, size_t *out_count);

/**
 * Free a Token array returned by tk_tokenize.
 */
void tk_free_tokens(Token *tokens, size_t count);


/* ─── Implementation (only when CREATE_TOKENIZER is defined) ────────────────── */

#ifdef CREATE_TOKENIZER

/* internal dynamic array for tokens */
typedef struct {
    Token *data;
    size_t count, cap;
} TokenArray;

static void tokens_init(TokenArray *a) {
    a->count = 0; a->cap = 16;
    a->data = malloc(a->cap * sizeof(Token));
}

static void tokens_push(TokenArray *a, Token t) {
    if (a->count == a->cap) {
        a->cap *= 2;
        a->data = realloc(a->data, a->cap * sizeof(Token));
    }
    a->data[a->count++] = t;
}

static void tokens_cleanup(TokenArray *a) {
    for (size_t i = 0; i < a->count; i++) {
        #ifdef TOKENIZER_DEBUG
        printf("%s:%i:%i: ", a->data[i].file, a->data[i].line, a->data[i].column);
        printf("freeing %i | %s\n", a->data[i].type, a->data[i].value);
        #endif
        free(a->data[i].value);
        free(a->data[i].file);
    }
    free(a->data);
}
#ifndef TK_KEYWORDS_LIST
#error "TK_KEYWORDS_LIST must be defined"
#endif


/* keyword list */
static const char *KEYWORDS[] = TK_KEYWORDS_LIST;
static const size_t NKEYWORDS = sizeof(KEYWORDS)/sizeof(*KEYWORDS);
static int is_keyword(const char *s) {
    for (size_t i = 0; i < NKEYWORDS; i++)
        if (strcmp(s, KEYWORDS[i]) == 0) return 1;
    return 0;
}

/* helpers */
static char *dup_range(const char *p, size_t len) {
    char *r = malloc(len+1);
    memcpy(r, p, len);
    r[len] = 0;
    return r;
}
static void lex_error(int line, int col, const char *line_text, const char *msg) {
    fprintf(stderr,
        "Tokenization error at line %d, column %d:\n%s\n%*s^\n%s\n",
        line, col, line_text, col, "", msg);
    exit(1);
}

/* the real workhorse */
Token *tk_tokenize(const char *source, const char *filename, size_t *out_count) {
    TokenArray toks; tokens_init(&toks);

    size_t idx = 0, len = strlen(source);
    int line = 1, col = 0;
    const char *line_start = source;

    while (idx < len) {
        char c = source[idx];
        int col0 = col;

        /* skip spaces/tabs */
        if (c==' '||c=='\t') { idx++; col++; continue; }
        /* newline */
        if (c=='\n') { line++; col=0; idx++; line_start = source+idx; continue; }

        /* ATTR: @foo */
        if (c=='@') {
            size_t st = idx++;
            col++;
            while (idx<len && !strchr(" \t\n", source[idx])) { idx++; col++; }
            Token t = { TOK_ATTR, dup_range(source+st, idx-st), line, col0, strdup(filename) };
            tokens_push(&toks, t);
            continue;
        }

        /* PP_DIRECTIVE: #... */
        if (c=='#') {
            size_t st = idx;
            while (idx<len && source[idx] != '\n') { idx++; col++; }
            Token t = { TOK_PP, dup_range(source+st, idx-st), line, col0, strdup(filename) };
            tokens_push(&toks, t);
            continue;
        }

        /* comments */
        if (c=='/' && idx+1 < len) {
            if (source[idx+1]=='/') {
                while (idx<len && source[idx]!='\n') idx++;
                continue;
            }
            if (source[idx+1]=='*') {
                int sl=line, sc=col;
                idx+=2; col+=2;
                while (idx+1<len && !(source[idx]=='*'&&source[idx+1]=='/')) {
                    if (source[idx]=='\n') { line++; col=0; }
                    else col++;
                    idx++;
                }
                if (idx+1>=len) {
                    size_t lsize = strcspn(line_start, "\n");
                    char *lt = dup_range(line_start, lsize);
                    lex_error(sl, sc, lt, "unterminated multiline comment");
                }
                idx+=2; col+=2;
                continue;
            }
        }

        /* ellipsis */
        if (idx+2<len && source[idx]=='.'&&source[idx+1]=='.'&&source[idx+2]=='.') {
            Token t = { TOK_ELLIPSIS, strdup("..."), line, col0, strdup(filename) };
            tokens_push(&toks, t);
            idx+=3; col+=3;
            continue;
        }

        /* char literal */
        if (c=='\'') {
            size_t st = idx++;
            col++;
            if (idx>=len) lex_error(line, col0, dup_range(line_start,strcspn(line_start,"\n")), "Unterminated character literal");
            if (source[idx]=='\\' && idx+1<len) { idx+=2; col+=2; }
            else { idx++; col++; }
            if (idx>=len || source[idx]!='\'')
                lex_error(line, col0, dup_range(line_start,strcspn(line_start,"\n")), "Unterminated character literal");
            idx++; col++;
            Token t = { TOK_CHAR, dup_range(source+st, idx-st), line, col0, strdup(filename) };
            tokens_push(&toks, t);
            continue;
        }

        /* string literal */
        if (c=='"') {
            size_t st = idx++;
            col++;
            while (idx<len && source[idx]!='"') {
                if (source[idx]=='\\' && idx+1<len) { idx+=2; col+=2; }
                else { idx++; col++; }
            }
            if (idx<len && source[idx]=='"') {
                size_t sl = idx-(st+1);
                idx++; col++;
                Token t = { TOK_STR, dup_range(source+st+1, sl), line, col0, strdup(filename) };
                tokens_push(&toks, t);
            } else
                lex_error(line, col0, dup_range(line_start,strcspn(line_start,"\n")), "Unterminated string literal");
            continue;
        }

        /* number (incl negative) */
        if ((c=='-'&&idx+1<len&&isdigit(source[idx+1]))||isdigit(c)) {
            bool isFloat = false;
            size_t st = idx;
            if (c=='-') { idx++; col++; }
            while (idx<len && isdigit(source[idx])) { idx++; col++; }
            if (idx<len && source[idx]=='.'&&idx+1<len&&isdigit(source[idx+1])) {
                idx++; col++;
                isFloat = true;
                while (idx<len && isdigit(source[idx])) { idx++; col++; }
            }
            TokenType type = isFloat ?  TOK_FLOAT : TOK_INT;
            Token t = { type, dup_range(source+st, idx-st), line, col0, strdup(filename) };
            tokens_push(&toks, t);
            continue;
        }

        /* identifier/keyword (or negative-id) */
        if (isalpha(c)||c=='_'||(c=='-'&&idx+1<len&&(isalpha(source[idx+1])||source[idx+1]=='_'))) {
            size_t st = idx;
            if (c=='-') { idx++; col++; }
            while (idx<len&&(isalnum(source[idx])||source[idx]=='_')) { idx++; col++; }
            size_t vl = idx-st;
            char *v = dup_range(source+st, vl);
            if (is_keyword(v)) {
                Token t = { TOK_KEYWORD, v, line, col0, strdup(filename) };
                tokens_push(&toks, t);
            } else {
                Token t = { TOK_ID, v, line, col0, strdup(filename) };
                tokens_push(&toks, t);
            }
            continue;
        }

        /* operators */
        if (strchr("=!<>+-*/&%|", c)) {
            char two[3] = {c, idx+1<len?source[idx+1]:0,0};
            const char *ops2[] = {"==","!=","<=",">=","+=","-=","*=","/=","&&","||"};
            int m=0;
            for (int i=0;i<10;i++) if (!strcmp(two, ops2[i])) {
                Token t = { TOK_OP, strdup(two), line, col0, strdup(filename) };
                tokens_push(&toks, t);
                idx+=2; col+=2; m=1; break;
            }
            if (m) continue;
            char s[2]={c,0};
            Token t = { TOK_OP, strdup(s), line, col0, strdup(filename) };
            tokens_push(&toks, t);
            idx++; col++;
            continue;
        }

        /* punctuation */
        if (strchr("().,{}:;[]", c)) {
            char s[2]={c,0};
            Token t = { TOK_PUNCT, strdup(s), line, col0, strdup(filename) };
            tokens_push(&toks, t);
            idx++; col++;
            continue;
        }

        /* unknown */
        {
            size_t lsize = strcspn(line_start,"\n");
            char *lt = dup_range(line_start, lsize);
            char msg[32];
            snprintf(msg,32,"Unknown character '%c'", c);
            lex_error(line, col, lt, msg);
        }
    }

    *out_count = toks.count;
    return toks.data;
}

/* free helper */
void tk_free_tokens(Token *tokens, size_t count) {
    TokenArray ta = { .data = tokens, .count = count, .cap = 0 };
    tokens_cleanup(&ta);
}

#endif /* CREATE_TOKENIZER */

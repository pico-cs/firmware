#ifndef _PROT_H
#define _PROT_H

#include "common.h"

#define PROT_BUFFER_SIZE 255

static const char prot_tag_start    = '+';
static const char prot_tag_success  = '=';
static const char prot_tag_error    = '?';
static const char prot_tag_multi    = '-';
static const char prot_tag_eor      = '.';
static const char prot_tag_push     = '!';

static const char prot_char_cr    = '\r';
static const char prot_char_nl    = '\n';
static const char prot_char_null  = '\0';
static const char prot_char_quote = '"';
static const char prot_char_space = ' ';

static const char prot_true   = 't';
static const char prot_false  = 'f';
static const char prot_toggle = '~';

// reader

typedef enum { 
    READER_INIT       = 0,
    READER_START      = 1,
    READER_END        = 2,
    READER_WHITESPACE = 3,
    READER_QUOTE      = 4,
    READER_CHAR       = 5,
} reader_status_t;

typedef struct {
    reader_status_t status;
	byte buf[PROT_BUFFER_SIZE];
	int pos; // position in buf
} reader_t;

// public interface
void reader_init(reader_t *reader);
void reader_reset(reader_t *reader);
bool reader_read_frame(reader_t *reader, const byte buf[], int size);
int reader_num_prm(reader_t *reader);
char *reader_get_prm(reader_t *reader, int idx);

bool parse_uint(char *ptr, uint *v);
bool parse_byte(char *ptr, byte *v);
bool parse_bool(char *ptr, bool *v);
bool parse_ternary(char *ptr, ternary_t *v);

// writer

typedef int (*flush_fn_t)(void *obj, const byte buf[], int size);

typedef struct {
    void *flusher;
    flush_fn_t flush_fn; 
	byte buf[PROT_BUFFER_SIZE];
	int pos; // position in buf
} writer_t;

// public interface
void writer_init(writer_t *writer, void *flusher, flush_fn_t flush_fn);

int write_event(writer_t *writer, const char text[]);
int write_eventf(writer_t *writer, const char *format, ...);
int write_error(writer_t *writer, const char text[]);
int write_success(writer_t *writer, const char *format, ...);
int write_multi(writer_t *writer, const char *format, ...);
int write_multi_start(writer_t *writer);
int write_multi_end(writer_t *writer);
int write(writer_t *writer, const char *format, ...);
int write_eor(writer_t *writer);

#endif
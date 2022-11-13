#ifndef _PROT_H
#define _PROT_H

#include "common.h"

#define READER_BUFFER_SIZE 255

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

typedef enum{
	PROT_TERNARY_FALSE  = 0,
	PROT_TERNARY_TRUE   = 1,
	PROT_TERNARY_TOGGLE = 2,
} prot_ternary_t;

// reader

typedef enum { 
    READER_INIT       = 0,
    READER_START      = 1,
    READER_END        = 2,
    READER_WHITESPACE = 3,
    READER_QUOTE      = 4,
    READER_CHAR       = 5,
} reader_status_t;

typedef int (*read_char_fn)(uint32_t timeout_us);

typedef struct {
    uint32_t timeout_us;
    read_char_fn read_char;
    reader_status_t status;
	byte buf[READER_BUFFER_SIZE];
	int pos; // position in buf
} reader_t;

// public interface
void reader_init(reader_t *reader, read_char_fn read_char);
void reader_reset(reader_t *reader);
void reader_poll(reader_t *reader);
bool reader_has_frame(reader_t *reader);
int reader_num_prm(reader_t *reader);
char *reader_get_prm(reader_t *reader, int idx);

bool parse_uint(char *ptr, uint *v);
bool parse_byte(char *ptr, byte *v);
bool parse_bool(char *ptr, bool *v);
bool parse_ternary(char *ptr, prot_ternary_t *v);

// writer

typedef int (*write_string_fn)(const char *s);
typedef int (*write_char_fn)(int ch);

typedef struct {
    write_string_fn write_string;
    write_char_fn write_char; 
} writer_t;

// public interface
void writer_init(writer_t *writer, write_string_fn write_string, write_char_fn write_char);

int write_log(writer_t *writer, const char text[]);
int write_error(writer_t *writer, const char text[]);
int write_success(writer_t *writer, const char *format, ...);
int write_multi(writer_t *writer, const char *format, ...);
int write_eor(writer_t *writer);

#endif
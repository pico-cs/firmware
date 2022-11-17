#include <stdarg.h>
#include <limits.h>
#include <string.h>

#include "pico/printf.h"

#include "prot.h"

// reader

static bool read_is_end(int ch) {
    return (ch == prot_char_cr || ch == prot_char_nl || ch == prot_char_null);
}

static bool read_is_whitespace(int ch) {
    return (ch == prot_char_space);
}

void reader_init(reader_t *reader) {
    reader_reset(reader);
}

void reader_reset(reader_t *reader) {
    reader->status = READER_INIT;
    reader->pos = 0;
}

// TODO: overflow -> write log
bool reader_read_frame(reader_t *reader, const byte buf[], int size) {

    for (int i = 0; i < size; i++) {

        int ch = buf[i];

        switch (reader->status) {

        case READER_INIT:
            if (ch == prot_tag_start) {
                reader->status = READER_START;
            }
            break;

        case READER_END:
            return true;

        default:

            // overflow check
            if (reader->pos >= PROT_BUFFER_SIZE) {
                // ignore input
                reader->pos = 0;
                reader->status = READER_INIT;
                break;
            }
            
            if (read_is_end(ch)) {
                reader->status = READER_END;
                reader->buf[reader->pos++] = prot_char_null;
                return true;
            }

            if (read_is_whitespace(ch)) {
                if (reader->status != READER_WHITESPACE) {
                    reader->status = READER_WHITESPACE;
                    reader->buf[reader->pos++] = prot_char_null;
                }    
                break;
            }

            if (ch == prot_char_quote) {
                reader->status = READER_QUOTE;
                break;
            }
          
            // TODO QUOTE / STRING handling
            reader->status = READER_CHAR;
            reader->buf[reader->pos++] = ch;

        }
    }
    return false;
}

int reader_num_prm(reader_t *reader) {
    int cnt = 0;
    for (int i = 0; i < reader->pos; i++) {
        if (reader->buf[i] == '\0') cnt++;
    }
    return cnt;
}

char *reader_get_prm(reader_t *reader, int idx) {
    int start = 0;
    int part = 0;

    for (int i = 0; i < reader->pos; i++) {
        if (reader->buf[i] == '\0') {
            if (part == idx) {
                return &reader->buf[start];
            }
            part++;
            start = i+1;
        }
    }
    return NULL;
}

bool parse_uint(char *ptr, uint *v) {
    //TODO overflow
    *v = 0;
    while (*ptr != prot_char_null) {
        if (*ptr < '0' || *ptr > '9') return false;
        // cmd_printf("loop %c\n", *ptr);
        *v = (*v * 10) + (*ptr - '0');
        // cmd_printf("%d\n", *v);
        *ptr++;
    }
    return true;
}

bool parse_byte(char *ptr, byte *v) {
    uint u;
    if (!parse_uint(ptr, &u)) return false;
    if (u > UCHAR_MAX) return false;
    *v = (byte) u;
    return true;
}

bool parse_bool(char *ptr, bool *v) {
    *v = false;
    if (strlen(ptr) != 1) return false;
    switch (*ptr) {
    case prot_true:  *v = true;  return true;
    case prot_false: *v = false; return true;
    }
    return false;
}

bool parse_ternary(char *ptr, prot_ternary_t *v) {
    *v = PROT_TERNARY_FALSE;
    if (strlen(ptr) != 1) return false;
    switch (*ptr) {
    case prot_true:   *v = PROT_TERNARY_TRUE;   return true;
    case prot_false:  *v = PROT_TERNARY_FALSE;  return true;
    case prot_toggle: *v = PROT_TERNARY_TOGGLE; return true;
    }
    return false;
}

// writer

// forward declaration
static void writer_write_char(writer_t *writer, char ch);

// called by vfctprintf
void write_char(char ch, void *arg) {
    writer_t *writer = (writer_t*)arg;
    writer_write_char(writer, ch);
}

static int writer_flush(writer_t *writer) {
    // TODO check writer-pos --> return error
    //if (writer->pos >= PROT_BUFFER_SIZE) {
    // TODO error event
    //}
    int size = writer->pos;
    writer->flush(writer->flusher, writer->buf, size);
    writer->pos = 0;
    return size;
}

static void writer_write_char(writer_t *writer, char ch) {
    if (writer->pos < PROT_BUFFER_SIZE) {
        writer->buf[writer->pos++] = ch;
    }   
}    

static void writer_write_string(writer_t *writer, const char *s) {
    while (*s != 0 && writer->pos < PROT_BUFFER_SIZE) {
        writer->buf[writer->pos++] = *s;
        s++;
    }
}

void writer_init(writer_t *writer, void *flusher, flush_fn flush) {
    writer->flusher = flusher;
    writer->flush = flush;
    writer->pos = 0;
}

int write_event(writer_t *writer, const char text[]) {
    writer_write_char(writer, prot_tag_push);
    writer_write_string(writer, text);
    writer_write_char(writer, prot_char_nl);
    return writer_flush(writer);
}

int write_eventf(writer_t *writer, const char *format, ...) {
    writer_write_char(writer, prot_tag_push);
    va_list va;
    va_start(va, format);
    vfctprintf(&write_char, (void *)writer, format, va);
    va_end(va);
    writer_write_char(writer, prot_char_nl);
    writer_flush(writer);
}

int write_error(writer_t *writer, const char text[]) {
    writer_write_char(writer, prot_tag_error);
    writer_write_string(writer, text);
    writer_write_char(writer, prot_char_nl);
    return writer_flush(writer);
}

int write_success(writer_t *writer, const char *format, ...) {
    writer_write_char(writer, prot_tag_success);
    va_list va;
    va_start(va, format);
    vfctprintf(&write_char, (void *)writer, format, va);
    va_end(va);
    writer_write_char(writer, prot_char_nl);
    return writer_flush(writer);
}

int write_multi(writer_t *writer, const char *format, ...) {
    writer_write_char(writer, prot_tag_multi);
    va_list va;
    va_start(va, format);
    vfctprintf(&write_char, (void *)writer, format, va);
    va_end(va);
    writer_write_char(writer, prot_char_nl);
    return writer_flush(writer);
}

int write_eor(writer_t *writer) { 
    writer_write_char(writer, prot_tag_eor);
    writer_write_char(writer, prot_char_nl);
    return writer_flush(writer);
}

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "assert.h"
#include "io.h"

struct IO
{
    // for the file implementation
    FILE *file;

    // for the buffer implementation
    char *buffer;
    size_t buffer_pos;
    size_t buffer_size;

    void (*seek)(struct IO *, size_t, int);
    void (*read)(struct IO *, size_t, void *);
    bool (*write)(struct IO *, size_t, void *);
    size_t (*tell)(struct IO *);
    size_t (*size)(struct IO *);
};



static void file_io_seek(IO *io, size_t offset, int whence)
{
    assert_not_null(io);
    fseek(io->file, offset, whence);
}

static void file_io_read(IO *io, size_t length, void *destination)
{
    assert_not_null(io);
    fread(destination, 1, length, io->file);
}

static bool file_io_write(IO *io, size_t length, void *source)
{
    assert_not_null(io);
    if (fwrite(source, 1, length, io->file) == length)
    {
        return true;
    }
    errno = EIO;
    return false;
}

static size_t file_io_tell(IO *io)
{
    assert_not_null(io);
    return ftell(io->file);
}

static size_t file_io_size(IO *io)
{
    size_t size;
    size_t old_pos;
    assert_not_null(io);
    old_pos = ftell(io->file);
    fseek(io->file, 0, SEEK_END);
    size = ftell(io->file);
    fseek(io->file, old_pos, SEEK_SET);
    return size;
}



static void buffer_io_seek(IO *io, size_t offset, int whence)
{
    assert_not_null(io);
    if (whence == SEEK_SET)
        io->buffer_pos = offset;
    else if (whence == SEEK_END)
        io->buffer_pos = io->buffer_size - 1 - offset;
    else if (whence == SEEK_CUR)
        io->buffer_pos += offset;
    else
        assert_that(1 == 0);
}

static void buffer_io_read(IO *io, size_t length, void *destination)
{
    assert_not_null(io);
    assert_that(io->buffer_pos + length <= io->buffer_size);
    strncpy(destination, io->buffer + io->buffer_pos, length);
    io->buffer_pos += length;
}

static bool buffer_io_write(IO *io, size_t length, void *source)
{
    char *destination;
    assert_not_null(io);
    if (io->buffer_pos + length > io->buffer_size)
    {
        destination = realloc(io->buffer, io->buffer_pos + length);
        if (!destination)
        {
            errno = ENOMEM;
            return false;
        }
        io->buffer_size = io->buffer_pos + length;
        io->buffer = destination;
    }
    destination = io->buffer + io->buffer_pos;
    strncpy(destination, source, length);
    io->buffer_pos += length;
    return true;
}

static size_t buffer_io_tell(IO *io)
{
    assert_not_null(io);
    return io->buffer_pos;
}

static size_t buffer_io_size(IO *io)
{
    return io->buffer_size;
}



IO *io_create_from_file(const char *const path, const char *const read_mode)
{
    FILE *fp = fopen(path, read_mode);
    if (!fp)
        return NULL;
    IO *io = malloc(sizeof(IO));
    io->file = fp;
    io->buffer = NULL;
    io->buffer_pos = 0;
    io->buffer_size = 0;
    io->seek = &file_io_seek;
    io->read = &file_io_read;
    io->write = &file_io_write;
    io->tell = &file_io_tell;
    io->size = &file_io_size;
    return io;
}

IO *io_create_from_buffer(const char *buffer, size_t buffer_size)
{
    IO *io = malloc(sizeof(IO));
    io->file = NULL;
    io->buffer = strdup(buffer);
    io->buffer_pos = 0;
    io->buffer_size = buffer_size;
    io->seek = &buffer_io_seek;
    io->read = &buffer_io_read;
    io->write = &buffer_io_write;
    io->tell = &buffer_io_tell;
    io->size = &buffer_io_size;
    return io;
}

void io_destroy(IO *io)
{
    assert_not_null(io);
    if (io->file != NULL)
        fclose(io->file);
    if (io->buffer != NULL)
        free(io->buffer);
}

size_t io_size(IO *io)
{
    assert_not_null(io);
    assert_that(io->size != NULL);
    return io->size(io);
}

void io_seek(IO *io, size_t offset)
{
    assert_not_null(io);
    assert_that(io->seek != NULL);
    io->seek(io, offset, SEEK_SET);
}

void io_skip(IO *io, size_t offset)
{
    assert_not_null(io);
    assert_that(io->seek != NULL);
    io->seek(io, offset, SEEK_CUR);
}

size_t io_tell(IO *io)
{
    assert_not_null(io);
    assert_that(io->tell != NULL);
    return io->tell(io);
}

char *io_read_string(IO *io, size_t length)
{
    char *str;
    assert_not_null(io);
    assert_that(io->read != NULL);
    str = malloc(length + 1);
    io->read(io, length, str);
    str[length] = '\0';
    return str;
}

char *io_read_until_zero(IO *io)
{
    char *str = NULL;
    char c;
    size_t length = 0;
    assert_not_null(io);
    do
    {
        str = realloc(str, length);
        c = io_read_u8(io);
        str[length ++] = c;
    }
    while (c != '\0');
    return str;
}

uint8_t io_read_u8(IO *io)
{
    uint8_t ret = 0;
    assert_not_null(io);
    assert_that(io->read != NULL);
    io->read(io, 1, &ret);
    return ret;
}

uint16_t io_read_u16(IO *io)
{
    uint16_t ret = 0;
    assert_not_null(io);
    assert_that(io->read != NULL);
    io->read(io, 2, &ret);
    return ret;
}

uint32_t io_read_u32(IO *io)
{
    uint32_t ret = 0;
    assert_not_null(io);
    assert_that(io->read != NULL);
    io->read(io, 4, &ret);
    return ret;
}

uint64_t io_read_u64(IO *io)
{
    uint64_t ret = 0;
    assert_not_null(io);
    assert_that(io->read != NULL);
    io->read(io, 8, &ret);
    return ret;
}

bool io_write_string(IO *io, char *str, size_t length)
{
    assert_not_null(io);
    assert_that(io->write != NULL);
    return io->write(io, length, str);
}

bool io_write_u8(IO *io, uint8_t value)
{
    assert_not_null(io);
    assert_that(io->write != NULL);
    return io->write(io, 1, &value);
}

bool io_write_u16(IO *io, uint16_t value)
{
    assert_not_null(io);
    assert_that(io->write != NULL);
    return io->write(io, 2, &value);
}

bool io_write_u32(IO *io, uint32_t value)
{
    assert_not_null(io);
    assert_that(io->write != NULL);
    return io->write(io, 4, &value);
}

bool io_write_u64(IO *io, uint64_t value)
{
    assert_not_null(io);
    assert_that(io->write != NULL);
    return io->write(io, 8, &value);
}

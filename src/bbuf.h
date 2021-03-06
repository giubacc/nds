/* Original Work Copyright (c) 2021 Giuseppe Baccini - giuseppe.baccini@live.com

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once
#include "nds.h"

namespace nds {

/**
 * A simple growing byte buffer providing read/write capabilities.
 * Used when receiving/sending from sockets.
 *
*/
struct g_bbuf {
    explicit g_bbuf();
    explicit g_bbuf(size_t initial_capacity);
    explicit g_bbuf(const g_bbuf &);
    explicit g_bbuf(g_bbuf &&);

    ~g_bbuf();

    void reset() {
        pos_ = limit_ = mark_ = 0;
    }

    void set_read() {
        pos_ = mark_;
    }

    void set_write() {
        pos_ = limit_;
    }

    void compact();

    RetCode grow(size_t amount);
    RetCode ensure_capacity(size_t capacity);

    RetCode append(g_bbuf &);
    size_t append_no_rsz(g_bbuf &);

    RetCode append(const void *buffer,
                   size_t offset,
                   size_t length);

    RetCode append_ushort(unsigned short);
    RetCode append_uint(unsigned int);

    RetCode put(const void *buffer,
                size_t gbb_offset,
                size_t length);

    size_t position() const;

    size_t limit() const {
        return limit_;
    }

    size_t mark() const {
        return mark_;
    }

    size_t capacity() const {
        return capcty_;
    }

    size_t remaining() const {
        return capcty_ - pos_;
    }

    unsigned char *buffer() {
        return (unsigned char *)buf_;
    }

    char *buffer_as_char() {
        return buf_;
    }

    unsigned int *buffer_as_uint() {
        return (unsigned int *)buf_;
    }

    RetCode advance_pos_write(size_t amount);
    RetCode set_pos_write(size_t new_pos);

    void move_pos_write(size_t amount) {
        pos_ += amount;
        limit_ = pos_;
    }

    RetCode advance_pos_read(size_t amount);
    RetCode set_pos_read(size_t new_pos);

    void set_mark() {
        mark_ = pos_;
    }

    RetCode set_mark(size_t new_mark);

    size_t from_mark() const {
        return pos_ - mark_;
    }

    size_t available_read() {
        return limit_ - pos_;
    }

    RetCode read(size_t length, void *out);
    RetCode read(size_t length, g_bbuf &out);
    RetCode read_ushort(unsigned short *);
    RetCode read_uint(unsigned int *);
    RetCode read_uint_to_sizet(size_t *);

    size_t capcty_;
    size_t pos_;
    size_t limit_;
    size_t mark_;
    char *buf_;
};
}

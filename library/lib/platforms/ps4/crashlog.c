// SPDX-FileCopyrightText: borealis contributors
// SPDX-License-Identifier: Apache-2.0

// Thanks to: https://gist.github.com/john-tornblom/943dc2e995f0fa0c9d2dc2f9d985daeb

#include <signal.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>

#include <orbis/libkernel.h>

#define MAX_MESSAGE_SIZE 0x1000
#define MAX_STACK_FRAMES 32

/**
 * A callframe captures the stack and program pointer of a
 * frame in the call path.
 **/
typedef struct
{
    void* sp;
    void* pc;
} callframe_t;

/**
 * A message built in a fixed buffer, always NUL-terminated, cut when full.
 * Plain loops only: snprintf() and strncat() are not async-signal-safe.
 **/
typedef struct
{
    char* data;
    size_t size;
    size_t len;
} message_t;

static void
append_text(message_t* m, const char* s, size_t max)
{
    for (size_t i = 0; i < max && s[i] && m->len + 1 < m->size; i++)
        m->data[m->len++] = s[i];
    m->data[m->len] = '\0';
}

static void
append(message_t* m, const char* s)
{
    append_text(m, s, (size_t)-1);
}

// `s`, at most `max` bytes of it, right-aligned on `width` columns (%*s)
static void
append_padded(message_t* m, const char* s, size_t max, size_t width)
{
    size_t n = 0;
    while (n < max && s[n])
        n++;
    for (; n < width; width--)
        append(m, " ");
    append_text(m, s, max);
}

// 0x and the value in hexadecimal (%lx)
static void
append_hex(message_t* m, unsigned long value)
{
    char digits[2 + sizeof value * 2 + 1];
    size_t i  = sizeof digits - 1;
    digits[i] = '\0';
    do
    {
        digits[--i] = "0123456789abcdef"[value & 0xf];
        value >>= 4;
    } while (value && i > 2);
    digits[--i] = 'x';
    digits[--i] = '0';
    append(m, digits + i);
}

// The value in decimal, at least `width` digits (%0*u)
static void
append_dec(message_t* m, unsigned int value, int width)
{
    char digits[16];
    size_t i  = sizeof digits - 1;
    digits[i] = '\0';
    do
    {
        digits[--i] = (char)('0' + value % 10);
        value /= 10;
        width--;
    } while ((value || width > 0) && i > 0);
    append(m, digits + i);
}

/**
 * Log a backtrace to /dev/klog
 **/
static void
backtrace(const char* reason, int sig)
{
    static char addr2line_data[MAX_STACK_FRAMES * 20];
    static char buf_data[MAX_MESSAGE_SIZE];
    callframe_t frames[MAX_STACK_FRAMES];
    OrbisKernelVirtualQueryInfo info;
    unsigned int nb_frames = 0;
    message_t buf          = { buf_data, sizeof buf_data, 0 };
    message_t addr2line    = { addr2line_data, sizeof addr2line_data, 0 };

    memset(frames, 0, sizeof frames);
    buf_data[0]       = '\0';
    addr2line_data[0] = '\0';

    append(&buf, "<118>[Crashlog]: ");
    append(&buf, reason);
    append_dec(&buf, (unsigned int)sig, 1);
    append(&buf, "\n<118>[Crashlog]: Backtrace:\n");
    sceKernelBacktraceSelf(frames, sizeof frames, &nb_frames, 0);
    if (nb_frames > MAX_STACK_FRAMES)
        nb_frames = MAX_STACK_FRAMES;
    for (unsigned int i = 0; i < nb_frames; i++)
    {
        memset(&info, 0, sizeof info);
        sceKernelVirtualQuery(frames[i].pc, 0, &info, sizeof info);
        unsigned long offset = (unsigned long)(frames[i].pc - info.unk01 - 1);

        append(&buf, "<118>[Crashlog]:   #");
        append_dec(&buf, i + 1, 2);
        append(&buf, " ");
        append_padded(&buf, info.name, sizeof info.name, 32);
        append(&buf, ": ");
        append_hex(&buf, offset);
        append(&buf, "\n");

        append_hex(&addr2line, offset);
        append(&addr2line, " ");
    }

    append(&buf, "<118>[Crashlog]: addr2line -e *.oelf -f -p -C -a ");
    append(&buf, addr2line_data);
    append(&buf, "\n");
    // Cut short: it still ends the line
    if (buf.len + 1 == buf.size)
        buf_data[buf.len - 1] = '\n';

    sceKernelDebugOutText(0, buf_data);
}

/**
 * Log fatal signals to kernel log.
 **/
static void
fatal_signal(int sig)
{
    backtrace("Received the fatal POSIX signal ", sig);
    _exit(1);
}

/**
 * Register handlers for fatal signals.
 **/
static void __attribute__((constructor))
init_signal(void)
{
    signal(5, fatal_signal); // SIGTRAP
    signal(6, fatal_signal); // SIGABRT
    signal(8, fatal_signal); // SIGFPE
    signal(10, fatal_signal); // SIGBUS
    signal(11, fatal_signal); // SIGSEGV
}
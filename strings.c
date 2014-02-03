/*
 * strings - find ASCII strings (and ASCII parts of
 *           UNICODE strings - both be and le) in
 *           binary files
 *
 * The author of this code (a) does not expect anything from you, and (b)
 * is not responsible for any of the problems you may have using this code.
 *
 * compile: gcc -m32 -o strings_test -D TEST strings.c
 * compile: gcc -m32 -o strings strings.c
 *
 * caveat:
 *      If strings is used on a file that contains a sequence of more than
 *      MAX_MATCHLEN ASCII characters, the sequence will be printed across
 *      multiple lines.  The screen width of the device behind stdout would
 *      be a better value for MAX_MATCHLEN.
 */

#define VERSION "version 1.00 2013/04/12\0"     // yes the \0 is useful ;)

#define STRLEN          6
#define MAX_MATCHLEN    4096

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

unsigned char   Filebuf[4096];                  // (not related to MAX_MATCHLEN)

unsigned char   Match_buf[MAX_MATCHLEN+1];
unsigned char * Match_buf_p;


//
// init_string() - begin a new match
//
void
init_string(void)
{
    Match_buf_p = Match_buf;
}

//
// reset_string() - complete a match
//
void
reset_string(void)
{
    // did we get a long enough string to print?
    if (Match_buf_p - Match_buf >= STRLEN) {
        *Match_buf_p = '\0';
        printf("%s\n", Match_buf);
    }

    init_string();
}

//
// add_char(c) - add another character to a match
//
void
add_char(unsigned char c)
{
    *Match_buf_p++ = c;

    // string too long?
    if (Match_buf_p - Match_buf >= MAX_MATCHLEN)
        reset_string();
}


//
// scan_buf(buf, len) - scan a buffer for printable characters
//
void
scan_buf(unsigned char * buf, int n)
{
    unsigned char * p;
    int    null_was_last = 0;

    for (p = buf; p < buf + n; p++) {
        // if a printable character is found
        if (*p >= ' '  &&  *p <= '~') {
            add_char(*p);
            null_was_last = 0;
            continue;
        // if char is not printable, 1 null is acceptable in
        // between printable characters.  This catches the
        // Latin part of both the little endian and big endian
        // versions of the UNICODE character set.
        } else if (!null_was_last  &&  *p == '\0') {
            null_was_last = 1;
            continue;
        }
        // else its not, or is no longer a string
        reset_string();
    }
}

//
// scan_file(fname) - scan a file for printable characters
//
void
scan_file(char * fname)
{
    int fd;
    int n;

    printf("%s\n", fname);
    fd = open(fname, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "cannot open %s: %s\n", fname, strerror(errno));
        return;
    }

    while ((n = read(fd, Filebuf, sizeof(Filebuf))) > 0)
        scan_buf(Filebuf, n);

    if (n < 0)
        fprintf(stderr, "cannot read %s: %s\n", fname, strerror(errno));
    close(fd);
}

// ---------------------------------------------------------------
// test code

#ifdef TEST

// The test buffer is used 16 bytes at a time (instead of 4096)
// so boundary crossing issues can be tested.  Embedded strings
// are the minimum length.
//
// Should print:
//      test01
//      test02
//      test03
//      test04
//      test05
//      test06
//      test07
//      test08
//      test09
//      test10
//      test11
//      test12

unsigned char testbuf[] = {
    // normal buffering tests
    "\xa5\xa5\xa5\xa5test01\xa5\xa5\xa5\xa5\xa5\xa5"            // ASCII
    "\xa5\xa5t\0e\0s\0t\0\x30\0\x32\0\xa5\xa5"                  // UNICODE be
    "\xa5\xa5\0t\0e\0s\0t\0\x30\0\x33\xa5\xa5"                  // UNICODE le
    // boundary crossing tests
    "\xa5\xa5\xa5\xa5\xa5\xa5\xa5\xa5\xa5\xa5\xa5\xa5test"      // ASCII
    "04\xa5\xa5\xa5\xa5\xa5\xa5\xa5\xa5\xa5\xa5\xa5\xa5\xa5\xa5"
    "\xa5\xa5\xa5\xa5\xa5\xa5\xa5\xa5t\0e\0s\0t\0"              // UNICODE be
    "0\0\x35\0\xa5\xa5\xa5\xa5\xa5\xa5\xa5\xa5\xa5\xa5\xa5\xa5"
    "\xa5\xa5\xa5\xa5\xa5\xa5\xa5\xa5\0t\0e\0s\0t"              // UNICODE le
    "\0\x30\0\x36\xa5\xa5\xa5\xa5\xa5\xa5\xa5\xa5\xa5\xa5\xa5"
    // end on a boundary tests
    "\xa5\xa5\xa5\xa5\xa5t\0e\0s\0t\0\x30\0\x37"                // UNICODE be
    "\0\xa5\xa5\xa5\xa5\0t\0e\0s\0t\0\x30\0"                    // UNICODE le
    // end near a boundary tests
    "\x38\xa5\xa5\xa5\xa5\xa5t\0e\0s\0t\0\x30\0"                // UNICODE be
    "9\xa5\xa5\xa5\xa5\0t\0e\0s\0t\0\x31\0"                     // UNICODE le
    "0\xa5\xa5\xa5\xa5\xa5t\0e\0s\0t\0\x31\0"                   // UNICODE be
    "1\0\xa5\xa5\xa5\0t\0e\0s\0t\0\x31\0"                       // UNICODE le
    "2\xa5\xa5\xa5\xa5\xa5\xa5\xa5\xa5\xa5\xa5\xa5\xa5\xa5\xa5\xa5"
};

char __version__[] = "test " VERSION;

int
main(int ac, char * av[])
{
    unsigned char * p;

    if (sizeof(testbuf) % 16 != 0) {
        printf("incorrect testbuf length "
               "(0x%x <- last hex digit must be zero)\n", sizeof(testbuf));
        return 1;
    }
    for (p = testbuf; p < testbuf + sizeof(testbuf); p += 16)
        scan_buf(p, 16);
}

#else /* not TEST */

// ---------------------------------------------------------------
// main

char __version__[] = "release " VERSION;

int
main(int ac, char * av[])
{
    int ai = 1;

    init_string();

    while (ai < ac)
        scan_file(av[ai++]);

    reset_string();
}

#endif

// ex: set tabstop=8 expandtab softtabstop=4 shiftwidth=4:

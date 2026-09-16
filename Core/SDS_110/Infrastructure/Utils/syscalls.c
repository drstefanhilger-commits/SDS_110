#include <stdint.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>


void _exit(int status) {
    while (1) {
        __asm__("wfi");
    }
}

int _close(int file) { return -1; }

int _fstat(int file, struct stat *st) {
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int file) { return 1; }

int _lseek(int file, int ptr, int dir) { return 0; }

int _read(int file, char *ptr, int len) {
    errno = EINVAL;
    return -1;
}

int _write(int file, const char *ptr, int len) {
    return len;
}

int _kill(int pid, int sig) {
    errno = EINVAL;
    return -1;
}

int _getpid(void) { return 1; }

//caddr_t _sbrk(int incr) {
//    extern char _end;      // linker symbol
//    extern char _estack;   // linker symbol
//
//    static char *heap_end;
//    char *prev_heap_end;
//
//    if (heap_end == 0) {
//        heap_end = &_end;
//    }
//
//    prev_heap_end = heap_end;
//
//    if (heap_end + incr > &_estack) {
//        errno = ENOMEM;
//        return (caddr_t)-1;
//    }
//
//    heap_end += incr;
//    return (caddr_t)prev_heap_end;
//}

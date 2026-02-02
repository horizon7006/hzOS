
#include <mlibc/all-sysdeps.hpp>
#include <mlibc/debug.hpp>
#include <errno.h>

namespace mlibc {

namespace {
constexpr long kSysExit = 1;
constexpr long kSysRead = 3;
constexpr long kSysWrite = 4;

static inline long do_syscall(long num, long a, long b, long c, long d, long e) {
    long ret;
    asm volatile("int $0x80"
                 : "=a"(ret)
                 : "a"(num), "b"(a), "c"(b), "d"(c), "S"(d), "D"(e)
                 : "memory");
    return ret;
}
} // namespace

void sys_libc_log(const char *message) {
    if (!message) {
        return;
    }

    size_t len = 0;
    while (message[len]) {
        len++;
    }

    ssize_t bytes_written = 0;
    sys_write(1, message, len, &bytes_written);
}

void sys_libc_panic(const char *message) {
    sys_libc_log(message);
    sys_exit(1);
}

int sys_tcb_set(void *pointer) {
    // TODO: Implement FS_BASE/GS_BASE setting
    (void)pointer;
    return ENOSYS;
}

int sys_anon_allocate(size_t size, void **pointer) {
    // TODO: Implement mmap syscall
    (void)size;
    (void)pointer;
    return ENOSYS;
}

int sys_anon_free(void *pointer, size_t size) {
    // TODO: Implement munmap syscall
    (void)pointer;
    (void)size;
    return ENOSYS;
}

void sys_exit(int status) {
    do_syscall(kSysExit, status, 0, 0, 0, 0);
    __builtin_unreachable();
}

int sys_open(const char *path, int flags, mode_t mode, int *fd) {
    (void)path;
    (void)flags;
    (void)mode;
    (void)fd;
    return ENOSYS;
}

int sys_read(int fd, void *buf, size_t count, ssize_t *bytes_read) {
    if (!bytes_read) {
        return EINVAL;
    }

    long ret = do_syscall(kSysRead, fd, (long)buf, count, 0, 0);
    if (ret < 0) {
        return EIO;
    }

    *bytes_read = ret;
    return 0;
}

int sys_write(int fd, const void *buf, size_t count, ssize_t *bytes_written) {
    if (!bytes_written) {
        return EINVAL;
    }

    long ret = do_syscall(kSysWrite, fd, (long)buf, count, 0, 0);
    if (ret < 0) {
        return EIO;
    }

    *bytes_written = ret;
    return 0;
}

int sys_close(int fd) {
    (void)fd;
    return ENOSYS;
}

int sys_seek(int fd, off_t offset, int whence, off_t *new_offset) {
    (void)fd;
    (void)offset;
    (void)whence;
    (void)new_offset;
    return ENOSYS;
}

int sys_vm_map(void *hint, size_t size, int prot, int flags,
               int fd, off_t offset, void **window) {
    (void)hint;
    (void)size;
    (void)prot;
    (void)flags;
    (void)fd;
    (void)offset;
    (void)window;
    return ENOSYS;
}

int sys_vm_unmap(void *pointer, size_t size) {
    (void)pointer;
    (void)size;
    return ENOSYS;
}

} // namespace mlibc

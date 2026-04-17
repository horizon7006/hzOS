
#include <mlibc/all-sysdeps.hpp>
#include <mlibc/debug.hpp>
#include <errno.h>

namespace mlibc {

void sys_libc_log(const char *message) {
    // TODO: Implement a system call for logging
    // For now, we might not be able to do anything without a syscall
}

void sys_libc_panic(const char *message) {
    sys_libc_log(message);
    while (1);
}

int sys_tcb_set(void *pointer) {
    // TODO: Implement FS_BASE/GS_BASE setting
    return 0;
}

int sys_anon_allocate(size_t size, void **pointer) {
    // TODO: Implement mmap syscall
    return -1;
}

int sys_anon_free(void *pointer, size_t size) {
    // TODO: Implement munmap syscall
    return 0;
}

void sys_exit(int status) {
    // TODO: Implement exit syscall
    while (1);
}

int sys_open(const char *path, int flags, mode_t mode, int *fd) {
    return -1;
}

int sys_read(int fd, void *buf, size_t count, ssize_t *bytes_read) {
    return -1;
}

int sys_write(int fd, const void *buf, size_t count, ssize_t *bytes_written) {
    return -1;
}

int sys_close(int fd) {
    return 0;
}

int sys_seek(int fd, off_t offset, int whence, off_t *new_offset) {
    return -1;
}

int sys_vm_map(void *hint, size_t size, int prot, int flags,
               int fd, off_t offset, void **window) {
    return -1;
}

int sys_vm_unmap(void *pointer, size_t size) {
    return -1;
}

} // namespace mlibc

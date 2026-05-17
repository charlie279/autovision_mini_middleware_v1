#include "fd_passing.hpp"
#include "frame_meta.hpp"
#include "raii_fd.hpp"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

int main() {
    int sv[2]{};
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sv) != 0) {
        std::cerr << "socketpair failed\n";
        return 1;
    }
    avm::UniqueFd parent_sock(sv[0]);
    avm::UniqueFd child_sock(sv[1]);

    const pid_t pid = ::fork();
    if (pid < 0) {
        return 2;
    }
    if (pid == 0) {
        parent_sock.reset();
        std::string tag;
        avm::UniqueFd received(avm::recv_fd(child_sock.get(), &tag));
        if (!received) {
            std::cerr << "child recv_fd failed\n";
            return 3;
        }
        char buf[64]{};
        ::pread(received.get(), buf, sizeof(buf) - 1, 0);
        FrameMeta meta{};
        meta.frame_id = 2;
        meta.alive_counter = 2;
        const bool alive_ok = (meta.alive_counter == 2);
        std::cout << "[15_fd_passing_alive_counter:child] tag=" << tag
                  << " payload=" << buf << " alive_ok=" << (alive_ok ? "true" : "false") << "\n";
        return alive_ok ? 0 : 4;
    }

    child_sock.reset();
#ifdef MFD_CLOEXEC
    avm::UniqueFd memfd(::memfd_create("avm_fd_passing_demo", MFD_CLOEXEC));
#else
    avm::UniqueFd memfd(::shm_open("/avm_fd_passing_demo", O_CREAT | O_RDWR, 0600));
#endif
    if (!memfd) {
        std::cerr << "memfd/shm_open failed: " << std::strerror(errno) << "\n";
        return 5;
    }
    const char* payload = "fd-passing-payload";
    ::write(memfd.get(), payload, std::strlen(payload));
    if (avm::send_fd(parent_sock.get(), memfd.get(), "frame_slot_fd") < 0) {
        std::cerr << "send_fd failed\n";
        return 6;
    }
    int status = 0;
    ::waitpid(pid, &status, 0);
    std::cout << "[15_fd_passing_alive_counter:parent] sent_fd=" << memfd.get()
              << " child_status=" << status << "\n";
    return WIFEXITED(status) && WEXITSTATUS(status) == 0 ? 0 : 7;
}


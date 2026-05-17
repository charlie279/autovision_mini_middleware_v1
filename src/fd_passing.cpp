/**
 * @file fd_passing.cpp
 * @brief Unix domain socket SCM_RIGHTS fd passing helpers.
 */
#include "fd_passing.hpp"

#include <cstring>
#include <sys/socket.h>
#include <unistd.h>

namespace avm {

int send_fd(int socket_fd, int fd_to_send, const std::string& tag) {
    char data[64]{};
    std::snprintf(data, sizeof(data), "%s", tag.c_str());

    iovec iov{};
    iov.iov_base = data;
    iov.iov_len = std::strlen(data) + 1;

    char control[CMSG_SPACE(sizeof(int))]{};
    msghdr msg{};
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = control;
    msg.msg_controllen = sizeof(control);

    cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));
    std::memcpy(CMSG_DATA(cmsg), &fd_to_send, sizeof(int));

    return ::sendmsg(socket_fd, &msg, 0);
}

int recv_fd(int socket_fd, std::string* tag) {
    char data[64]{};
    iovec iov{};
    iov.iov_base = data;
    iov.iov_len = sizeof(data);

    char control[CMSG_SPACE(sizeof(int))]{};
    msghdr msg{};
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = control;
    msg.msg_controllen = sizeof(control);

    const ssize_t n = ::recvmsg(socket_fd, &msg, 0);
    if (n <= 0) {
        return -1;
    }
    if (tag != nullptr) {
        *tag = std::string(data);
    }
    for (cmsghdr* cmsg = CMSG_FIRSTHDR(&msg); cmsg != nullptr; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
        if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS) {
            int received_fd = -1;
            std::memcpy(&received_fd, CMSG_DATA(cmsg), sizeof(int));
            return received_fd;
        }
    }
    return -1;
}

}  // namespace avm

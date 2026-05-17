/**
 * @file fd_passing.hpp
 * @brief Unix domain socket SCM_RIGHTS fd passing helpers.
 */
#pragma once

#include <string>

namespace avm {

int send_fd(int socket_fd, int fd_to_send, const std::string& tag);
int recv_fd(int socket_fd, std::string* tag);

}  // namespace avm


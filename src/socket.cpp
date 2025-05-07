/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   socket.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pepi <pepi@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/26 17:15:42 by pepi              #+#    #+#             */
/*   Updated: 2025/05/07 11:08:38 by pepi             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Socket.hpp"
#include <unistd.h>

Socket::Socket(int port, const std::string& host) : port_(port), host_(host) {
    create_socket();
    set_non_blocking();
    bind_socket();
    listen_socket();
    std::cout << "Socket created and listening on " << host_ << ":" << port_ << std::endl;
}

Socket::~Socket() {
    if (fd_ >= 0) {
        close(fd_);
    }
}

int Socket::get_fd() const {
    return fd_;
}

void Socket::set_non_blocking() {
    int flags = fcntl(fd_, F_GETFL, 0);
    if (flags == -1) {
        throw std::runtime_error("Failed to get socket flags");
    }
    if (fcntl(fd_, F_SETFL, flags | O_NONBLOCK) == -1) {
        throw std::runtime_error("Failed to set socket non-blocking");
    }
}

void Socket::create_socket() {
    fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_ < 0) {
        throw std::runtime_error("Failed to create socket");
    }
    
    // Permet la réutilisation de l'adresse
    int opt = 1;
    if (setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        throw std::runtime_error("setsockopt failed");
    }
}

void Socket::bind_socket() {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    addr.sin_addr.s_addr = inet_addr(host_.c_str());

    if (bind(fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        throw std::runtime_error("Failed to bind socket");
    }
}

void Socket::listen_socket() {
    if (listen(fd_, SOMAXCONN) < 0) {
        throw std::runtime_error("Failed to listen on socket");
    }
}
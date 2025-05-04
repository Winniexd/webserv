/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/29 13:24:11 by marvin            #+#    #+#             */
/*   Updated: 2025/04/29 13:24:11 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"

Server::Server() {};

Server::~Server() {};

Server::Server(const Server& rhs) {
    *this = rhs;
};

Server& Server::operator=(const Server& src) {
    *this = src;
    return *this;
}

int Server::init() {
    this->fd = socket(AF_INET, SOCK_STREAM, 0);
    if (this->fd == -1) {
        std::cout << "Could not open socket." << std::endl;
        return 1;
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        std::cout << "Could not bind to port" << std::endl;
        return 1;
    }
    if (listen(this->fd, 1000) == -1)
    {
        std::cout << "Could not listen to port" << std::endl;
        return 1;
    }
    return 0;
}

void Server::run() {
    int clientSocket = accept(this->fd, NULL, NULL);
    char buffer[1024] = {0};
    recv(clientSocket, buffer, sizeof(buffer), 0);
    std::cout << "Message from client: " << buffer << std::endl;
    close(this->fd);
}
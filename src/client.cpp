/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/29 13:47:46 by marvin            #+#    #+#             */
/*   Updated: 2025/04/29 13:47:46 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "webserv.hpp"
#include <cstring>

int main() {
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    connect(clientSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
    const char* message = "Hello, server!";
    send(clientSocket, message, strlen(message), 0);
    close(clientSocket);
}
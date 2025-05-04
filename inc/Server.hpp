/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/29 13:23:29 by marvin            #+#    #+#             */
/*   Updated: 2025/04/29 13:23:29 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */
//#pragma once
#ifndef SERVER_HPP
#define SERVER_HPP
#include "webserv.hpp"
#include <netinet/in.h>
#include <arpa/inet.h>

class Server {
    private:
        struct sockaddr_in addr;
        int fd;
    public:
        Server();
        ~Server();
        Server(const Server& rhs);
        Server& operator=(const Server& src);
        int init();
        void run();
};

#endif
/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   socket.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rpepi <rpepi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/26 17:15:42 by pepi              #+#    #+#             */
/*   Updated: 2025/05/12 11:24:36 by rpepi            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Socket.hpp"


// Constructeur de la classe Socket
// Initialise le socket, le configure en mode non-bloquant, le lie à une adresse et commence à écouter
Socket::Socket(int port, const std::string& host) : port_(port), host_(host) {
    create_socket();       // Crée le socket   // Configure le socket en mode non-bloquant
    bind_socket();         // Lie le socket à une adresse et un port
    listen_socket();       // Met le socket en mode écoute
    std::cout << "Socket created and listening on " << host_ << ":" << port_ << std::endl;
}

// Destructeur de la classe Socket
// Ferme le descripteur de fichier du socket s'il est valide
Socket::~Socket() {
    if (fd_ >= 0) {
        close(fd_); // Ferme le socket
    }
}

// Retourne le descripteur de fichier du socket
int Socket::get_fd() const {
    return fd_;
}

// Crée un socket
void Socket::create_socket() {
    fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_ < 0) {
        throw std::runtime_error("Failed to create socket");
    }
    
    // Ajoute le socket principal au poll
    struct pollfd pfd;
    pfd.fd = fd_;
    pfd.events = POLLIN;  // Écoute uniquement les connexions entrantes
    pfd.revents = 0;
    poll_fds_.push_back(pfd);
    
    int opt = 1;
    if (setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        throw std::runtime_error("setsockopt failed");
    }
}

// Lie le socket à une adresse et un port
void Socket::bind_socket() {
    struct sockaddr_in addr; // Structure pour stocker l'adresse
    addr.sin_family = AF_INET; // Utilise IPv4
    addr.sin_port = htons(port_); // Définit le port (converti en big-endian)
    addr.sin_addr.s_addr = inet_addr(host_.c_str()); // Définit l'adresse IP (convertie depuis une chaîne)

    // Lie le socket à l'adresse et au port spécifiés
    if (bind(fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        throw std::runtime_error("Failed to bind socket"); // Erreur si la liaison échoue
    }
}

// Met le socket en mode écoute
void Socket::listen_socket() {
    // Définit le socket pour écouter les connexions entrantes
    if (listen(fd_, SOMAXCONN) < 0) { //SOMAXCONN est une constante qui définit le nombre maximum de connexions en attente (128)
        throw std::runtime_error("Failed to listen on socket"); // Erreur si l'écoute échoue
    }
}

void Socket::add_to_poll(int fd) {
    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN | POLLOUT;  // Surveille lecture et écriture
    pfd.revents = 0;
    poll_fds_.push_back(pfd);
}

void Socket::remove_from_poll(int fd) {
    for (std::vector<struct pollfd>::iterator it = poll_fds_.begin(); 
         it != poll_fds_.end(); ++it) {
        if (it->fd == fd) {
            poll_fds_.erase(it);
            break;
        }
    }
}

int Socket::wait_for_events(int timeout_ms) {
    return poll(&poll_fds_[0], poll_fds_.size(), timeout_ms);
}

bool Socket::can_read(int fd) const {
    for (std::vector<struct pollfd>::const_iterator it = poll_fds_.begin();
         it != poll_fds_.end(); ++it) {
        if (it->fd == fd) {
            return (it->revents & POLLIN);
        }
    }
    return false;
}

bool Socket::can_write(int fd) const {
    for (std::vector<struct pollfd>::const_iterator it = poll_fds_.begin();
         it != poll_fds_.end(); ++it) {
        if (it->fd == fd) {
            return (it->revents & POLLOUT);
        }
    }
    return false;
}
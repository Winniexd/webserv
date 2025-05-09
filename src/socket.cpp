/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   socket.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rpepi <rpepi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/26 17:15:42 by pepi              #+#    #+#             */
/*   Updated: 2025/05/07 12:19:28 by rpepi            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Socket.hpp"


// Constructeur de la classe Socket
// Initialise le socket, le configure en mode non-bloquant, le lie à une adresse et commence à écouter
Socket::Socket(int port, const std::string& host) : port_(port), host_(host) {
    create_socket();       // Crée le socket
    set_non_blocking();    // Configure le socket en mode non-bloquant
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

// Configure le socket en mode non-bloquant
void Socket::set_non_blocking() {
    int flags = 1; // 1 pour activer le mode non-bloquant
    if (ioctl(fd_, FIONBIO, &flags) == -1) {
        throw std::runtime_error("Failed to set socket non-blocking using ioctl");
    }
}

// Crée un socket
void Socket::create_socket() {
    // Crée un socket IPv4 (AF_INET) en mode TCP (SOCK_STREAM)
    fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_ < 0) {
        throw std::runtime_error("Failed to create socket"); // Erreur si la création échoue
    }
    
    // Configure poll
    poll_fd_.fd = fd_;
    poll_fd_.events = POLLIN | POLLOUT;
    poll_fd_.revents = 0;
    
    // Configure l'option SO_REUSEADDR pour permettre la réutilisation de l'adresse
    int opt = 1;
    if (setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        throw std::runtime_error("setsockopt failed"); // Erreur si la configuration échoue
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
    if (listen(fd_, SOMAXCONN) < 0) {
        throw std::runtime_error("Failed to listen on socket"); // Erreur si l'écoute échoue
    }
}

int Socket::wait_for_event(int timeout_ms) {
    return poll(&poll_fd_, 1, timeout_ms);
}

bool Socket::can_read() const {
    return (poll_fd_.revents & POLLIN);
}

bool Socket::can_write() const {
    return (poll_fd_.revents & POLLOUT);
}
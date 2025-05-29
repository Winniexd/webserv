#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <unistd.h>
#include <sys/ioctl.h> // For ioctl function
#include <sys/poll.h>
#include <vector>

class Socket {
public:
    // Constructeur: initialise un socket sur le port et l'hôte spécifiés
    Socket(int port, const std::string& host = "0.0.0.0");
    ~Socket();
    
    // Retourne le descripteur de fichier du socket
    int get_fd() const;
    
    // Configure le socket en mode non-bloquant
    
    // Ajouter ces méthodes
    void add_to_poll(int fd);              // Ajoute un FD à surveiller
    void remove_from_poll(int fd);         // Retire un FD
    int wait_for_events(int timeout_ms);   // Surveille tous les FDs
    bool can_read(int fd) const;           // Vérifie si on peut lire
    bool can_write(int fd) const;          // Vérifie si on peut écrire

private:
    int fd_;          // Descripteur de fichier du socket
    int port_;        // Port d'écoute
    std::string host_; // Adresse IP
    std::vector<struct pollfd> poll_fds_;  // Vector pour tous les FDs à surveiller
    
    // Méthodes privées pour l'initialisation du socket
    void create_socket();   // Crée le socket
    void bind_socket();     // Lie le socket à l'adresse
    void listen_socket();   // Met le socket en mode écoute
};

#endif
#include "../includes/Socket.hpp"
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdlib> // For getenv
#include <sstream> // For stringstream

// Fonction utilitaire pour lire un fichier
std::string read_file(const std::string& path) {
    std::ifstream file(path.c_str());
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + path);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main() {
    try {
        // Initialisation du serveur
        Socket server(8080, "127.0.0.1");
        std::cout << "Server is running on http://127.0.0.1:8080" << std::endl;
        
        // Définition du chemin de base pour les fichiers statiques
        std::string base_path = "/home/rpepi/github/webserv/www";
        
        // Boucle principale du serveur
        while (true) {
            // Attendre un événement avec poll
            int events = server.wait_for_event(1000);
            
            if (events > 0) {
                if (server.can_read()) {
                    // Structure pour stocker l'adresse du client
                    struct sockaddr_in client_addr;
                    socklen_t client_len = sizeof(client_addr);
                    
                    // Accepter une nouvelle connexion
                    int client_fd = accept(server.get_fd(), 
                                         (struct sockaddr*)&client_addr, 
                                         &client_len);
                    
                    if (client_fd > 0) {
                        // SECTION CRITIQUE: Gestion des requêtes client
                        std::cout << "New connection accepted" << std::endl;
                        
                        // Lecture de la requête
                        char buffer[1024] = {0};
                        recv(client_fd, buffer, sizeof(buffer), 0);
                        
                        try {
                            // Lecture et envoi du fichier index.html
                            std::string content = read_file(base_path + "/index.html");
                            
                            // Construction de la réponse HTTP
                            std::stringstream ss;
                            ss << content.length();
                            std::string response = "HTTP/1.1 200 OK\r\n"
                                                 "Content-Type: text/html\r\n"
                                                 "Content-Length: " + 
                                                 ss.str() + 
                                                 "\r\n\r\n" + content;
                            
                            send(client_fd, response.c_str(), response.length(), 0);
                        } catch (const std::exception& e) {
                            // Gestion des erreurs - envoi d'une page 404
                            std::string error_response = "HTTP/1.1 404 Not Found\r\n"
                                                       "Content-Type: text/plain\r\n"
                                                       "Content-Length: 13\r\n\r\n"
                                                       "404 Not Found";
                            send(client_fd, error_response.c_str(), 
                                 error_response.length(), 0);
                        }
                        
                        close(client_fd);
                    }
                }
            }
            
            // Pas besoin de usleep car poll gère déjà le timing
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>


struct Location {
    std::string root;                    // Chemin racine pour les fichiers
    std::vector<std::string> methods;    // Méthodes HTTP autorisées
    std::string index;                   // Fichier index par défaut
    bool upload;                         // Autorise les uploads si true
    std::string cgi_extension;           // Extension pour les fichiers CGI
};


struct ServerConfig {
    std::string host;                    // Adresse d'écoute
    int port;                            // Port d'écoute
    std::string server_name;             // Nom du serveur virtuel
    std::map<int, std::string> error_pages; // Pages d'erreur personnalisées
    size_t client_max_body_size;         // Taille maximale des requêtes
    std::map<std::string, Location> locations; // Configuration des routes
};

// Classe principale de configuration
class Config {
public:
    Config(const std::string& file_path);
    const std::vector<ServerConfig>& get_servers() const;

private:
    std::vector<ServerConfig> servers_;   // Liste des serveurs configurés
    void parse_file(const std::string& file_path); // Parse le fichier de config
};

#endif
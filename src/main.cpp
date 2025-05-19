#include "../includes/Socket.hpp"
#include "../includes/Config.hpp"
#include "../includes/HTTPRequest.hpp"
#include "../includes/cgi.hpp"
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <string>
#include <map>

// Map pour stocker les requêtes en cours de traitement
std::map<int, HTTPRequest> pending_requests;

bool is_cgi_request(const std::string& path, const Location& loc) {
    if (loc.cgi.empty()) return false; //loc.cgi is always empty
    
    // Vérifier si le chemin commence par /cgi-bin/
    if (path.find("/cgi-bin/") != 0) return false;
    
    size_t dot_pos = path.find_last_of('.');
    if (dot_pos == std::string::npos) return false;
    
    std::string extension = path.substr(dot_pos);
    std::cout << "Checking CGI: path=" << path << ", extension=" << extension << ", cgi=" << loc.cgi << std::endl;
    
    // Vérifier si l'extension correspond à celle configurée
    return extension == loc.cgi;
}

std::string handle_cgi_request(const HTTPRequest& request, const std::string& base_path) {
    (void)base_path; // Pour éviter l'avertissement de paramètre non utilisé
    
    Cgi cgi;
    cgi.init_env(request);
    int status = cgi.exec();
    
    if (status != 0) {
        std::stringstream ss;
        ss << "CGI execution failed with status: " << status;
        throw std::runtime_error(ss.str());
    }
    
    // Lire la sortie du CGI
    char buffer[4096];
    std::string output;
    ssize_t bytes;
    
    while ((bytes = read(cgi.get_out_fd(), buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes] = '\0';
        output += buffer;
    }
    
    return output;
}

int main() {
    try {
        Config config("config/default.conf");
        const std::vector<ServerConfig>& servers = config.get_servers();
        
        if (servers.empty()) {
            throw std::runtime_error("No server configurations found");
        }

        const ServerConfig& server_config = servers[0];
        Socket server(server_config.port, server_config.host);
        
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) == NULL) { //La fonction getcwd() copie le chemin d'accès absolu du répertoire de travail courant dans la chaîne pointée par buf, qui est de longueur size. 
            throw std::runtime_error("Cannot get current working directory");
        }

        std::string root_path = server_config.locations.at("/").root;
        if (!root_path.empty() && root_path[root_path.size() - 1] == ';') {
            root_path.erase(root_path.size() - 1);
        }

        std::string base_path = std::string(cwd) + root_path;
        std::cout << "Server running at http://" << server_config.host 
                  << ":" << server_config.port << std::endl;

        std::vector<int> client_fds;

        while (true) {
            // Attendre les événements avec un timeout de 1000ms
            int ret = server.wait_for_events(1000);
            if (ret < 0) {
                throw std::runtime_error("Poll error");
            }

            // Gérer les nouvelles connexions
            int client_fd = server.accept_connection();
            if (client_fd > 0) {
                server.add_to_poll(client_fd);
                client_fds.push_back(client_fd);
                std::cout << "New client connected: " << client_fd << std::endl;
            }
            
            // Gérer les clients existants
            for (std::vector<int>::iterator it = client_fds.begin(); 
                 it != client_fds.end();) {
                int fd = *it;
                bool should_remove = false;
                
                // Lecture des données
                if (server.can_read(fd)) {
                    char buffer[4096] = {0};
                    ssize_t bytes = recv(fd, buffer, sizeof(buffer) - 1, 0);
                    
                    if (bytes <= 0) {
                        should_remove = true;
                    } else {
                        try {
                            // Traiter la requête
                            HTTPRequest request(buffer);
                            std::string method = request.get_method();
                            std::string path = request.get_path();
                            
                            std::cout << "Received " << method << " request for " << path << std::endl;
                            
                            // Vérifier la méthode autorisée
                            std::string location = "/";
                            const Location& loc = server_config.locations.at(location);
                            
                            bool method_allowed = false;
                            for (std::vector<std::string>::const_iterator mit = loc.methods.begin(); 
                                 mit != loc.methods.end(); ++mit) {
                                if (*mit == method) {
                                    method_allowed = true;
                                    break;
                                }
                            }
                            
                            if (!method_allowed) {
                                std::string error = create_error_response(405, "Method Not Allowed");
                                send(fd, error.c_str(), error.length(), 0);
                            } else {
                                // Stocker la requête pour traitement asynchrone
                                pending_requests[fd] = request;
                            }
                        } catch (const std::exception& e) {
                            std::string error = create_error_response(500, "Internal Server Error");
                            send(fd, error.c_str(), error.length(), 0);
                            should_remove = true;
                        }
                    }
                }
                
                // Écriture des réponses
                if (server.can_write(fd) && pending_requests.find(fd) != pending_requests.end()) {
                    HTTPRequest& request = pending_requests[fd];
                    std::string method = request.get_method();
                    std::string path = request.get_path();
                    
                    try {
                        // Trouver la location correspondante
                        std::string location = "/";
                        for (std::map<std::string, Location>::const_iterator it = server_config.locations.begin();
                             it != server_config.locations.end(); ++it) {
                            if (path.find(it->first) == 0 && it != server_config.locations.begin()) {
                                location = it->first;
                                break;
                            }
                        }
                        std::cout << location << std::endl;
                        
                        const Location& loc = server_config.locations.at(location);
                        std::cout << loc.cgi << std::endl;
                        // Vérifier si c'est une requête CGI
                        if (location == "/cgi-bin") { //placeholder until is_cgi_request is fixed
                            std::string output = handle_cgi_request(request, base_path);
                            std::string response = create_http_response(output, "text/html");
                            std::cout << response << std::endl;
                            send(fd, response.c_str(), response.length(), 0);
                        } else {
                            if (method == "GET") {
                                handle_get_request(path, fd, base_path);
                            } else if (method == "POST") {
                                handle_post_request(request, fd, base_path);
                            } else if (method == "DELETE") {
                                handle_delete_request(path, fd, base_path);
                            }
                        }
                        
                        // Nettoyer la requête traitée
                        pending_requests.erase(fd);
                    } catch (const std::exception& e) {
                        std::string error = create_error_response(500, "Internal Server Error");
                        send(fd, error.c_str(), error.length(), 0);
                        should_remove = true;
                    }
                }
                
                // Gérer la déconnexion
                if (should_remove) {
                    server.remove_from_poll(fd);
                    close(fd);
                    pending_requests.erase(fd);
                    it = client_fds.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
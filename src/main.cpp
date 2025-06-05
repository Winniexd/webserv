#include "../includes/Socket.hpp"
#include "../includes/Config.hpp"
#include "../includes/HTTPRequest.hpp"
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <vector>

int main(int argc, char **argv) {
    try {
        std::string config_path = (argc > 1) ? argv[1] : "config/default.conf";
        Config config(config_path.c_str());
        const std::vector<ServerConfig>& servers = config.get_servers();
        if (servers.empty()) throw std::runtime_error("No server configurations found");

        std::vector<Socket*> server_sockets;
        std::vector<int> client_fds;
        std::map<int, size_t> client_to_server; // Associe chaque client_fd à l'index du serveur

        // Crée un socket par port/host
        for (size_t i = 0; i < servers.size(); ++i) {
            server_sockets.push_back(new Socket(servers[i].port, servers[i].host));
            std::cout << "Server running at http://" << servers[i].host << ":" << servers[i].port << std::endl;
        }

        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) == NULL)
            throw std::runtime_error("Cannot get current working directory");

        while (true) {
            // Pour chaque socket serveur, on attend les connexions
            for (size_t i = 0; i < server_sockets.size(); ++i) {
                if (server_sockets[i]->wait_for_events(0) > 0) {
                    if (server_sockets[i]->can_read(server_sockets[i]->get_fd())) {
                        int client_fd = accept(server_sockets[i]->get_fd(), NULL, NULL);
                        if (client_fd > 0) {
                            server_sockets[i]->add_to_poll(client_fd);
                            client_fds.push_back(client_fd);
                            client_to_server[client_fd] = i; // Associe ce client au bon serveur
                        }
                    }
                }
            }
            // Vérifier les clients existants
            for (std::vector<int>::iterator it = client_fds.begin(); it != client_fds.end();) {
                int fd = *it;
                bool handled = false;
                for (size_t i = 0; i < server_sockets.size(); ++i) {
                    if (server_sockets[i]->can_read(fd)) {
                        char buffer[4096] = {0};
                        ssize_t bytes = recv(fd, buffer, sizeof(buffer), 0);
                        if (bytes <= 0) {
                            server_sockets[i]->remove_from_poll(fd);
                            close(fd);
                            client_to_server.erase(fd);
                            it = client_fds.erase(it);
                            handled = true;
                            break;
                        }
                        try {
                            size_t server_idx = client_to_server[fd];
                            const ServerConfig& server_conf = servers[server_idx];

                            HTTPRequest request(buffer, fd, server_conf);
                            std::string method = request.get_method();
                            std::string path = request.get_path();


                          
                            std::string location = "/";
                            for (std::map<std::string, Location>::const_iterator it = server_conf.locations.begin(); it != server_conf.locations.end(); it++) {
                                std::string loc_path = it->first;
                                if (path.find(loc_path) == 0) {
                                    if (loc_path.size() > location.size())
                                        location = loc_path;
                                }
                            }
                            const Location& loc = server_conf.locations.at(location);

                            bool method_allowed = false;
                            for (std::vector<std::string>::const_iterator mit = loc.methods.begin();
                                 mit != loc.methods.end(); ++mit) {
                                if (*mit == method) {
                                    method_allowed = true;
                                    break;
                                }
                            }

                            std::string root_path = loc.root;
                           
                            if (!root_path.empty() && root_path[root_path.size() - 1] == ';')
                                root_path.erase(root_path.size() - 1);
                            std::string base_path = std::string(cwd) + root_path;
                            if (!method_allowed) {
                                std::string error = create_error_response(405, "Method Not Allowed", server_conf);
                                send(fd, error.c_str(), error.length(), 0);
                            }
                            else request.handle_request(base_path, location);
                            // Après avoir répondu, ferme la connexion et retire le client :
                            server_sockets[i]->remove_from_poll(fd);
                            close(fd);
                            client_to_server.erase(fd);
                            it = client_fds.erase(it);
                            handled = true;
                            break;
                        }
                        catch (const std::exception& e) {
                            server_sockets[i]->remove_from_poll(fd);
                            close(fd);
                            client_to_server.erase(fd);
                            it = client_fds.erase(it);
                            handled = true;
                            break;
                        }
                    }
                }
                if (!handled)
                    ++it;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
#include "../includes/Socket.hpp"
#include "../includes/Config.hpp"
#include "../includes/HTTPRequest.hpp"
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <string>


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
        if (getcwd(cwd, sizeof(cwd)) == NULL) {
            throw std::runtime_error("Cannot get current working directory");
        }

        std::string root_path = server_config.locations.at("/").root;
        if (!root_path.empty() && root_path[root_path.size() - 1] == ';') {
            root_path.erase(root_path.size() - 1);
        }

        std::string base_path = std::string(cwd) + root_path;
        std::cout << "Server running at http://" << server_config.host 
                  << ":" << server_config.port << std::endl;

        while (true) {
            if (server.wait_for_event(1000) > 0 && server.can_read()) {
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                
                int client_fd = accept(server.get_fd(), 
                                     (struct sockaddr*)&client_addr, 
                                     &client_len);
                
                if (client_fd > 0) {
                    char buffer[4096] = {0};
                    recv(client_fd, buffer, sizeof(buffer), 0);
                    
                    try {
                        HTTPRequest request(buffer);
                        std::string method = request.get_method();
                        std::string path = request.get_path();
                        
                        // Add debug output
                        std::cout << "Received " << method << " request for " << path << std::endl;
                        
                        // Check if method is allowed for this location
                        std::string location = "/";
                        const Location& loc = server_config.locations.at(location);
                        
                        // Debug output for allowed methods
                        std::cout << "Allowed methods: ";
                        std::vector<std::string>::const_iterator debug_it;
                        for (debug_it = loc.methods.begin(); debug_it != loc.methods.end(); ++debug_it) {
                            std::cout << *debug_it << " ";
                        }
                        std::cout << std::endl;
                        
                        bool method_allowed = false;
                        std::vector<std::string>::const_iterator it;
                        for (it = loc.methods.begin(); it != loc.methods.end(); ++it) {
                            if (*it == method) {
                                method_allowed = true;
                                break;
                            }
                        }
                        
                        if (!method_allowed) {
                            std::string error = create_error_response(405, "Method Not Allowed");
                            send(client_fd, error.c_str(), error.length(), 0);
                        }
                        else if (method == "GET") {
                            handle_get_request(path, client_fd, base_path);
                        }
                        else if (method == "POST") {
                            handle_post_request(request, client_fd, base_path);
                        }
                        else if (method == "DELETE") {
                            handle_delete_request(path, client_fd, base_path);
                        }
                        else {
                            std::string error = create_error_response(501, "Not Implemented");
                            send(client_fd, error.c_str(), error.length(), 0);
                        }
                    }
                    catch (const std::exception& e) {
                        std::string error = create_error_response(500, "Internal Server Error");
                        send(client_fd, error.c_str(), error.length(), 0);
                    }
                    close(client_fd);
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
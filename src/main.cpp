#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <stdexcept>
#include <unistd.h>
#include <sys/socket.h>
#include <signal.h>
#include "../includes/Socket.hpp"
#include "../includes/Config.hpp"
#include "../includes/HTTPRequest.hpp"

// Global variable to control server shutdown
static volatile int keep_running = 1;

// Signal handler for SIGINT (Ctrl+C)
void signal_handler(int /*sig*/) {
    keep_running = 0;
}

// Initializes configuration from the specified file
Config initialize_config(int argc, char **argv) {
    std::string config_path = (argc > 1) ? argv[1] : "config/default.conf";
    Config config(config_path.c_str());
    if (config.get_servers().empty()) {
        throw std::runtime_error("No server configurations found");
    }
    return config;
}

// Creates sockets for each configured server
std::vector<Socket*> create_server_sockets(const std::vector<ServerConfig>& servers) {
    std::vector<Socket*> server_sockets;
    for (std::vector<ServerConfig>::size_type i = 0; i < servers.size(); ++i) {
        server_sockets.push_back(new Socket(servers[i].port, servers[i].host));
        std::cout << "Server running at http://" << servers[i].host << ":" << servers[i].port << std::endl;
    }
    return server_sockets;
}

// Retrieves the current working directory
std::string get_current_working_directory() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        throw std::runtime_error("Cannot get current working directory");
    }
    return std::string(cwd);
}

// Accepts incoming connections for a server socket
void accept_new_connections(Socket* socket, std::vector<ServerConfig>::size_type server_index, 
                           std::vector<int>& client_fds, 
                           std::map<int, std::vector<ServerConfig>::size_type>& client_to_server) {
    if (socket->can_read(socket->get_fd())) {
        int client_fd = accept(socket->get_fd(), NULL, NULL);
        if (client_fd > 0) {
            socket->add_to_poll(client_fd);
            client_fds.push_back(client_fd);
            client_to_server[client_fd] = server_index;
        }
    }
}

// Closes and cleans up a client socket
void cleanup_client(int client_fd, Socket* socket, 
                   std::vector<int>& client_fds, 
                   std::map<int, std::vector<ServerConfig>::size_type>& client_to_server,
                   std::vector<int>::iterator& it) {
    socket->remove_from_poll(client_fd);
    close(client_fd);
    client_to_server.erase(client_fd);
    it = client_fds.erase(it);
}

// Finds the location matching the request path
std::string find_matching_location(const std::string& path, 
                                  const std::map<std::string, Location>& locations) {
    std::string location = "/";
    for (std::map<std::string, Location>::const_iterator it = locations.begin(); 
         it != locations.end(); ++it) {
        std::string loc_path = it->first;
        if (path.find(loc_path) == 0 && loc_path.size() > location.size()) {
            location = loc_path;
        }
    }
    return location;
}

// Checks if the HTTP method is allowed for the location
bool is_method_allowed(const std::string& method, const Location& loc) {
    for (std::vector<std::string>::const_iterator mit = loc.methods.begin();
         mit != loc.methods.end(); ++mit) {
        if (*mit == method) {
            return true;
        }
    }
    return false;
}

// Builds the base path for serving files
std::string build_base_path(const std::string& cwd, std::string root_path) {
    if (!root_path.empty() && root_path[root_path.size() - 1] == ';') {
        root_path.erase(root_path.size() - 1);
    }
    return cwd + root_path;
}

// Processes an HTTP request from a client
void process_client_request(int client_fd, Socket* socket, 
                          const std::vector<ServerConfig>& servers,
                          std::map<int, std::vector<ServerConfig>::size_type>& client_to_server,
                          std::vector<int>& client_fds, 
                          const std::string& cwd,
                          std::vector<int>::iterator& it) {
    char buffer[4096] = {0};
    ssize_t bytes = recv(client_fd, buffer, sizeof(buffer), 0);
    if (bytes <= 0) {
        cleanup_client(client_fd, socket, client_fds, client_to_server, it);
        return;
    }

    try {
        std::map<int, std::vector<ServerConfig>::size_type>::iterator server_it = client_to_server.find(client_fd);
        if (server_it == client_to_server.end()) {
            cleanup_client(client_fd, socket, client_fds, client_to_server, it);
            return;
        }
        std::vector<ServerConfig>::size_type server_idx = server_it->second;
        const ServerConfig& server_conf = servers[server_idx];
        HTTPRequest request(buffer, client_fd, server_conf);
        std::string method = request.get_method();
        std::string path = request.get_path();

        std::string location = find_matching_location(path, server_conf.locations);
        const Location& loc = server_conf.locations.at(location);

        if (!is_method_allowed(method, loc)) {
            std::string error = create_error_response(405, "Method Not Allowed", server_conf);
            // Only send if can_write
            if (socket->can_write(client_fd)) {
                send(client_fd, error.c_str(), error.length(), 0);
            }
        } else {
            std::string base_path = build_base_path(cwd, loc.root);
            // You must ensure handle_request only writes after can_write
            if (socket->can_write(client_fd)) {
                request.handle_request(base_path, location, socket);
            }
        }
        cleanup_client(client_fd, socket, client_fds, client_to_server, it);
    } catch (const std::exception& e) {
        cleanup_client(client_fd, socket, client_fds, client_to_server, it);
    }
}

// Main loop to handle connections and requests
void run_server_loop(const std::vector<ServerConfig>& servers, 
                    std::vector<Socket*>& server_sockets, 
                    const std::string& cwd) {
    std::vector<int> client_fds;
    std::map<int, std::vector<ServerConfig>::size_type> client_to_server;
    
    while (keep_running) {
        // Wait for events on all fds (listening + clients) for each server
        for (std::vector<Socket*>::size_type i = 0; i < server_sockets.size(); ++i) {
            server_sockets[i]->wait_for_events(0);
        }

        // Accept new connections
        for (std::vector<Socket*>::size_type i = 0; i < server_sockets.size(); ++i) {
            accept_new_connections(server_sockets[i], i, client_fds, client_to_server);
        }

        // Process existing clients 
        for (std::vector<int>::iterator it = client_fds.begin(); it != client_fds.end();) {
            int client_fd = *it;
            bool handled = false;
            for (std::vector<Socket*>::size_type i = 0; i < server_sockets.size(); ++i) {
                if (server_sockets[i]->can_read(client_fd)) {
                    process_client_request(client_fd, server_sockets[i], servers, 
                                         client_to_server, client_fds, cwd, it);
                    handled = true;
                    break;
                }
            }
            if (!handled) {
                ++it;
            }
        }
    }
}

// Cleans up server sockets
void cleanup_server_sockets(std::vector<Socket*>& server_sockets) {
    for (std::vector<Socket*>::size_type i = 0; i < server_sockets.size(); ++i) {
        delete server_sockets[i];
    }
    server_sockets.clear();
}

int main(int argc, char **argv) {
    // Register signal handler for SIGINT
    signal(SIGINT, signal_handler);

    try {
        Config config = initialize_config(argc, argv);
        const std::vector<ServerConfig>& servers = config.get_servers();
        std::vector<Socket*> server_sockets = create_server_sockets(servers);
        std::string cwd = get_current_working_directory();
        run_server_loop(servers, server_sockets, cwd);
        cleanup_server_sockets(server_sockets);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}

/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Config.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pepi <pepi@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/26 14:03:44 by pepi              #+#    #+#             */
/*   Updated: 2025/04/28 15:47:39 by pepi             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Config.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

//constructeur par def
Config::Config(const std::string& file_path) {
    parse_file(file_path);
}

//fonction get servers
const std::vector<ServerConfig>& Config::get_servers() const {
    return servers_;
}

//fonction parse file
void Config::parse_file(const std::string& file_path) {
    std::ifstream file(file_path.c_str());
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open config file: " << file_path << std::endl;
        exit(1);
    }

    std::string line;
    ServerConfig current_server;
    Location current_location;
    std::string current_block;

    while (std::getline(file, line)) {
        // Supprimer les espaces et commentaires
        size_t comment_pos = line.find('#');
        if (comment_pos != std::string::npos)
            line = line.substr(0, comment_pos);
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);

        if (line.empty())
            continue;

        // Détecter les blocs
        if (line.find("server {") != std::string::npos) {
            current_block = "server";
            current_server = ServerConfig();
            continue;
        }
        if (line.find("location") != std::string::npos) {
            current_block = "location";
            std::string path = line.substr(line.find("location") + 9, line.find("{") - 9);
            path.erase(0, path.find_first_not_of(" \t"));
            path.erase(path.find_last_not_of(" \t") + 1);
            current_location = Location();
            current_server.locations[path] = current_location;
            continue;
        }
        if (line == "}") {
            if (current_block == "server") {
                servers_.push_back(current_server);
                current_block.clear();
            } else if (current_block == "location") {
                current_block = "server";
            }
            continue;
        }

        // Parser les directives
        std::istringstream iss(line); // transforme la ligne en flux
        std::string key;
        iss >> key;

        if (current_block == "server") {
            if (key == "listen") {
                iss >> current_server.port;
            } else if (key == "host") {
                std::string host;
                iss >> host;
                // Supprimer le point-virgule s'il existe
                if (!host.empty() && host.back() == ';') {
                    host.pop_back();
                }
                current_server.host = host;
            } else if (key == "server_name") {
                iss >> current_server.server_name;
            } else if (key == "error_page") {
                int code;
                std::string path;
                iss >> code >> path;
                current_server.error_pages[code] = path;
            } else if (key == "client_max_body_size") {
                iss >> current_server.client_max_body_size;
            }
        } else if (current_block == "location") {
            std::string path = current_server.locations.rbegin()->first;
            Location& loc = current_server.locations[path];
            if (key == "root") {
                iss >> loc.root;
            } else if (key == "index") {
                iss >> loc.index;
            } else if (key == "methods") {
                std::string method;
                while (iss >> method) {
                    loc.methods.push_back(method);
                }
            } else if (key == "upload") {
                std::string upload;
                iss >> upload;
                loc.upload = (upload == "on");
            } else if (key == "cgi") {
                iss >> loc.cgi_extension;
            }
        }
    }

    file.close();
}
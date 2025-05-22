/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Config.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: winniexd <winniexd@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/26 14:03:44 by pepi              #+#    #+#             */
/*   Updated: 2025/05/22 12:41:35 by winniexd         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Config.hpp"

Config::Config(const std::string& file_path) {
    parse_file(file_path);
}

const std::vector<ServerConfig>& Config::get_servers() const {
    return servers_;
}

void Config::parse_file(const std::string& file_path) {
    std::ifstream file(file_path.c_str());
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open config file: " + file_path);
    }

    std::string line;
    ServerConfig current_server;
    Location current_location;
    std::string current_block;
    std::string current_location_path;

    while (std::getline(file, line)) {
        size_t comment_pos = line.find('#');
        if (comment_pos != std::string::npos)
            line = line.substr(0, comment_pos);
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);

        if (line.empty())
            continue;

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
            current_location_path = path;
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

        std::istringstream iss(line);
        std::string key;
        iss >> key;

        if (current_block == "server") {
            if (key == "listen") {
                iss >> current_server.port;
            } else if (key == "host") {
                std::string host;
                iss >> host;
                if (!host.empty() && host[host.size() - 1] == ';') {
                    host.erase(host.size() - 1);
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
            Location& loc = current_server.locations[current_location_path];
            if (key == "root") {
                std::string root;
                iss >> root;
                if (!root.empty() && root[root.size() - 1] == ';') {
                    root.erase(root.size() - 1);
                }
                loc.root = root;
            } else if (key == "index") {
                iss >> loc.index;
            } else if (key == "methods") {
                std::string method;
                while (iss >> method) {
                    if (!method.empty() && method[method.size() - 1] == ';') {
                        method.erase(method.size() - 1);
                    }
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
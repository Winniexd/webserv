#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <string>
#include <map>
#include <sstream>
#include "Config.hpp"

class HTTPRequest {
public:
    HTTPRequest(const std::string& raw_request, int fd, const ServerConfig& server_conf);
    
    std::string get_method() const;
    std::string get_path() const;
    std::string get_version() const;
    std::string get_header(const std::string& key) const;
    std::string get_body() const;
    void handle_request(std::string base_path, std::string location);
    void handle_cgi_request();
    void handle_get_request(const std::string& base_path);
    void handle_post_request(const std::string& base_path);
    void handle_delete_request(const std::string& base_path);

private:
    std::string method_;
    std::string path_;
    std::string version_;
    std::map<std::string, std::string> headers_;
    std::string body_;
    int fd;
    ServerConfig server_conf;

    void parse_request(const std::string& raw_request);
};


std::string read_file(const std::string& path);
std::string create_http_response(const std::string& content, const std::string& content_type = "text/html");
std::string create_error_response(int status_code, const std::string& message, const ServerConfig& server_conf);
void handle_get_request(const std::string& path, int client_fd, const std::string& base_path, const ServerConfig& server_conf);
void handle_post_request(const HTTPRequest& request, int client_fd, const std::string& base_path, const ServerConfig& server_conf);
void handle_delete_request(const std::string& path, int client_fd, const std::string& base_path, const ServerConfig& server_conf);
void handle_cgi_request(const HTTPRequest &request, int client_fd, const ServerConfig& server_conf);
std::string find_uploaded_file(const std::string& filename, const std::string& base_path);

#endif
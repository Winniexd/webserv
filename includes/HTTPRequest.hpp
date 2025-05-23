#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <string>
#include <map>
#include <sstream>

class HTTPRequest {
public:
    HTTPRequest(const std::string& raw_request);
    
    std::string get_method() const;
    std::string get_path() const;
    std::string get_version() const;
    std::string get_header(const std::string& key) const;
    std::string get_body() const;

private:
    std::string method_;
    std::string path_;
    std::string version_;
    std::map<std::string, std::string> headers_;
    std::string body_;
    
    void parse_request(const std::string& raw_request);
};


std::string read_file(const std::string& path);
std::string create_http_response(const std::string& content, const std::string& content_type = "text/html");
std::string create_error_response(int status_code, const std::string& message);
void handle_get_request(const std::string& path, int client_fd, const std::string& base_path);
void handle_post_request(const HTTPRequest& request, int client_fd, const std::string& base_path);
void handle_delete_request(const std::string& path, int client_fd, const std::string& base_path);
void handle_cgi_request(const HTTPRequest &request, int client_fd);
std::string find_uploaded_file(const std::string& filename, const std::string& base_path);

#endif
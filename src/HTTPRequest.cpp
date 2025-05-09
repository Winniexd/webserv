#include "../includes/HTTPRequest.hpp"
#include "../includes/Socket.hpp"
#include "../includes/Config.hpp"

HTTPRequest::HTTPRequest(const std::string& raw_request) {
    parse_request(raw_request);
}

std::string HTTPRequest::get_method() const {
    return method_;
}

std::string HTTPRequest::get_path() const {
    return path_;
}

std::string HTTPRequest::get_version() const {
    return version_;
}

std::string HTTPRequest::get_header(const std::string& key) const {
    std::map<std::string, std::string>::const_iterator it = headers_.find(key);
    return it != headers_.end() ? it->second : "";
}

std::string HTTPRequest::get_body() const {
    return body_;
}

void HTTPRequest::parse_request(const std::string& raw_request) {
    std::istringstream stream(raw_request);
    std::string line;

    // Parse request line
    std::getline(stream, line);
    std::istringstream request_line(line);
    request_line >> method_ >> path_ >> version_;

    // Parse headers
    while (std::getline(stream, line) && line != "\r") {
        size_t colon = line.find(':');
        if (colon != std::string::npos) {
            std::string key = line.substr(0, colon);
            std::string value = line.substr(colon + 2);
            headers_[key] = value;
        }
    }

    // Parse body
    while (std::getline(stream, line)) {
        body_ += line + "\n";
    }
}

std::string read_file(const std::string& path) {
    std::ifstream file(path.c_str());
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + path);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string create_http_response(const std::string& content, const std::string& content_type) {
    std::stringstream response;
    response << "HTTP/1.1 200 OK\r\n"
             << "Content-Type: " << content_type << "\r\n"
             << "Content-Length: " << content.length() << "\r\n"
             << "\r\n"
             << content;
    return response.str();
}

std::string create_error_response(int status_code, const std::string& message) {
    std::stringstream response;
    response << "HTTP/1.1 " << status_code << " " << message << "\r\n"
             << "Content-Type: text/plain\r\n"
             << "Content-Length: " << message.length() << "\r\n"
             << "\r\n"
             << message;
    return response.str();
}

void handle_get_request(const std::string& path, int client_fd, const std::string& base_path) {
    try {
        std::string file_path = base_path + (path == "/" ? "/index.html" : path);
        std::string content = read_file(file_path);
        std::string response = create_http_response(content);
        send(client_fd, response.c_str(), response.length(), 0);
    } 
    catch (const std::exception&) {
        std::string error = create_error_response(404, "404 Not Found");
        send(client_fd, error.c_str(), error.length(), 0);
    }
}

void handle_post_request(const HTTPRequest& request, int client_fd, const std::string& base_path) {
    try {
        std::string upload_path = base_path + request.get_path();
        std::ofstream file(upload_path.c_str());
        if (!file) {
            throw std::runtime_error("Cannot create file");
        }
        file << request.get_body();
        std::string response = create_http_response("File uploaded successfully", "text/plain");
        send(client_fd, response.c_str(), response.length(), 0);
    }
    catch (const std::exception&) {
        std::string error = create_error_response(500, "Internal Server Error");
        send(client_fd, error.c_str(), error.length(), 0);
    }
}

void handle_delete_request(const std::string& path, int client_fd, const std::string& base_path) {
    try {
        std::string file_path = base_path + path;
        if (remove(file_path.c_str()) != 0) {
            throw std::runtime_error("Cannot delete file");
        }
        std::string response = create_http_response("File deleted successfully", "text/plain");
        send(client_fd, response.c_str(), response.length(), 0);
    }
    catch (const std::exception&) {
        std::string error = create_error_response(404, "404 Not Found");
        send(client_fd, error.c_str(), error.length(), 0);
    }
}

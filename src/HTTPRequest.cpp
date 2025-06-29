#include "../includes/Config.hpp"
#include "../includes/HTTPRequest.hpp"
#include "../includes/Socket.hpp"
#include "../includes/MIME.hpp"
#include "../includes/cgi.hpp"
#include <sys/stat.h> // pour mkdir
#include <dirent.h> // pour parcourir les dossiers

static std::string safe_mime_dir(const std::string& mime) {
    std::string dir = mime;
    size_t i;
    for (i = 0; i < dir.size(); ++i) {
        if (dir[i] == '/')
            dir[i] = '_';
    }
    return dir;
}

//find path of file recursively 
std::string find_uploaded_file(const std::string& filename, const std::string& base_path) {
    DIR* dir = opendir(base_path.c_str());
    if (!dir)
        return "";
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) { //DT_DIR for directories
            std::string name = entry->d_name;
            if (name != "." && name != "..") {
                std::string found = find_uploaded_file(filename, base_path + "/" + name);
                if (!found.empty()) {
                    closedir(dir);
                    return found;
                }
            }
        } else if (entry->d_type == DT_REG) { // DT_REG for regular files
            if (filename == entry->d_name) {
                closedir(dir);
                return base_path + "/" + filename;
            }
        }
    }
    closedir(dir);
    return "";
}

// Parses raw HTTP request string into structured data
HTTPRequest::HTTPRequest(const std::string& raw_request, int fd, const ServerConfig& server_conf) {
    this->fd = fd;
    this->server_conf = server_conf;
    handled = false;
    parse_request(raw_request);
}

// Access methods for HTTP request components
std::string HTTPRequest::get_method() const {
    return method_;
}

std::string HTTPRequest::get_path() const {
    return path_;
}

std::string HTTPRequest::get_version() const {
    return version_;
}

// Retrieves header value by key, returns empty string if not found
std::string HTTPRequest::get_header(const std::string& key) const {
    std::map<std::string, std::string>::const_iterator it = headers_.find(key);
    return it != headers_.end() ? it->second : "";
}

// Returns request body content
std::string HTTPRequest::get_body() const {
    return body_;
}

std::string HTTPRequest::get_response() const {
    return response;
}

bool HTTPRequest::get_handled() const {
    return handled;
}

void HTTPRequest::set_response(const std::string& response) {
    this->response = response;
}

void HTTPRequest::set_handled(const bool handled) {
    this->handled = handled;
}

// Parses HTTP request following the format:
// METHOD PATH HTTP/VERSION
// Header1: Value1
// Header2: Value2
// <empty line>
// body content
void HTTPRequest::parse_request(const std::string& raw_request) {
    std::istringstream stream(raw_request);
    std::string line;

    // First line contains method, path, and HTTP version
    std::getline(stream, line);
    std::istringstream request_line(line);
    request_line >> method_ >> path_ >> version_;

    // Headers are key-value pairs separated by colon
    while (std::getline(stream, line) && line != "\r") {
        size_t colon = line.find(':');
        if (colon != std::string::npos) {
            std::string key = line.substr(0, colon);
            std::string value = line.substr(colon + 2);  // +2 to skip ': '
            headers_[key] = value;
        }
    }

    // Everything after empty line is body content
    while (std::getline(stream, line)) {
        body_ += line + "\n";
    }
}

// Utility function to read file contents safely
std::string read_file(const std::string& path) {
    std::ifstream file(path.c_str());
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + path);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Creates a success (200) HTTP response with content
std::string create_http_response(const std::string& content, const std::string& content_type) {
    std::stringstream response;
    response << "HTTP/1.1 200 OK\r\n"
             << "Content-Type: " << content_type << "\r\n"
             << "Content-Length: " << content.length() << "\r\n"
             << "\r\n"
             << content;
    return response.str();
}

// Creates an error HTTP response with custom status code
std::string create_error_response(int status_code, const std::string& message, const ServerConfig& server_conf) {
    try {
        char buffer[PATH_MAX];
        getcwd(buffer, PATH_MAX);
        std::string filepath = std::string(buffer) + server_conf.error_pages.at(status_code);
        std::ifstream error_file(filepath.c_str());
        if (!error_file.is_open()) {
            throw std::runtime_error("Failed to open error file: " + filepath);
        }
        std::string line, content;
        while (getline(error_file, line))
            content += line;

        std::stringstream response;
        response << "HTTP/1.1 " << status_code << " " << message << "\r\n"
                 << "Content-Type: text/html\r\n"
                 << "Content-Length: " << content.size() << "\r\n"
                 << "\r\n"
                 << content;

        return response.str();
    } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
        std::stringstream response;
        std::ostringstream oss;
        oss << "Error " << status_code << ": " << message;
        std::string fallback = oss.str();
        response << "HTTP/1.1 " << status_code << " " << message << "\r\n"
                 << "Content-Type: text/plain\r\n"
                 << "Content-Length: " << fallback.length() << "\r\n"
                 << "\r\n"
                 << fallback;
        return response.str();
    }
}

// Handles GET requests:
// - Serves index.html for root path (/)
// - Serves requested file for other paths
// - Returns 404 if file not found
void HTTPRequest::handle_get_request(const std::string& base_path) {
    try {
        std::string filename = (path_ == "/" ? "index.html" : path_.substr(path_.find_last_of('/') + 1));
        std::string file_path;

        if (path_.find("/upload") == 0 || path_.find("/uploads") == 0) {
            file_path = find_uploaded_file(filename, base_path);
            if (file_path.empty())
                throw std::runtime_error("Not found");
        } else {
            file_path = base_path + (path_ == "/" ? "/index.html" : path_);
        }

        std::string content = read_file(file_path);
        std::string mime_type = MIME::get_type(file_path);
        response = create_http_response(content, mime_type);
        handled = true;
    }
    catch (const std::exception&) {
        response = create_error_response(404, "404 Not Found", server_conf);
        handled = true;
    }
}

void HTTPRequest::handle_post_request(const std::string& base_path) {
    try {
        // Check body size against limit
        if (body_.length() > server_conf.client_max_body_size) {
            response = create_error_response(413, "Payload Too Large", server_conf);
            handled = true;
            return;
        }

        std::string::size_type slash = path_.find_last_of('/');
        std::string filename = (slash != std::string::npos) ? path_.substr(slash + 1) : path_;
        std::string mime = MIME::get_type(filename);
        std::string subdir = safe_mime_dir(mime);
        std::string upload_dir = base_path + "/" + subdir;
        mkdir(upload_dir.c_str(), 0777);
        std::string upload_path = upload_dir + "/" + filename;
        std::cout << upload_path << std::endl;
        std::ofstream file(upload_path.c_str(), std::ios::binary);
        if (!file)
            throw std::runtime_error("Cannot create file");
        file << body_;
        file.close();

        response = create_http_response("File uploaded successfully", "text/plain");
        handled = true;
    }
    catch (const std::exception&) {
        response = create_error_response(500, "Internal Server Error", server_conf);
        handled = true;
    }
}

void HTTPRequest::handle_delete_request(const std::string& base_path) {
    try {
        std::string filename = path_.substr(path_.find_last_of('/') + 1);
        std::string file_path = find_uploaded_file(filename, base_path);
        if (file_path.empty() || remove(file_path.c_str()) != 0) {
            throw std::runtime_error("Cannot delete file");
        }
        response = create_http_response("File deleted successfully", "text/plain");
        handled = true;
    }
    catch (const std::exception&) {
        response = create_error_response(404, "404 Not Found", server_conf);
        handled = true;
    }
}

void HTTPRequest::handle_cgi_request() {
    Cgi cgi;
    int cgi_fd = -1;
    try {
        if (cgi.init_env(*this))
            throw std::runtime_error("CGI Init Failed");

        if (cgi.exec())
            throw std::runtime_error("CGI Exec Failed");

        cgi_fd = cgi.get_out_fd();
        if (cgi_fd < 0)
            throw std::runtime_error("Invalid CGI output file descriptor");

        std::string output;
        char buffer[4096];
        ssize_t bytes_read;

        while ((bytes_read = read(cgi_fd, buffer, sizeof(buffer))) > 0) {
            output.append(buffer, bytes_read);
        }

        if (bytes_read < 0) {
            throw std::runtime_error("Error reading from CGI pipe");
        }
        close(cgi_fd);

        if (output.empty())
            throw std::runtime_error("CGI output is empty");
        else
            response = create_http_response(output, "text/html");

        handled = true;
    } catch (const std::exception& e) {
        if (cgi_fd >= 0)
            close(cgi_fd);
        std::cerr << e.what() << std::endl;
        response = create_error_response(500, "Internal Server Error", server_conf);
        handled = true;
    }
}

void HTTPRequest::handle_request(std::string base_path, std::string location) {
    if (location == "/cgi-bin") {
        handle_cgi_request();
    }
    else if (method_ == "GET") {
        handle_get_request(base_path);
    }
    else if (method_ == "POST") {
        handle_post_request(base_path);
    }
    else if (method_ == "DELETE") {
        handle_delete_request(base_path);
    }
    else {
        response = create_error_response(501, "Not Implemented", server_conf);
        handled = true;
    }
}
#include "../includes/HTTPRequest.hpp"
#include "../includes/Socket.hpp"
#include "../includes/Config.hpp"
#include "../includes/MIME.hpp"
#include <sys/stat.h> // pour mkdir
#include <dirent.h> // pour parcourir les dossiers

// Parses raw HTTP request string into structured data
HTTPRequest::HTTPRequest(const std::string& raw_request) {
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
std::string create_error_response(int status_code, const std::string& message) {
    std::stringstream response;
    response << "HTTP/1.1 " << status_code << " " << message << "\r\n"
             << "Content-Type: text/plain\r\n"
             << "Content-Length: " << message.length() << "\r\n"
             << "\r\n"
             << message;
    return response.str();
}

// Handles GET requests:
// - Serves index.html for root path (/)
// - Serves requested file for other paths
// - Returns 404 if file not found
void handle_get_request(const std::string& path, int client_fd, const std::string& base_path) {
    try {
        std::string filename = (path == "/" ? "index.html" : path.substr(path.find_last_of('/') + 1));
        std::string file_path;

        // Si on est dans le dossier d'upload, cherche dans les sous-dossiers MIME
        if (path.find("/upload") == 0 || path.find("/uploads") == 0) {
            file_path = find_uploaded_file(filename, base_path);
            if (file_path.empty())
                throw std::runtime_error("Not found");
        } else {
            file_path = base_path + (path == "/" ? "/index.html" : path);
        }

        std::string content = read_file(file_path);
        std::string mime_type = MIME::get_type(file_path);
        std::string response = create_http_response(content, mime_type);
        send(client_fd, response.c_str(), response.length(), 0);
    } 
    catch (const std::exception&) {
        std::string error = create_error_response(404, "404 Not Found");
        send(client_fd, error.c_str(), error.length(), 0);
    }
}

// Remplace les '/' par '_' pour les noms de dossiers valides (C++98)
static std::string safe_mime_dir(const std::string& mime) {
    std::string dir = mime;
    for (std::string::size_type i = 0; i < dir.size(); ++i)
        if (dir[i] == '/')
            dir[i] = '_';
    return dir;
}

// Cherche le fichier dans tous les sous-dossiers MIME de base_path
std::string find_uploaded_file(const std::string& filename, const std::string& base_path) {
    DIR* dir = opendir(base_path.c_str());
    if (!dir) return "";

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        // Ignore . et ..
        if (entry->d_type == DT_DIR && std::string(entry->d_name) != "." && std::string(entry->d_name) != "..") {
            std::string subdir = base_path + "/" + entry->d_name;
            std::string candidate = subdir + "/" + filename;
            std::ifstream file(candidate.c_str(), std::ios::binary);
            if (file.is_open()) {
                closedir(dir);
                return candidate;
            }
        }
    }
    closedir(dir);
    return ""; // Not found
}

// Handles POST requests:
// - Creates or overwrites file at specified path
// - Uses request body as file content
// - Returns 500 if file creation fails
void handle_post_request(const HTTPRequest& request, int client_fd, const std::string& base_path) {
    try {
        // Récupère le nom du fichier à partir du path
        std::string path = request.get_path();
        std::string::size_type slash = path.find_last_of('/');
        std::string filename = (slash != std::string::npos) ? path.substr(slash + 1) : path;

        // Détermine le type MIME
        std::string mime = MIME::get_type(filename);

        // Crée un sous-dossier pour chaque type MIME (ex: image_jpeg)
        std::string subdir = safe_mime_dir(mime);
        std::string upload_dir = base_path + "/" + subdir;

        // Crée le dossier s'il n'existe pas
        mkdir(upload_dir.c_str(), 0777);

        // Chemin complet du fichier à écrire
        std::string upload_path = upload_dir + "/" + filename;

        // Écrit le fichier uploadé
        std::ofstream file(upload_path.c_str(), std::ios::binary);
        if (!file)
            throw std::runtime_error("Cannot create file");
        file << request.get_body();
        file.close();

        std::string response = create_http_response("File uploaded successfully", "text/plain");
        send(client_fd, response.c_str(), response.length(), 0);
    }
    catch (const std::exception&) {
        std::string error = create_error_response(500, "Internal Server Error");
        send(client_fd, error.c_str(), error.length(), 0);
    }
}

// Handles DELETE requests:
// - Removes file at specified path
// - Returns 200 if successful
// - Returns 404 if file doesn't exist or can't be deleted
void handle_delete_request(const std::string& path, int client_fd, const std::string& base_path) {
    try {
        std::string filename = path.substr(path.find_last_of('/') + 1);
        std::string file_path = find_uploaded_file(filename, base_path);
        if (file_path.empty() || remove(file_path.c_str()) != 0) {
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

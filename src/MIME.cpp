#include "../includes/MIME.hpp"
// Initialize static members
std::map<std::string, std::string> MIME::mime_types_;
bool MIME::initialized_ = false;

void MIME::init_mime_types() {
    if (initialized_)
        return;
        
    // Text files
    mime_types_["html"] = "text/html";
    mime_types_["htm"] = "text/html";
    mime_types_["css"] = "text/css";
    mime_types_["js"] = "application/javascript";
    mime_types_["txt"] = "text/plain";
    mime_types_["csv"] = "text/csv";
    
    // Images
    mime_types_["jpg"] = "image/jpeg";
    mime_types_["jpeg"] = "image/jpeg";
    mime_types_["png"] = "image/png";
    mime_types_["svg"] = "image/svg+xml";
    mime_types_["ico"] = "image/x-icon";
    
    // Documents
    mime_types_["pdf"] = "application/pdf";
    mime_types_["json"] = "application/json";
    mime_types_["xml"] = "application/xml";
    
    // Archives
    mime_types_["zip"] = "application/zip";
    mime_types_["gz"] = "application/gzip";
    mime_types_["tar"] = "application/x-tar";
    
    initialized_ = true;
}

std::string MIME::get_type(const std::string& path) {
    init_mime_types();
    
    // Extract file extension
    size_t dot_pos = path.find_last_of(".");
    if (dot_pos == std::string::npos) {
        return "application/octet-stream";
    }
    
    std::string ext = path.substr(dot_pos + 1);
    std::map<std::string, std::string>::const_iterator it = mime_types_.find(ext);
    
    // Return found MIME type or default binary type
    return (it != mime_types_.end()) ? it->second : "application/octet-stream";
}

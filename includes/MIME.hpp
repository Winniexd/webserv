#ifndef MIME_HPP
#define MIME_HPP

#include <string>
#include <map>

class MIME {
public:
    // Get MIME type for a file extension
    static std::string get_type(const std::string& path);
   
private:
    // Initialize MIME types map
    static void init_mime_types();
    
    // Map of file extensions to MIME types
    static std::map<std::string, std::string> mime_types_;
    
    // Flag to check if MIME types are initialized
    static bool initialized_;
};

#endif
/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cgi.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/08 12:28:57 by marvin            #+#    #+#             */
/*   Updated: 2025/05/08 12:28:57 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/cgi.hpp"

Cgi::Cgi() {
    char buffer[PATH_MAX];
    getcwd(buffer, PATH_MAX);
    this->cgi_path = std::string(buffer) + "/www/cgi-bin/";
    this->argv = (char **)malloc(2 * sizeof(char *));
}

Cgi::~Cgi() {
    if (envp) {
        for (int i = 0; envp[i]; i++)
            free(envp[i]);
        free(envp);
    }
    if (argv) {
        for (int i = 0; argv[i]; i++)
            free(argv[i]);
        free(argv);
    }
}

int Cgi::get_out_fd() const {
    return out_fd[0];
}

void Cgi::add(const std::string &key, const std::string &value) {
    env.push_back(key + "=" + value);
}

void Cgi::convert() {
    this->envp = (char **)calloc(this->env.size() + 1, sizeof(char *));
    if (!this->envp) {
        std::cerr << "Envp allocation error" << std::endl;
        return ;
    }
    for (std::size_t i = 0; i < env.size(); i++) {
        this->envp[i] = strdup(this->env[i].c_str());
    }
    this->envp[env.size()] = NULL;
}

void Cgi::init_env(const HTTPRequest &request) {
    std::string request_path = request.get_path();
    std::string cgi_dir = "/cgi-bin/";

    if (request_path.empty()) {
        std::cerr << "Request path is empty!" << std::endl;
        return ;
    }
    
    if (request_path[0] != '/') //Checks if path is relative, if so make the path absolute
        exec_path = cgi_path + request_path;
    else { //else exec_path is equal to the absolute path
        size_t pos = request_path.find(cgi_dir);
        if (pos == std::string::npos) { //check if the absolute path contains the cgi-bin directory
            std::cerr << "CGI path must contain: " << cgi_dir << std::endl;
            return ;
        }
        if (pos != 0) {
            std::cerr << "CGI path must start with: " << cgi_dir << std::endl;
            return ;
        }
        exec_path = cgi_path + request_path.substr(cgi_dir.length());
    }
    if (request_path.find("..") != std::string::npos) {
        std::cerr << "Path traversal is not allowed" << std::endl;
        return ;
    }
    std::string host = request.get_header("host");
    size_t pos = host.find(":");
    std::string host_name = host;
    std::string host_port = "80";
    if (pos != std::string::npos) {
        host_name = host.substr(0, pos);
        host_port = host.substr(pos + 1);
    }
    add("AUTH_TYPE", request.get_header("auth-scheme"));
    add("CONTENT_LENGTH", request.get_header("content-length"));
    add("CONTENT_TYPE", request.get_header("content-type"));
    add("GATEWAY_INTERFACE", "CGI/1.1");
    add("PATH_INFO", request_path);
    add("PATH_TRANSLATED", exec_path);
    add("QUERY_STRING", ""); //Need a get_query_string() function
    add("REMOTE_ADDR", ""); //Don't think we need this one since our client will have the same address as our host in our case
    add("REQUEST_METHOD", request.get_method());
    add("SCRIPT_NAME", request_path.substr(cgi_dir.length()));
    add("SERVER_NAME", host_name);
    add("SERVER_PORT", host_port);
    add("SERVER_PROTOCOL", request.get_version());

    convert(); //Convert env to char array for execve
    argv[0] = strdup(exec_path.c_str());
    argv[1] = NULL;

    //for (std::size_t i = 0; envp[i]; i++)
    //    std::cout << envp[i] << std::endl;
    //std::cout << std::endl;
}

int Cgi::exec() {
    if (pipe(in_fd) || pipe(out_fd)) { //Check if pipe command fails
        std::cerr << "Pipe failed!" << std::endl;
        return 1;
    }
    //std::cout << argv[0] << std::endl;
    //std::cout << argv[1] << std::endl;

    pid_t pid = fork();
    if (pid < 0) {
        std::cerr << "Fork failed!" << std::endl;
        return 1;
    }

    if (pid == 0) {
        dup2(in_fd[0], STDIN_FILENO);
        dup2(out_fd[1], STDOUT_FILENO);
        close(in_fd[0]);
        close(in_fd[1]);
        close(out_fd[0]);
        close(out_fd[1]);
        char* const* null = NULL;
        execve(argv[0], null, envp); //Failing don't know why yet
        std::cout << argv[0] << " :Execve Failed" << std::endl;
        exit(1);
    }
    else {
        close(in_fd[0]);
        close(out_fd[1]);
    }

    int status;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status): 1;
}
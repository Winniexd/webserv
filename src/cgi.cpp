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
    this->cgi_path = std::string(buffer) + "/www/cgi-bin";
    this->argv = (char **)malloc(3 * sizeof(char *));  // 3 arguments : php, script, NULL
    this->envp = NULL;
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

void Cgi::add(const std::string &key, const std::string &value) {
    env.push_back(key + "=" + value);
}

int Cgi::get_out_fd() const {
    return out_fd[0];
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
        throw std::runtime_error("Request path is empty");
    }
    
    if (request_path.find(cgi_dir) != 0) {
        throw std::runtime_error("CGI path must start with: " + cgi_dir);
    }

    std::string script_name = request_path.substr(cgi_dir.length());
    exec_path = cgi_path + "/" + script_name;

    if (request_path.find("..") != std::string::npos) {
        throw std::runtime_error("Path traversal is not allowed");
    }

    if (access(exec_path.c_str(), F_OK) == -1) {
        throw std::runtime_error("CGI script not found: " + exec_path);
    }

    if (access(exec_path.c_str(), X_OK) == -1) {
        throw std::runtime_error("CGI script is not executable: " + exec_path);
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

    convert();
    argv[0] = strdup(exec_path.c_str());
    argv[1] = NULL;

    for (std::size_t i = 0; envp[i]; i++)
        std::cout << envp[i] << std::endl;
    std::cout << std::endl;
}

int Cgi::exec() {
    if (pipe(in_fd) || pipe(out_fd)) {
        throw std::runtime_error("Pipe creation failed");
    }

    pid_t pid = fork();
    if (pid < 0) {
        throw std::runtime_error("Fork failed");
    }

    if (pid == 0) {
        dup2(in_fd[0], STDIN_FILENO);
        dup2(out_fd[1], STDOUT_FILENO);
        close(in_fd[0]);
        close(in_fd[1]);
        close(out_fd[0]);
        close(out_fd[1]);
        
        argv[0] = strdup("/opt/homebrew/bin/php");
        argv[1] = strdup(exec_path.c_str());
        argv[2] = NULL;
        
        if (execve(argv[0], argv, envp) == -1) {
            std::cerr << "Execve failed: " << strerror(errno) << std::endl;
            exit(1);
        }
    }
    
    close(in_fd[0]);
    close(out_fd[1]);

    int status;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
}
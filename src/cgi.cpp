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

Request::Request() {
    this->path = "/cgi-bin/test.sh";
}

Request::~Request() {

}

std::string const Request::get_path() const {
    return this->path;
}

Cgi::Cgi() {
    char buffer[PATH_MAX];
    getcwd(buffer, PATH_MAX);
    this->cgi_path = std::string(buffer) + "/cgi-bin/";
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

void Cgi::add(const std::string &key, const std::string &value) {
    env.emplace_back(key + "=" + value);
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
    this->envp[env.size()] = nullptr;
}

void Cgi::init_env(Request &request) {
    std::string request_path = request.get_path();
    std::string cgi_dir = "/cgi-bin/";

    if (request_path.empty()) {
        std::cerr << "Request path is empty!" << std::endl;
        return ;
    }
    
    if (request_path[0] != '/') //Checks if path is relative, if so make the path absolute
        exec_path = cgi_path + request.get_path();
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
    add("GATEWAY", "CGI/1.1");
    add("PATH_INFO", request_path);
    add("SCRIPT_NAME", request_path.substr(cgi_dir.length()));
    add("PATH_TRANSLATED", exec_path);

    convert(); //Convert env to char array for execve
    argv[0] = strdup(exec_path.c_str());
    argv[1] = nullptr;

    for (std::size_t i = 0; envp[i]; i++)
        std::cout << envp[i] << std::endl;
    std::cout << std::endl;
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
        std::cout << "Execve Failed" << std::endl;
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


int main() {
    Cgi cgi;
    Request request;
    cgi.init_env(request);
    cgi.exec();
    char buffer[1024];
    int ret = 0;
    while ((ret = read(cgi.out_fd[0], buffer, sizeof(buffer) - 1)) > 0) {
        buffer[ret] = '\0';
        std::cout << buffer;
    }
    close(cgi.out_fd[0]);
}
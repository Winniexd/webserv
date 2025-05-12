/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cgi.hpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/08 12:28:03 by marvin            #+#    #+#             */
/*   Updated: 2025/05/08 12:28:03 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGI_HPP
#define CGI_HPP

#include <iostream>
#include <vector>
#include <filesystem>
#include <string>
#include <sys/types.h>
#include <cstring>
#include <sys/wait.h>
#include <unistd.h>

#define PATH_MAX 4096

class Request {
    private:
        std::string path;
    public:
        Request();
        ~Request();
        std::string const get_path() const;
};

class Cgi {
    private:
        std::vector<std::string> env;
        int content_len;
        std::string query_string;
        std::string cgi_path;
        std::string exec_path;
        char **argv;
        char **envp;
    public:
        int in_fd[2];
        int out_fd[2];
        Cgi();
        ~Cgi();
        Cgi(const Cgi &rhs);
        Cgi &operator=(const Cgi &src);
        void add(const std::string &key, const std::string &value);
        void convert();

        void init_env(Request &request);
        int exec();
};

#endif
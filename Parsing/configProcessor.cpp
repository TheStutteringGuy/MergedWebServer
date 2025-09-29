/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   configProcessor.cpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aahlaqqa <aahlaqqa@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/01 16:54:24 by ahmed             #+#    #+#             */
/*   Updated: 2025/08/01 22:44:20 by aahlaqqa         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "configProcessor.hpp"
#include "parser.hpp"

ConfigProcessor::ConfigProcessor() {

};

ConfigProcessor::~ConfigProcessor() {

};

const char *ConfigProcessor::InvalidPortException::what() const throw()
{
    return "Invalid Port number !!";
};

const char *ConfigProcessor::DuplicatePortEXception::what() const throw()
{
    return "Duplicate Port detected !!";
};

const char *ConfigProcessor::InvalidPathException::what() const throw()
{
    return "Invalid Path detected !!";
};

const char *ConfigProcessor::InvalidDirectiveException::what() const throw()
{
    return "Invalid directive is detected !!";
};

void ConfigProcessor::processServerBlock(std::vector<ServerBlock> &blocks)
{
    for (size_t i = 0; i < blocks.size(); i++)
    {
        setDefaultValue(blocks[i]);
        processServerDirective(blocks[i]);
        for (size_t j = 0; j < blocks[i].locationBlocks.size(); j++)
        {
            setLocation(blocks[i].locationBlocks[j], blocks[i]);
            processLocationDirective(blocks[i].locationBlocks[j], blocks[i]);
        }
    }
    validateConfig(blocks);
};

void ConfigProcessor::processServerDirective(ServerBlock &blocks)
{
    for (size_t i = 0; i < blocks.directives.size(); i++)
    {
        const Directive &dir = blocks.directives[i];
        if (dir.name == "listen")
        {
            if (!dir.values.empty())
                blocks.port = stringToInt(dir.values[0]);
        }
        else if (dir.name == "host")
        {
            if (!dir.values.empty())
                blocks.host = dir.values[0];
        }
        else if (dir.name == "server_name")
        {
            if (!dir.values.empty())
                blocks.server_name = dir.values[0];
        }
        else if (dir.name == "root")
        {
            if (!dir.values.empty())
                blocks.root = dir.values[0];
        }
        else if (dir.name == "index")
        {
            if (!dir.values.empty())
                blocks.index = dir.values[0];
        }
        else if (dir.name == "error_page")
        {
            if (dir.values.size() >= 2)
            {
                int err_code = stringToInt(dir.values[0]);
                blocks.error_pages[err_code] = dir.values[1];
            }
        }
        else if (dir.name == "client_max_body_size")
        {
            if (!dir.values.empty())
                blocks.clientSizeBody = static_cast<size_t>(stringToInt(dir.values[0]));
        }
        else if (dir.name == "allow_methods")
        {
            if (!dir.values.empty())
            {
                for (size_t i = 0; i < dir.values.size(); i++)
                {
                    if (dir.values[i] != "GET" && dir.values[i] != "POST" && dir.values[i] != "DELETE" )
                    {
                        throw Parser::InvalidMthode();
                        
                    }
                }
            }
        }
    }
};

void ConfigProcessor::processLocationDirective(LocationBlock &location, const ServerBlock &server)
{
    (void)server;
    for (size_t i = 0; i < location.directives.size(); i++)
    {
        const Directive &dir = location.directives[i];
        if (dir.name == "root")
        {
            if (!dir.values.empty())
                location.root = dir.values[0];
        }
        else if (dir.name == "index")
        {
            if (!dir.values.empty())
                location.index = dir.values[0];
        }
        else if (dir.name == "allow_methods")
            location.allowed_methods = dir.values;
        else if (dir.name == "autoindex")
        {
            if (!dir.values.empty())
                location.autoindex = (dir.values[0] == "on");
        }
        else if (dir.name == "return")
        {
            if (!dir.values.empty())
                location.redirect_url = dir.values[0];
        }
        else if (dir.name == "cgi_path")
        {
            if (!dir.values.empty())
                location.cgi_path = dir.values[0];
        }
        else if (dir.name == "cgi_ext")
            location.cgi_extention = dir.values;
        else if (dir.name == "upload_path")
        {
            if (!dir.values.empty())
                location.upload_path = dir.values[0];
        }
    }
};

void ConfigProcessor::validateConfig(const std::vector<ServerBlock> &blocks)
{
    validatePort(blocks);
    validatePath(blocks);
};

void ConfigProcessor::validatePort(const std::vector<ServerBlock> &blocks)
{
    std::set<int> nonused_port;
    for (size_t i = 0; i < blocks.size(); i++)
    {
        if (!isValidPort(blocks[i].port))
            throw InvalidPortException();
        if (nonused_port.find(blocks[i].port) != nonused_port.end())
            throw DuplicatePortEXception();
        nonused_port.insert(blocks[i].port);
    }
};

void ConfigProcessor::validatePath(const std::vector<ServerBlock> &blocks)
{
    for (size_t i = 0; i < blocks.size(); i++)
    {
        const ServerBlock &server = blocks[i];
        //std::cout << "Checking server root: " << server.root << std::endl;
        if (!existDirectory(server.root))
        {
            std::cout << "ERROR: Server root directory does not exist: " << server.root << std::endl;
            throw InvalidPathException();
        }

        for (std::map<int, std::string>::const_iterator it = server.error_pages.begin(); it != server.error_pages.end(); it++)
        {
            std::string fullPath = handlePath(server.root, it->second);
            if (!existFile(fullPath))
            {
                std::cout << "ERROR: Error page file does not exist: " << fullPath << std::endl;
                throw InvalidPathException();
            }
        }
        for (size_t j = 0; j < server.locationBlocks.size(); j++)
        {
            const LocationBlock &location = server.locationBlocks[j];
            //std::cout << "Checking location root: " << location.root << std::endl;

            if (!existDirectory(location.root))
            {
                std::cout << "ERROR: Location root directory does not exist: " << location.root << std::endl;
                throw InvalidPathException();
            }
        }
    }
};

void ConfigProcessor::setDefaultValue(ServerBlock &server)
{
    // if (server.port == 80 && server.host == "0.0.0.0")
    // {
    //     // guys this is a do nothing block because i already set it in the serverblock constructor :)
    //     // And it's not neccery to use it in the first place
    // }
    if (server.server_name.empty())
        server.server_name = "localhost";
    if (server.index.empty())
        server.index = "index.html";
};

void ConfigProcessor::setLocation(LocationBlock &location, const ServerBlock &server)
{
    if (location.root.empty())
        location.root = server.root; // here if the root of the location block is empty i inherit it from the serever root
    if (location.index.empty())
        location.index = server.index; // the same thing here too
    if (location.allowed_methods.empty())
    {
        location.allowed_methods.push_back("GET");
        location.allowed_methods.push_back("POST");
        location.allowed_methods.push_back("DELETE");
    }
    if (location.upload_path.empty())
        location.upload_path = "";
};

int ConfigProcessor::stringToInt(const std::string &str)
{
    std::stringstream ss(str);
    int res;

    ss >> res;
    if (ss.fail())
        throw InvalidDirectiveException();
    return (res);
};

bool ConfigProcessor::isValidPort(int port)
{
    if (port > 0 && port <= 65535)
        return (true);
    else
        return (false);

    // 1-1023: Well-known ports for example :  (HTTP=80, HTTPS=443, SSH=22 ...)
    // 1024-49151: Registered ports
    // 49152-65535: Dynamic/private ports
};

bool ConfigProcessor::existFile(const std::string &path)
{
    struct stat buffer;
    if (stat(path.c_str(), &buffer) != 0)
        return (false);
    if (S_ISREG(buffer.st_mode))
        return (true);
    else
        return (false);
};

bool ConfigProcessor::existDirectory(const std::string &path)
{
    struct stat buffer;
    if (stat(path.c_str(), &buffer) != 0)
        return (false);
    if (S_ISDIR(buffer.st_mode))
        return (true);
    else
        return (false);
};

std::string ConfigProcessor::handlePath(const std::string &base, const std::string &path)
{
    if (path.empty())
        return (base); // return base directory if path is empty
    if (path[0] == '/')
        return (path); // return the absolute path
    std::string res = base;
    if (!res.empty() && res[res.length() - 1] != '/')
        res += "/";
    res += path;
    return (res); // return the full path after combination
};
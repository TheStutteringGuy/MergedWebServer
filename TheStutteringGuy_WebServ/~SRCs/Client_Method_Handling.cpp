#include "WebServer.hpp"
#include <cstdio>
#include <cstring>
#include <stdexcept>

static std::string getParentPath(const std::string& filepath)
{
    std::string parent_path;
    size_t pos;

    pos = filepath.find_last_of('/');
    parent_path = filepath.substr(0, pos + 1);

    return parent_path;
}

void Client::handle_DELETE(void)
{
    struct stat test;

    std::string actual_URI(this->m_request.m_URI);
    actual_URI.erase(0, 1);
    actual_URI.insert(0, this->m_Myserver.m_root);

    if (access(actual_URI.c_str(), F_OK) != 0)
        this->response_Error(404, true);

    stat(actual_URI.c_str(), &test);
    if (S_ISDIR(test.st_mode))
        this->response_Error(403, true);

    std::string parent_path = getParentPath(actual_URI);
    if (access(parent_path.c_str(), W_OK) != 0)
        this->response_Error(403, true);

    std::remove(actual_URI.c_str());
    this->response_justAstatus(204);
}

void Client::handle_GET(void)
{
    struct stat test;
    
    std::string actual_URI(this->m_request.m_URI);
    actual_URI.erase(0, 1);
    actual_URI.insert(0, this->m_Myserver.m_root);

    if (access(actual_URI.c_str(), F_OK) != 0)
        this->response_Error(404, true);
    
    stat(actual_URI.c_str(), &test);
    if (S_ISDIR(test.st_mode))
    {
        // listing the directory
        if (this->m_request.m_URI[this->m_request.m_URI.size() - 1] != '/')
        {
            this->m_request.m_URI.append("/");
            actual_URI.append("/");
        }

        if (access(actual_URI.c_str(), R_OK | X_OK) != 0)
            this->response_Error(403, true);

        DIR* dir_ptr;
        struct dirent *entry;

        dir_ptr = opendir(actual_URI.c_str());
        if (NULL == dir_ptr)
            this->response_Error(503, true);

        std::string to_send;
        std::string s = " ";
        std::string end = "\n";

        to_send = generic_index_page(this->m_request.m_URI);

        size_t pos = to_send.find("INSERT HERE");
        if (pos != std::string::npos)
            to_send.erase(pos, static_cast<std::string>("INSERT HERE").size());
        while ((entry = readdir(dir_ptr)) != NULL)
        {
            if (entry->d_name[0] != '.')
            {
                if (entry->d_type == DT_DIR)
                {
                    std::stringstream ss;
                    ss << "            <li><a href=\"" << this->m_request.m_URI + entry->d_name << "/\">" << entry->d_name << '/' << "</a></li>" << end;
                    to_send.insert(pos, ss.str());
                    pos += ss.str().size();
                }
                else if (entry->d_type == DT_REG)
                {
                    std::stringstream ss;
                    ss << "            <li><a href=\"" << this->m_request.m_URI + entry->d_name << "\" class=\"file-link\">" << entry->d_name << "</a></li>" << end;
                    to_send.insert(pos, ss.str());
                    pos += ss.str().size();
                }
            }
        }
        closedir(dir_ptr);
        this->response_html_ready(to_send);
    }
    else
    {
        if (access(actual_URI.c_str(), R_OK) != 0)
            this->response_Error(403, true);
        
        this->response_Get(actual_URI);
    }
}

void Client::handle_POST(void)
{
    const std::vector<std::string> *content_type = find_Value_inMap(this->m_request.m_headers, "Content-Type");

    std::string media_type;
    if (NULL == content_type)
        media_type = "";
    else
    {
        media_type = ValuesSingleton::getValuesSingleton().m_Reverse_FileContent_type.getContent_type((*content_type)[0]);
    }

    std::string uid = uid_generator();
    std::string filename = uid + media_type;
    std::string upload_path = static_cast<std::string>(this->m_Myserver.m_root) + static_cast<std::string>(upload_dir);
    std::string final_path = upload_path + filename;

    if (std::rename(this->m_body_asFile_path.c_str(), final_path.c_str()) != 0)
    {
        this->response_Error(500, true);
        throw std::runtime_error("std::rename()" + static_cast<std::string>(strerror(errno)));
    }

    std::string Headers = headers_Creator(Response("HTTP/1.1", 201, false, std::string(), 0), 0);
    std::string Locattion_Header = "Location: " + static_cast<std::string>(upload_dir) + filename + "\r\n";

    Headers += Locattion_Header + "\r\n";

    this->m_response_buffer = Headers;
    this->readyto_send = true;

    epoll_event event;
    event.events = EPOLLOUT | EPOLLHUP | EPOLLERR;
    event.data.fd = this->m_client_fd;
    epoll_ctl(ValuesSingleton::getValuesSingleton().epoll_fd, EPOLL_CTL_MOD, this->m_client_fd, &event);
    throw CONTINUE;
}

void Client::handle_Request(void)
{
    this->handling_request = true;

    if (this->m_request.m_method == "GET")
        this->handle_GET();
    else if (this->m_request.m_method == "POST")
        this->handle_POST();
    else if (this->m_request.m_method == "DELETE")
        this->handle_DELETE();
}
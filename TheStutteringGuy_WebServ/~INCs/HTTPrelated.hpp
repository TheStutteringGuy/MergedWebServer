#pragma once
#include "../~INCs/'Standards'.hpp"

#include "Namespaces.hpp"
#include "HTTPconst.hpp"
#include "../Exposed_Functions.hpp"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ServerBlock :

struct MyLocationBlock
{
    std::string path;
    std::map<std::string, std::vector<std::string> > directives;

    std::string root;
    std::string index;
    bool autoindex;
    std::vector<std::string> allowed_methods;
    // std::string upload_path;
    std::string redirect_url;
    std::string cgi_path;
    std::vector<std::string> cgi_extention;
};

struct MyServerBlock
{
    std::map<std::string, std::vector<std::string> > m_directives;
    std::vector<MyLocationBlock> m_locationBlocks;

    std::string m_root;
    std::string m_cache;

    std::map<int, std::string> m_error_pages;
    // std::string m_index_def;
    // bool   m_autoindex_def;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RESPONSE / REQUEST / CLIENT :

struct Response // this is more like struct HEADER
{
    std::string     m_HTTP_version;
    unsigned int    m_status_code;

    bool            m_content_needed;

    std::string     m_content_type;
    int             m_content_length;

    Response() : m_HTTP_version("HTTP/1.1") {};
    ~Response() {};
    Response(std::string HTTP_version, unsigned int status_code, bool content_needed, std::string content_type, int content_length) 
    : m_HTTP_version(HTTP_version), m_status_code(status_code), m_content_needed(content_needed),m_content_type(content_type), m_content_length(content_length) {}
};

struct Request
{
    std::string                                         m_method;

    std::string                                         m_URI;
    std::string                                         m_queryString;

    std::string                                         m_version;
    std::map< std::string, std::vector<std::string> >   m_headers;

    std::string                                         m_body;
};

inline std::string uid_generator(void)
{
    struct timespec time;

    clock_gettime(CLOCK_REALTIME, &time);

    size_t value;

    value = time.tv_sec * 1000000000ULL + time.tv_nsec;

    std::stringstream ss;

    ss << value;
    return ss.str();
}

class Client
{
public:
    www::fd_t       m_client_fd;
    MyServerBlock   m_Myserver;

    bool            header_done;
    bool            need_body;
    bool            m_isChunked;
    bool            handling_request;            
    bool            readyto_send;

    std::string     m_request_buffer;
    std::string     m_headers_buffer;
    std::string     m_body_buffer;

private:
    Request         m_request;
    std::string     m_response_buffer;

private:
    bool            m_body_asFile_init;
    bool            m_response_asFile_init;

public:
    std::fstream*   m_body_asFile;
    std::string     m_body_asFile_path;
    
private:
    bool            m_response_is_aFile;
    std::fstream*   m_response_asFile;
    std::string     m_response_asFile_path;


public:
    Client() : m_client_fd(-1), header_done(false), need_body(true), m_isChunked(false), handling_request(false), readyto_send(false) , m_body_asFile_init(false), m_response_asFile_init(false), m_body_asFile(0), m_response_is_aFile(false),  m_response_asFile(0) {};
    Client(www::fd_t client_fd) : m_client_fd(client_fd), header_done(false), need_body(true), m_isChunked(false), handling_request(false), readyto_send(false), m_body_asFile_init(false), m_response_asFile_init(false), m_body_asFile(0),  m_response_is_aFile(false), m_response_asFile(0) {};
    ~Client() 
    {
        if (m_body_asFile)
        {
            m_body_asFile->close();
            delete m_body_asFile;
            std::remove(m_body_asFile_path.c_str());
        }
        if (m_response_asFile)
        {
            m_response_asFile->close();
            delete m_response_asFile;
        }
    };
    // Client(const Client& other) { *this = other; };

public:
    Client& operator=(const Client& other)
    {
        if (this == &other)
            return *this;
        
        m_client_fd = other.m_client_fd;
        m_Myserver = other.m_Myserver;
        header_done = other.header_done;
        need_body = other.need_body;
        m_isChunked = other.m_isChunked;
        handling_request = other.handling_request;
        readyto_send = other.readyto_send;
        m_request_buffer = other.m_request_buffer;
        m_headers_buffer = other.m_headers_buffer;
        m_body_buffer = other.m_body_buffer;
        m_request = other.m_request;
        m_response_buffer = other.m_response_buffer;
        
        if (m_body_asFile)
            delete m_body_asFile;
        if (m_response_asFile)
            delete m_response_asFile;
        m_body_asFile_init = false;
        m_response_asFile_init = false;
        m_body_asFile_path = other.m_body_asFile_path;
        m_response_is_aFile = other.m_response_is_aFile;
        m_response_asFile_path = other.m_response_asFile_path;
        return *this;   
    }

public:
    void initialize_body_asFile(void)
    {
        if (this->m_body_asFile_init == false)
        {
            this->m_body_asFile = new std::fstream();

            std::string l_string = m_Myserver.m_cache + uid_generator();
            this->m_body_asFile_path = l_string;
            m_body_asFile->open(l_string.c_str(), std::ios::in | std::ios::out | std::ios::binary | std::ios::app);
            if (!m_body_asFile->is_open())
                throw std::runtime_error("m_body_asFile.isopem()");
            this->m_body_asFile_init = true;
        }
    }
    void initialize_response_asFile(const std::string& filename)
    {
        if (m_response_asFile_init == false)
        {
            this->m_response_asFile = new std::fstream();

            this->m_response_asFile_path = filename;
            this->m_response_asFile->open(filename.c_str(), std::ios::in | std::ios::binary);
            if (!m_response_asFile->is_open())
                throw std::runtime_error("m_body_asFile.isopem()");
            m_response_asFile_init = true;
        }
    }

public:
    bool            headerEnd_Checker(void);
    void            headers_parser(void);
    void            URI_parser(void);
    void            headers_implication(void);
    bool            body_checker(void);

public:
    void            response_Error(const unsigned int &error_code, bool Content_needed);
    void            response_html_ready(const std::string &string_);

public:
    void            handle_Request(void);
    void            handle_Response(void);

private:
    void            handle_GET(void);
    void            handle_POST(void);
    void            handle_DELETE(void);
    void            response_Get(const std::string& File);
    void            response_justAstatus(const unsigned int &status_code);
    void            response_Creator(const unsigned int &status_code, const bool& content_needed, const std::string& content_type, const std::string &body);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Singletons :

struct ValuesSingleton
{
    std::vector<addrinfo* >           addrinfo_vect;
    www::fd_t                       epoll_fd;
    std::vector<MyServerBlock>      servers_blocks;

    std::map<www::fd_t, MyServerBlock>    _serverfd_map;
    std::map <www::fd_t, Client>          _clients_map;

    struct HTTPstatus_phrase        m_HTTPstatus_phrase;
    struct FileContent_type         m_FileContent_type;
    struct Reverse_FileContent_type m_Reverse_FileContent_type;

    static ValuesSingleton& getValuesSingleton()
    {
        static ValuesSingleton Singleton;
        return Singleton;
    }
};

void    multiplexer(void);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions : 

template <typename T>
static std::string  to_string_c98(T value) 
{
    std::stringstream ss;
    ss << value;
    return ss.str();
}

inline std::string readFile(const std::string& filename) 
{
    std::fstream file(filename.c_str(), std::ios::in | std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("file.open()" + static_cast<std::string>(strerror(errno)));
    
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::string buffer(size, '\0');
    file.read(&buffer[0], size);
    return buffer;
}

inline const std::vector<std::string>* find_Value_inMap(const std::map<std::string, std::vector<std::string> > &map, const std::string& key)
{
    std::map<std::string, std::vector<std::string> >::const_iterator it = map.find(key);
    if (it != map.end())
        return &(it->second);
    return NULL;
}

// Response Related :

inline static std::string get_http_date() 
{
    char buf[128];
    time_t t = time(NULL);
    tm *tm = gmtime(&t);
    strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", tm);
    return std::string(buf);
}

inline std::string headers_Creator(struct Response response)
{
    std::string end = "\r\n";
    std::string eoh = "\r\n\r\n";
    std::string s = " ";

    std::string l_status_code = to_string_c98(response.m_status_code);
    std::string l_phrase = s + ValuesSingleton::getValuesSingleton().m_HTTPstatus_phrase.getStatusPhrase(l_status_code);

    std::string Response = response.m_HTTP_version + s + l_status_code + s + l_phrase + end;
    Response += "Date: " + get_http_date() + end;
    if (true == response.m_content_needed)
    {
        Response += "Content-Type: " + response.m_content_type + end;
        Response += "Content-Length: " + to_string_c98(response.m_content_length);
    }
    Response += eoh;

    return Response;
}

inline std::string headers_Creator(struct Response response, int)
{
    std::string end = "\r\n";
    std::string s = " ";

    std::string l_status_code = to_string_c98(response.m_status_code);
    std::string l_phrase = s + ValuesSingleton::getValuesSingleton().m_HTTPstatus_phrase.getStatusPhrase(l_status_code);

    std::string Response = response.m_HTTP_version + s + l_status_code + s + l_phrase + end;
    Response += "Date: " + get_http_date() + end;
    if (true == response.m_content_needed)
    {
        Response += "Content-Type: " + response.m_content_type + end;
        Response += "Content-Length: " + to_string_c98(response.m_content_length);
    }
    return Response;
}

// inline std::string getFullPath(const std::string& relative_path) 
// {
//     char resolved_path[PATH_MAX];
//     if (realpath(relative_path.c_str(), resolved_path) != NULL) 
//         return std::string(resolved_path);
//     return "";
// }
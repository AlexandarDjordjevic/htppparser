#include <HTTP/Request.h>
#include <iostream>
namespace HTTP{

    Request::Request()
    {
    }

    Request::~Request()
    {
    }

    Method Request::get_method()
    {
        return m_method;
    }

    Version Request::get_version()
    {
        return m_version;
    }

    std::string Request::get_uri_type()
    {
        return uri_type_to_stirng();
    }

    std::string Request::get_field_value(const std::string& key)
    {
        auto it = std::find_if(m_header.begin(), m_header.end(), [&key](const Header_field &field){ return field.key == key; });
        if (it != m_header.end())
        {
            auto index = std::distance(m_header.begin(), it);
            return m_header[index].value;
        }
        return "";
    }

    bool Request::parse_request_body(const std::string& body)
    {
        // The presence of a message-body in a request is signaled by the
        // inclusion of a Content-Length or Transfer-Encoding header field in
        // the request's message-headers. A message-body MUST NOT be included in
        // a request if the specification of the request method (section 5.1.1)
        // does not allow sending an entity-body in requests. A server SHOULD
        // read and forward a message-body on any request; if the request method
        // does not include defined semantics for an entity-body, then the
        // message-body SHOULD be ignored when handling the request.
    }
    
    std::pair<std::string, std::size_t> Request::tokenize(const std::string& text, const std::string& delimeter, std::size_t position) 
    { 
        std::size_t found = text.find(delimeter, position);
        return { text.substr(position, found - position), found + delimeter.length() };
    }

    bool Request::parse_request(const std::string& request)
    {
        auto result= (parse_request_line(request) && from_string(request))? true :  false;
        return result;
    }

    const std::string Request::uri_type_to_stirng()
    {
        switch (m_uri.type)
        {
            case Uri_type::asterisk: return "asterisk";
            case Uri_type::absolute_uri: return "absolute_uri";
            case Uri_type::absolute_path: return "absolute_path";
            case Uri_type::authority: return "authority";
            default: return "unknown";
        }
    }

    bool Request::validate_method(const std::string& method)
    {
        auto it = table.find(method);   
        if (it != table.end())
        {
            m_method = it->second;
            return true;
        }
        return false; 
    }

    bool Request::validate_aterisk_uri(const std::string& uri)
    {
        if( uri == asterisk && m_method == Method::OPTIONS){
            m_uri.uri_str = asterisk; 
            m_uri.type = Uri_type::asterisk;
            return true;
        }
        return false;
    }

    bool Request::validate_absolute_uri(const std::string& uri)
    {
        m_uri.uri_obj.from_string(uri);
        if((m_uri.uri_obj.get_scheme() == http || m_uri.uri_obj.get_scheme() == https ) && (m_uri.uri_obj.get_path().empty() == false))
        {
            m_uri.type = Uri_type::absolute_uri;
            m_uri.uri_str = uri;
            return true;
        }
        return false;
    }

    bool Request::validate_absolute_path(const std::string& uri)
    {
        m_uri.uri_obj.from_string(uri);
        if(m_uri.uri_obj.get_scheme().empty() == true && m_uri.uri_obj.get_path().empty() == false)
        {
            m_uri.type = Uri_type::absolute_path;
            m_uri.uri_str = uri;
            return true;
        }
        return false;
    }

    bool Request::validate_authority_uri(const std::string& uri)
    {
        if(std::regex_match( uri.begin(), uri.end(), std::regex(R"([a-zA-Z0-9+:\_\.@]*)")) && m_method == Method::CONNECT){
            m_uri.uri_str = uri;
            m_uri.type = Uri_type::authority;
            return true; 
        }
        return false;
    }

    bool Request::validate_uri(const std::string& uri)
    {
        return validate_aterisk_uri(uri) || validate_absolute_uri(uri) || validate_absolute_path(uri) || validate_authority_uri(uri);
    }

    bool Request::parse_request_line(const std::string& request_line)
    {
        if (request_line.empty() == true)
        {   
            return false;
        }

        std::pair<std::string, std::size_t > req_line_tokens;
        req_line_tokens = tokenize(request_line, " ", 0);
        if(validate_method(req_line_tokens.first) == false)
        {
            return false;
        }
        req_line_tokens = tokenize(request_line, " ", req_line_tokens.second);
        if(validate_uri(req_line_tokens.first) == false)
        {
            return false;
        }
        req_line_tokens = tokenize(request_line, CRLF,req_line_tokens.second);
        if(validate_version(req_line_tokens.first) == false)
        {
            return false;
        }
        return true;
    }
    
    bool Request::from_string(const std::string& request)
    {
        if (request.empty() == true)
        {   
            return false;
        }
        std::pair<std::string, std::size_t> request_tokens;
        request_tokens = tokenize(request, CRLF, 0);
        if(parse_request_line(request_tokens.first) == false)
        {
            return false;
        }

        request_tokens = tokenize(request, CRLF + CRLF,request_tokens.second);
        if(parse_header_fields(request_tokens.first)==false)
        {
            return false;
            
        }
        return true;
    }

    bool Request::validate_version(const std::string& version)
    {
        auto ver = table_versions.find(version);   
        if (ver != table_versions.end())
        {
            m_version = ver->second;
            return true;
        }
        return false; 
    }    

    bool Request::ends_with(const std::string &main_str, const std::string &to_match)
    {
       return (main_str.size() >= to_match.size() &&
            main_str.compare(main_str.size() - to_match.size(), to_match.size(), to_match) == 0);
    }

    bool Request::parse_header_fields(const std::string& header)
    {
        Header_field h_field;
        std::pair<std::string, std::size_t> header_tokens;
        std::pair<std::string, std::size_t> fields_tokens;
       
        header_tokens = tokenize(header, "\n", 0);
        do
        {
            fields_tokens = tokenize(header_tokens.first, ": ", 0);
            h_field.key = fields_tokens.first;
            h_field.value = header_tokens.first.substr(fields_tokens.second, header_tokens.first.length()-1);
            m_header.push_back(h_field);

            header_tokens = tokenize(header, "\n", header_tokens.second);
        }
        while(header_tokens.second != 0);
        
        return true;
    }

}//namespace HTTP

// ----------------------------------------------------------------------------
//
//     Filename:  jsonrpc.h
//
//  Description:
//
//       Author:  Benny Bach
//      Company:
//
// ----------------------------------------------------------------------------
#ifndef __jsonrpc_h__
#define __jsonrpc_h__

// ----------------------------------------------------------------------------
#include <json.h>

// ----------------------------------------------------------------------------
#include <string>

// ----------------------------------------------------------------------------
class jsonrpc_request
{
public:
    jsonrpc_request()
        :
        m_error_code(0)
    {
    }
public:
    const std::string& version() { return m_version; }
    const std::string& method()  { return m_method;  }
    const json::value& params()  { return m_params;  }
    const json::value& id()      { return m_id;      }
public:
    bool is_valid() { return m_error_code == 0; }
public:
    json::object error()
    {
        json::object e;

        e.set("code", m_error_code);
        e.set("message", "Invalid Request");

        return std::move(e);
    }
public:
    static jsonrpc_request from_json(const json::value& v)
    {
        jsonrpc_request self;

        if ( v.is_object() )
        {
            auto o = v.get<json::object>();

            if ( !o.has("jsonrpc") ) {
                self.m_error_code = -32600;
            }

            if ( !o.has("method") ) {
                self.m_error_code = -32600;
            }

            if ( !o.has("params") ) {
                self.m_error_code = -32600;
            }

            if ( self.is_valid() )
            {
                self.m_version = o.get("jsonrpc").get<json::string>().str();
                self.m_method = o.get("method").get<json::string>().str();
                self.m_params = o.get("params");

                if ( o.has("id") ) {
                    self.m_id = o.get("id");
                }
            }
        }
        else
        {
            self.m_error_code = -32600;
        }
        return std::move(self);
    }
private:
    std::string m_version;
    std::string m_method;
    json::value m_params;
    json::value m_id;
    // If error != 0 none of the above will be valid.
    int m_error_code;
};

// ----------------------------------------------------------------------------
class jsonrpc_handler
{
public:
    virtual void call_method(const std::string& method, json::value params, json::object& response) = 0;
};

// ----------------------------------------------------------------------------
#endif // __jsonrpc_h__

// ----------------------------------------------------------------------------
//
//        Filename:  jsonrpc.h
//
//          Author:  Benny Bach
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#ifndef __jsonrpc_h__
#define __jsonrpc_h__

// ----------------------------------------------------------------------------
#include <json/json.h>

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
    return std::move(json::object{ { "code", m_error_code }, { "messge", "Invalid Request" } });
  }
public:
  static jsonrpc_request from_json(json::value& v)
  {
    jsonrpc_request self;

    if ( v.is_object() )
    {
      auto& o = v.as_object();

      if ( !o["jsonrpc"].is_string() ) {
        self.m_error_code = -32600;
      }

      if ( !o["method"].is_string() ) {
        self.m_error_code = -32600;
      }

      if ( o["params"].is_null() ) {
        self.m_error_code = -32600;
      }

      if ( self.is_valid() )
      {
        self.m_version = o["jsonrpc"].as_string();
        self.m_method = o["method"].as_string();
        self.m_params = o["params"];
        self.m_id = o["id"];
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

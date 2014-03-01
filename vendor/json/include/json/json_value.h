// ----------------------------------------------------------------------------
#ifndef __json__value_h__
#define __json__value_h__
// ----------------------------------------------------------------------------
#include <string>
#include <memory>
#include <cassert>
// ----------------------------------------------------------------------------
namespace json
{
  enum class type
  {
    nul = 0x01,
    num = 0x02,
    tru = 0x03,
    fal = 0x04,
    str = 0x81,
    obj = 0x82,
    arr = 0x83
  };

  class object;
  class array;

  class value
  {
    using str_ptr = std::string*;
    using arr_ptr = array*;
    using obj_ptr = object*;
  public:
    value() : type_(type::nul) {}
  public:
    value(const value& other);
    value(value&& other);
    value& operator= (const value& rhs);
    value& operator= (value&& rhs);
  public:
    value(const char* v);
    value(const std::string& v);
    value(std::string&& v);
    value(double v);
    value(int v);
    value(bool v);
    value(const json::array& v);
    value(json::array&& v);
    value(const json::object& v);
    value(json::object&& v);
  public:
    ~value();
  public:
    type type_id() const noexcept { return type_; }
  public:
    bool is_null()   const noexcept { return type_ == type::nul; }
    bool is_true()   const noexcept { return type_ == type::tru; }
    bool is_false()  const noexcept { return type_ == type::fal; }
    bool is_string() const noexcept { return type_ == type::str; }
    bool is_number() const noexcept { return type_ == type::num; }
    bool is_array()  const noexcept { return type_ == type::arr; }
    bool is_object() const noexcept { return type_ == type::obj; }
  public:
    const std::string& as_string() const
    {
      assert(is_string());
      return *str_;
    }
  public:
    double as_number() const
    {
      assert(is_number());
      return num_;
    }
  public:
    //const array& as_array()  const
    array& as_array()
    {
      assert(is_array());
      return *arr_;
    }
  public:
    //const object& as_object() const
    object& as_object()
    {
      assert(is_object());
      return *obj_;
    }
  public:
    std::string move_string()
    {
      assert(is_string());
      return std::move(*str_);
    }
  public:
    void write(std::ostream& os) const;
  private:
    void free_value();
  private:
    type type_;
  private:
    union
    {
      str_ptr str_;
      double  num_;
      bool    t_f_;
      arr_ptr arr_;
      obj_ptr obj_;
    };
  };

  inline std::string escape(const std::string& s)
  {
      std::string result;

      result.reserve(s.size()+2);

      result += "\"";
      for ( auto c : s )
      {
        switch ( c )
        {
          case '"':
          case '\\':
          case '/':
          case '\b':
          case '\f':
          case '\n':
          case '\r':
          case '\t':
            result += '\\';
            // FALL THROUGH INTENEDED
          default:
            result += c;
        }
      }
      result += "\"";

      return std::move(result);
  }
}

// ----------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, json::type t);

// ----------------------------------------------------------------------------
inline std::ostream& operator<<(std::ostream& os, const json::value& v)
{
  v.write(os);
  return os;
}

// ----------------------------------------------------------------------------
#endif // __json__value_h__

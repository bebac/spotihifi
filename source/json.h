// ----------------------------------------------------------------------------
//
//        Filename:  json.h
//
//          Author:  Benny Bach
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#ifndef __json__json_h__
#define __json__json_h__

// ----------------------------------------------------------------------------
#include <memory>
#include <stdexcept>
#include <map>
#include <unordered_map>
#include <vector>
#include <string>

#include <iostream>

// ----------------------------------------------------------------------------
namespace json
{

// ----------------------------------------------------------------------------
class value;
class object;
class array;

// ----------------------------------------------------------------------------
enum class value_type : int
{
  object,
  array,
  number,
  string,
  false_,
  null,
  true_
};

// ----------------------------------------------------------------------------
class value_base
{
public:
  value_base(value_type type) : type_(type)
  {
  }
public:
  value_type type() const
  {
    return type_;
  }
public:
  virtual void write(std::ostream& os) const = 0;
private:
  value_type type_;
};

// ----------------------------------------------------------------------------
template< class T >
std::shared_ptr<T> static_pointer_cast(const std::shared_ptr<value_base> v)
{
  if ( v->type() != T::static_value_type() ) {
    throw std::runtime_error("invalid json type cast");
  }
  return std::static_pointer_cast<T>(v);
}

// ----------------------------------------------------------------------------
class string : public value_base
{
public:
  string() : value_base(static_value_type())
  {
  }
public:
  string(const std::string& s)
    :
    value_base(static_value_type()),
    m_value(s)
  {
  }
public:
  string(std::string&& s)
    :
    value_base(static_value_type()),
    m_value(std::move(s))
  {
  }
public:
  static constexpr value_type static_value_type()
  {
    return value_type::string;
  }
public:
  const std::string& str() const { return m_value; }
public:
  void write(std::ostream& os) const;
private:
  std::string m_value;
};

// ----------------------------------------------------------------------------
class number : public value_base
{
public:
  number() : value_base(static_value_type())
  {
  }
public:
  number(double v)
    :
    value_base(static_value_type()),
    m_value(v)
  {
  }
public:
  static constexpr value_type static_value_type()
  {
    return value_type::number;
  }
public:
  double value() const { return m_value; }
public:
  void write(std::ostream& os) const;
private:
  double m_value;
};

// ----------------------------------------------------------------------------
class array : public value_base
{
public:
  typedef std::vector<value>::const_iterator value_iterator;
public:
  array() : value_base(static_value_type())
  {
  }
public:
  static constexpr value_type static_value_type()
  {
    return value_type::array;
  }
public:
  const json::value& at(size_t n)
  {
    return elms.at(n);
  }
public:
  size_t size() const;
public:
  void push_back(value v);
public:
  void write(std::ostream& os) const;
private:
  std::vector<value> elms;
};

// ----------------------------------------------------------------------------
class value
{
public:
  value() {}
  value(std::string&& s)
  {
    elm = std::make_shared<json::string>(json::string(std::move(s)));
  }
  value(const std::string& s)
  {
    elm = std::make_shared<json::string>(json::string(s));
  }
  value(double v)
  {
    elm = std::make_shared<json::number>(json::number(v));
  }
#if 0
  value(const object& o)
  {
    elm = std::make_shared<object>(o);
  }
#endif
  value(object&& o);
#if 0
  {
    elm = std::make_shared<object>(std::move(o));
  }
#endif
#if 0
  value(const array& a)
  {
    elm = std::make_shared<array>(a);
  }
#endif
  value(array&& a)
  {
    elm = std::make_shared<array>(std::move(a));
  }
public:
#if 0
  void operator=(const object& rhs)
  {
    elm = std::make_shared<object>(rhs);
  }
#endif
public:
  value_type type() const
  {
    return elm->type();
  }
public:
#if 0
  template< class T >
  std::shared_ptr<T> get() const
  {
    return static_pointer_cast<T>(elm);
  }
#endif
  template< class T >
  const T& get() const
  {
    return *static_pointer_cast<T>(elm);
  }
public:
  bool is_null()   const { return elm.get() == nullptr; }
  bool is_string() const { return elm->type() == value_type::string; }
  bool is_number() const { return elm->type() == value_type::number; }
  bool is_object() const { return elm->type() == value_type::object; }
  bool is_array()  const { return elm->type() == value_type::array; }
public:
  void write(std::ostream& os) const;
private:
  std::shared_ptr<value_base> elm;
public:
  friend std::ostream& operator<<(std::ostream& os, const value& v);
};

// ----------------------------------------------------------------------------
class object : public value_base
{
public:
  //typedef std::map<std::string, value>::const_iterator member_iterator;
  typedef std::unordered_map<std::string, value>::const_iterator member_iterator;
public:
  object() : value_base(static_value_type())
  {
  }
public:
  static constexpr value_type static_value_type()
  {
    return value_type::object;
  }
public:
  void set(std::string key, value v);
  void set(std::string key, std::string v);
  void set(std::pair<std::string, value>&& member);
  void set(std::string key, array a);
  void set(std::string key, object o);
public:
  bool has(std::string&& key);
public:
  const value& get(std::string&& key);
public:
  void write(std::ostream& os) const;
private:
  //std::map<std::string, value> elms;
  std::unordered_map<std::string, value> elms;
};

//
// Parser classes.
//

// ----------------------------------------------------------------------------
class value_parser_base
{
public:
  value_parser_base();
public:
  virtual size_t parse(const char* data, size_t data_len) = 0;
public:
  virtual bool complete() = 0;
public:
  virtual json::value value() = 0;
protected:
  std::unique_ptr<value_parser_base> m_value_parser;
};

// ----------------------------------------------------------------------------
class value_parser : public value_parser_base
{
public:
  value_parser() : m_state(state::initial)
  {
  }
public:
  size_t parse(const char* data, size_t data_len);
public:
  bool complete() { return m_state == state::complete; }
  void reset()    { m_state = state::initial; }
public:
  json::value value()
  {
    reset();
    return std::move(m_value);
  }
private:
  enum class state
  {
    initial,
    string,
    literal_or_number,
    array,
    object,
    complete,
  };
private:
  json::value m_value;
  state m_state;
};

// ----------------------------------------------------------------------------
class string_parser : public value_parser_base
{
public:
  string_parser() : m_state(state::initial)
  {
    m_value.reserve(20);
  }
public:
  size_t parse(const char* data, size_t data_len);
public:
  bool complete() { return m_state == state::complete; }
  void reset()    { m_state = state::initial; }
public:
  json::value value()
  {
    reset();
    return std::move(json::value(std::move(m_value)));
  }
private:
  enum class state
  {
    initial,
    escape,
    complete,
  };
private:
  std::string m_value;
  state m_state;
};

// ----------------------------------------------------------------------------
class literal_or_number_parser : public value_parser_base
{
public:
  literal_or_number_parser() : m_state(state::initial)
  {
  }
public:
  size_t parse(const char* data, size_t data_len);
public:
  bool complete() { return m_state == state::complete; }
  void reset()    { m_state = state::initial; }
public:
  json::value value()
  {
    reset();
    if ( m_value == "null" ) {
      return std::move(json::value());
    }
    else if ( m_value == "false" ) {
      throw std::runtime_error("false not implemented!");
    }
    else if ( m_value == "true" ) {
      throw std::runtime_error("true not implemented!");
    }
    else {
      return std::move(json::value(std::stod(m_value)));
    }
  }
private:
  enum class state
  {
    initial,
    value,
    complete,
  };
private:
  std::string m_value;
  state m_state;
};

// ----------------------------------------------------------------------------
class object_parser : public value_parser_base
{
public:
  object_parser() : m_state(state::initial)
  {
  }
public:
  size_t parse(const char* data, size_t data_len);
public:
  bool complete() { return m_state == state::complete; }
  void reset()    { m_state = state::initial; }
public:
  json::value value()
  {
    reset();
    return std::move(m_object);
  }
private:
  enum class state
  {
    initial,
    key,
    separator,
    value_begin,
    value_end,
    complete,
  };
private:
  std::pair<std::string, json::value> m_tmp_member;
  json::object m_object;
  state m_state;
};

// ----------------------------------------------------------------------------
class array_parser : public value_parser_base
{
public:
  array_parser() : m_state(state::initial)
  {
    m_value_parser.reset(new value_parser());
  }
public:
  size_t parse(const char* data, size_t data_len);
public:
  bool complete() { return m_state == state::complete; }
  void reset()    { m_state = state::initial; }
public:
  json::value value()
  {
    reset();
    return std::move(m_array);
  }
private:
  enum class state
  {
    initial,
    value_begin,
    value_end,
    complete,
  };
private:
  json::array m_array;
  state m_state;
};

// ----------------------------------------------------------------------------
class parser : public value_parser_base
{
public:
  parser(json::value& value);
public:
  size_t parse(const char* data, size_t data_len);
public:
  bool complete() { return m_state == state::complete; }
  void reset()    { m_state = state::initial; }
public:
  json::value value()
  {
    return m_value;
  }
private:
  enum class state
  {
    initial,
    object,
    array,
    complete
  };
private:
  json::value& m_value;
  state m_state;
};

// ----------------------------------------------------------------------------
std::string to_string(const json::string& s);
std::string to_string(const json::number& v);
std::string to_string(const json::object& o);
std::string to_string(const json::array& a);
std::string to_string(const value& v);

// ----------------------------------------------------------------------------
inline std::ostream& operator<<(std::ostream& os, const string& v)
{
  v.write(os);
  return os;
}

// ----------------------------------------------------------------------------
inline std::ostream& operator<<(std::ostream& os, const number& v)
{
  v.write(os);
  return os;
}

// ----------------------------------------------------------------------------
inline std::ostream& operator<<(std::ostream& os, const object& o)
{
  o.write(os);
  return os;
}

// ----------------------------------------------------------------------------
inline std::ostream& operator<<(std::ostream& os, const array& a)
{
  a.write(os);
  return os;
}

// ----------------------------------------------------------------------------
inline std::ostream& operator<<(std::ostream& os, const value& v)
{
#if 0
  switch ( v.type() )
  {
    case value_type::string:
      v.get<string>()->write(os); break;
    case value_type::number:
      v.get<number>()->write(os); break;
    case value_type::object:
      v.get<object>()->write(os); break;
    case value_type::array:
      v.get<array>()->write(os); break;
    default:
      throw std::runtime_error("invalid value type");
      break;
  }
#else
  v.elm->write(os);
  return os;
#endif
}

// ----------------------------------------------------------------------------
} // namespace json

// ----------------------------------------------------------------------------
#endif // __json__json_h__

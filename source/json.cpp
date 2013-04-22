// ----------------------------------------------------------------------------
//
//        Filename:  json.cpp
//
//          Author:  Benny Bach
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#include <json.h>

// ----------------------------------------------------------------------------
#include <sstream>

// ----------------------------------------------------------------------------
namespace json
{

// ----------------------------------------------------------------------------
static const std::string ws = " \t\n\r";
static const std::string structural = "{[]},:";

// ----------------------------------------------------------------------------
value::value(object&& o)
{
  elm = std::make_shared<object>(std::move(o));
}

// ----------------------------------------------------------------------------
void string::write(std::ostream& os) const
{
  //os << "\"" << m_value << "\"";
  os << to_string(*this);
}

// ----------------------------------------------------------------------------
void number::write(std::ostream& os) const
{
  os << m_value;
}

// ----------------------------------------------------------------------------
void object::set(std::string key, value v)
{
  elms[std::move(key)] = std::move(v);
}

// ----------------------------------------------------------------------------
void object::set(std::string key, std::string v)
{
  elms[std::move(key)] = value(std::move(v));
}

// ----------------------------------------------------------------------------
void object::set(std::pair<std::string, value>&& member)
{
  elms.insert(std::move(member));
}

// ----------------------------------------------------------------------------
void object::set(std::string key, array a)
{
  elms[std::move(key)] = value(std::move(a));
}

// ----------------------------------------------------------------------------
void object::set(std::string key, object o)
{
  elms[std::move(key)] = value(std::move(o));
}

// ----------------------------------------------------------------------------
bool object::has(std::string&& key)
{
  auto it = elms.find(key);
  return it != elms.end();
}

// ----------------------------------------------------------------------------
const value& object::get(std::string&& key)
{
  return elms[key];
}

// ----------------------------------------------------------------------------
void object::write(std::ostream& os) const
{
  os << "{";
  for ( member_iterator it = elms.begin(); it != elms.end(); )
  {
    //os << "\"" << (*it).first << "\"" << ":" << to_string((*it).second);
    os << "\"" << (*it).first << "\"" << ":" << (*it).second;
    if ( ++it != elms.end() )
    {
      os << ",";
    }
  }
  os << "}";
}

// ----------------------------------------------------------------------------
size_t array::size() const
{
  return elms.size();
}

// ----------------------------------------------------------------------------
void array::push_back(value v)
{
  elms.push_back(std::move(v));
}

// ----------------------------------------------------------------------------
void array::write(std::ostream& os) const
{
  os << "[";
  for ( value_iterator it = elms.begin(); it != elms.end(); )
  {
    //os << to_string(*it);
    os << *it;
    if ( ++it != elms.end() )
    {
      os << ",";
    }
  }
  os << "]";
}

//
// Parser classes.
//

// ----------------------------------------------------------------------------
value_parser_base::value_parser_base()
{
}

// ----------------------------------------------------------------------------
parser::parser(json::value& value)
  :
  m_value(value),
  m_state(state::initial)
{
}

// ----------------------------------------------------------------------------
size_t string_parser::parse(const char* data, size_t data_len)
{
  const char* pb = data;
  const char* pe = pb+data_len;

  while ( pb < pe && m_state != state::complete )
  {
    switch ( m_state )
    {
      case state::initial:
#if 0
        if ( *pb == '\\' ) {
          m_state = state::escape;
        }
        else if ( *pb == '"') {
          m_state = state::complete;
        }
        else {
          m_value += *pb;
          //m_value.push_back(*pb);
        }
#else
        switch ( *pb )
        {
          case '\\':
            m_state = state::escape; break;
          case '"':
            m_state = state::complete; break;
          default:
            m_value += *pb; break;
        }
#endif
        pb++;
        break;
      case state::escape:
        if ( *pb != 'u' ) {
          m_value += *pb;
          m_state = state::initial;
        }
        else {
          throw std::runtime_error("escaped unicode characters not implemented!");
        }
        pb++;
        break;
      case state::complete:
        break;
    }
  }
  return pb-data;
}

// ----------------------------------------------------------------------------
size_t literal_or_number_parser::parse(const char* data, size_t data_len)
{
  const char* pb = data;
  const char* pe = pb+data_len;

  while ( pb < pe && m_state != state::complete )
  {
    switch ( m_state )
    {
      case state::initial:
        if ( ws.find(*pb) == std::string::npos )
        {
          m_value += *pb;
          m_state = state::value;
        }
        pb++;
        break;
      case state::value:
        if ( ws.find(*pb) != std::string::npos ) {
          m_state = state::complete;
        }
        else if ( structural.find(*pb) != std::string::npos ) {
          m_state = state::complete;
        }
        else {
          m_value += *pb++;
        }
        break;
      case state::complete:
        break;
    }
  }
  return pb-data;
}

// ----------------------------------------------------------------------------
size_t object_parser::parse(const char* data, size_t data_len)
{
  const char* pb = data;
  const char* pe = pb+data_len;

  while ( pb < pe && m_state != state::complete )
  {
    switch ( m_state )
    {
      case state::initial:
        if ( ws.find(*pb) == std::string::npos )
        {
          if ( *pb == '"' ) {
            m_value_parser.reset(new string_parser());
            m_state = state::key;
          }
          else if ( *pb == '}' ) {
            m_state = state::complete;
          }
          else {
            throw std::runtime_error("syntax error");
          }
        }
        break;
      case state::key:
        pb += m_value_parser->parse(pb, pe-pb);
        if ( m_value_parser->complete() )
        {
          m_tmp_member.first = std::move(m_value_parser->value().get<string>().str());
          m_state = state::separator;
          // FALL THROUGH INTENDED.
        }
        else {
          break;
        }
      case state::separator:
        if ( ws.find(*pb) == std::string::npos )
        {
          if ( *pb == ':' ) {
            m_value_parser.reset(new value_parser());
            m_state = state::value_begin;
          }
          else {
            throw std::runtime_error("syntax error");
          }
        }
        break;
      case state::value_begin:
        pb += m_value_parser->parse(pb, pe-pb);
        if ( m_value_parser->complete() )
        {
          m_tmp_member.second = m_value_parser->value();
          m_object.set(std::move(m_tmp_member));
          if ( *pb == ',' ) {
            m_state = state::initial;
          }
          else if ( *pb == '}' ) {
            m_state = state::complete;
          }
          else {
            m_state = state::value_end;
          }
        }
        break;
      case state::value_end:
        if ( ws.find(*pb) == std::string::npos )
        {
          if ( *pb == ',') {
            m_state = state::initial;
          }
          else if ( *pb == '}' ) {
            m_state = state::complete;
          }
          else {
            throw std::runtime_error("syntax error");
          }
        }
        break;
      case state::complete:
        break;
    }
    pb++;
  }
  return pb-data;
}

// ----------------------------------------------------------------------------
size_t array_parser::parse(const char* data, size_t data_len)
{
  const char* pb = data;
  const char* pe = pb+data_len;

  while ( pb < pe && m_state != state::complete )
  {
    switch ( m_state )
    {
      case state::initial:
        if ( ws.find(*pb) == std::string::npos )
        {
          if ( *pb == ']' ) {
            m_state = state::complete;
            pb++;
            break;
          }
          else {
            m_state = state::value_begin;
            // FALL THROUGH INTENDED
          }
        }
      case state::value_begin:
        pb += m_value_parser->parse(pb, pe-pb);
        if ( m_value_parser->complete() )
        {
          m_array.push_back(std::move(m_value_parser->value()));
          if ( *pb == ',' ) {
            m_state = state::initial;
          }
          else if ( *pb == ']' ) {
            m_state = state::complete;
          }
          else {
            m_state = state::value_end;
          }
        }
        pb++;
        break;
      case state::value_end:
        if ( ws.find(*pb) == std::string::npos )
        {
          if ( *pb == ',') {
            m_state = state::initial;
          }
          else if ( *pb == ']' ) {
            m_state = state::complete;
          }
          else {
            throw std::runtime_error("syntax error");
          }
        }
        pb++;
        break;
      case state::complete:
        //m_value_parser.reset(new value_parser());
        break;
    }
    //pb++;
  }
  return pb-data;
}

// ----------------------------------------------------------------------------
size_t value_parser::parse(const char* data, size_t data_len)
{
  const char* pb = data;
  const char* pe = pb+data_len;

  while ( pb < pe && m_state != state::complete )
  {
    switch ( m_state )
    {
      case state::initial:
        if ( ws.find(*pb) == std::string::npos )
        {
          if ( *pb == '"' )
          {
            m_value_parser.reset(new string_parser());
            m_state = state::string;
            pb++;
            break;
          }
          else if ( *pb == '{' )
          {
            m_value_parser.reset(new object_parser());
            m_state = state::object;
            pb++;
            break;
          }
          else if ( *pb == '[' )
          {
            m_value_parser.reset(new array_parser());
            m_state = state::array;
            pb++;
            break;
          }
          else {
            // true|false|null ?
            m_value_parser.reset(new literal_or_number_parser());
            m_state = state::literal_or_number;
            // FALL THROUGH INTENDED.
          }
        }
        else {
          pb++;
          break;
        }
      case state::string:
      case state::literal_or_number:
      case state::array:
      case state::object:
        pb += m_value_parser->parse(pb, pe-pb);
        if ( m_value_parser->complete() )
        {
          m_value = m_value_parser->value();
          m_state = state::complete;
        }
        return pb-data;
        break;
      case state::complete:
        break;
    }
    //pb++;
  }
  return pb-data;
}

// ----------------------------------------------------------------------------
size_t parser::parse(const char* data, size_t data_len)
{
  const char* pb = data;
  const char* pe = pb+data_len;

  while ( pb < pe && m_state != state::complete )
  {
    switch ( m_state )
    {
      case state::initial:
        if ( ws.find(*pb) == std::string::npos )
        {
          if ( *pb == '{' )
          {
            m_value_parser.reset(new object_parser());
            m_state = state::object;
          }
          else if ( *pb == '[' )
          {
            m_value_parser.reset(new array_parser());
            m_state = state::array;
          }
          else {
            // Error!
          }
        }
        pb++;
        break;
      case state::object:
      case state::array:
        pb += m_value_parser->parse(pb, pe-pb);
        if ( m_value_parser->complete() )
        {
          m_value = m_value_parser->value();
          m_state = state::complete;
        }
        break;
      case state::complete:
        break;
    }
    //pb++;
  }
  return pb-data;
}

// ----------------------------------------------------------------------------
std::string to_string(const json::string& s)
{
#if 0
  std::stringstream ss;
  ss << "\"" << s.str() << "\"";
  return std::move(ss.str());
#endif
    std::string result;

    result.reserve(s.str().length()+2);

    result += "\"";
    for ( auto c : s.str() )
    {
      switch ( c )
      {
        case '"':
        case '\\':
          result += '\\';
          // FALL THROUGH INTENEDED
        default:
          result += c;
      }
    }
    result += "\"";

    return std::move(result);
}

// ----------------------------------------------------------------------------
std::string to_string(const json::number& v)
{
  std::stringstream ss;
  ss << v.value();
  return std::move(ss.str());
}

// ----------------------------------------------------------------------------
std::string to_string(const json::object& o)
{
  std::stringstream ss;
  ss << o;
  return std::move(ss.str());
}

// ----------------------------------------------------------------------------
std::string to_string(const json::array& a)
{
  std::stringstream ss;
  ss << a;
  return std::move(ss.str());
}

// ----------------------------------------------------------------------------
std::string to_string(const value& v)
{
  std::stringstream ss;
  ss << v;
  return std::move(ss.str());
}

// ----------------------------------------------------------------------------
} // namespace json

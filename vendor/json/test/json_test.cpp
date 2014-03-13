// ----------------------------------------------------------------------------
#define CATCH_CONFIG_MAIN
#include "catch.hpp"
// ----------------------------------------------------------------------------
#include <json/json.h>

// ----------------------------------------------------------------------------
TEST_CASE( "default-constructed-value-is-null", "[value]" )
{
  json::value v;

  REQUIRE( v.is_null() == true );
}

// ----------------------------------------------------------------------------
TEST_CASE( "construct-number", "[value]" )
{
  json::value v1{1.234};
  json::value v2 = 10;

  REQUIRE( v1.as_number() == 1.234 );
  REQUIRE( v2.as_number() == 10 );
}

// ----------------------------------------------------------------------------
TEST_CASE( "construct-object-by-initializer-list", "[object]" )
{
  json::object v{ { "x", "y" }, { "t", "u" } };

  REQUIRE( v["x"].is_string() == true );
  REQUIRE( v["x"].as_string() == "y" );
  REQUIRE( v["t"].is_string() == true );
  REQUIRE( v["t"].as_string() == "u" );
}

// ----------------------------------------------------------------------------
TEST_CASE( "construct-array-by-initializer-list", "[array]" )
{
  json::array v{ "x", "y" };

  REQUIRE( v[0].is_string() == true );
  REQUIRE( v[0].as_string() == "x" );
  REQUIRE( v[1].is_string() == true );
  REQUIRE( v[1].as_string() == "y" );
}

// ----------------------------------------------------------------------------
TEST_CASE( "array-range-based-for", "[array]" )
{
  json::array arr;

  arr.push_back(1);
  arr.push_back(1);
  arr.push_back(1);

  REQUIRE( arr.size() == 3 );

  for ( auto& v : arr )
  {
    REQUIRE( v.is_number() );
    REQUIRE( v.as_number() == 1 );
  }
}

// ----------------------------------------------------------------------------
TEST_CASE( "string-escape", "[array]" )
{
  // char = unescaped /
  //        escape (
  //            %x22 /          ; "    quotation mark  U+0022
  //            %x5C /          ; \    reverse solidus U+005C
  //            %x2F /          ; /    solidus         U+002F
  //            %x62 /          ; b    backspace       U+0008
  //            %x66 /          ; f    form feed       U+000C
  //            %x6E /          ; n    line feed       U+000A
  //            %x72 /          ; r    carriage return U+000D
  //            %x74 /          ; t    tab             U+0009
  //            %x75 4HEXDIG )  ; uXXXX                U+XXXX

  json::array v1{ "a\"b\"c", "a\\bc", "a/bc", "a\bbc", "a\fbc", "a\nbc", "a\rbc", "a\tbc" };

  REQUIRE( v1[0].is_string() == true );
  REQUIRE( v1[0].as_string() == "a\"b\"c" );

  REQUIRE( v1[1].is_string() == true );
  REQUIRE( v1[1].as_string() == "a\\bc" );

  REQUIRE( v1[2].is_string() == true );
  REQUIRE( v1[2].as_string() == "a/bc" );

  REQUIRE( v1[3].is_string() == true );
  REQUIRE( v1[3].as_string() == "a\bbc" );

  REQUIRE( v1[4].is_string() == true );
  REQUIRE( v1[4].as_string() == "a\fbc" );

  REQUIRE( v1[5].is_string() == true );
  REQUIRE( v1[5].as_string() == "a\nbc" );

  REQUIRE( v1[6].is_string() == true );
  REQUIRE( v1[6].as_string() == "a\rbc" );

  REQUIRE( v1[7].is_string() == true );
  REQUIRE( v1[7].as_string() == "a\tbc" );

  std::stringstream os;
  os << v1;

  REQUIRE( os.str() == "[\"a\\\"b\\\"c\",\"a\\\\bc\",\"a\\/bc\",\"a\\bbc\",\"a\\fbc\",\"a\\nbc\",\"a\\rbc\",\"a\\tbc\"]" );
}

// ----------------------------------------------------------------------------
TEST_CASE( "parse-empty-array", "[parser]" )
{
  json::value  value;
  json::parser parser(value);

  const char buf[] = "[]";

  size_t consumed = parser.parse(buf, sizeof(buf)-1);

  REQUIRE( consumed == sizeof(buf)-1 );
  REQUIRE( parser.complete() == true );

  REQUIRE( value.is_array() == true );
}

// ----------------------------------------------------------------------------
TEST_CASE( "parse-array", "[parser]" )
{
  json::value  value;
  json::parser parser(value);

  const char buf[] = "[ null, true, false, \"\", 0 ]";

  size_t consumed = parser.parse(buf, sizeof(buf)-1);

  REQUIRE( consumed == sizeof(buf)-1 );
  REQUIRE( parser.complete() == true );

  REQUIRE( value.is_array() == true );

  json::array arr = value.as_array();

  REQUIRE( arr[0].is_null() == true );
  REQUIRE( arr[1].is_true() == true );
  REQUIRE( arr[2].is_false() == true );
  REQUIRE( arr[3].is_string() == true );
  REQUIRE( arr[3].as_string() == "" );
  REQUIRE( arr[4].is_number() == true );
  REQUIRE( arr[4].as_number() == 0 );
}

// ----------------------------------------------------------------------------
TEST_CASE( "parse-array-numbers", "[parser]" )
{
  json::value  value;
  json::parser parser(value);

  const char buf[] = "[ 1.234, 1e3, -234E-3 ]";

  size_t consumed = parser.parse(buf, sizeof(buf)-1);

  REQUIRE( consumed == sizeof(buf)-1 );
  REQUIRE( parser.complete() == true );

  REQUIRE( value.is_array() == true );

  json::array arr = value.as_array();

  REQUIRE( arr[0].is_number() == true );
  REQUIRE( arr[0].as_number() == 1.234 );

  REQUIRE( arr[1].is_number() == true );
  REQUIRE( arr[1].as_number() == 1000 );

  REQUIRE( arr[2].is_number() == true );
  REQUIRE( arr[2].as_number() == -0.234 );
}

// ----------------------------------------------------------------------------
TEST_CASE( "parse-simple-array", "[parser]" )
{
  json::value  value;
  json::parser parser(value);

  const char buf[] = "[ \"x\" ]";

  size_t consumed = parser.parse(buf, sizeof(buf)-1);

  REQUIRE( consumed == sizeof(buf)-1 );
  REQUIRE( parser.complete() == true );

  REQUIRE( value.is_array() == true );

  json::array arr = value.as_array();

  REQUIRE( arr[0].is_string() == true );
  REQUIRE( arr[0].as_string() == "x" );
}

// ----------------------------------------------------------------------------
TEST_CASE( "parse-nested-array", "[parser]" )
{
  json::value  value;
  json::parser parser(value);

  const char buf[] = "[ [ \"x\" ], [ 1.234 ] ]";

  size_t consumed = parser.parse(buf, sizeof(buf)-1);

  REQUIRE( consumed == sizeof(buf)-1 );
  REQUIRE( parser.complete() == true );

  REQUIRE( value.is_array() == true );

  json::array arr = value.as_array();

  REQUIRE( arr[0].is_array() == true );
  REQUIRE( arr[1].is_array() == true );
}

// ----------------------------------------------------------------------------
TEST_CASE( "parse-empty-object", "[parser]" )
{
  json::value  value;
  json::parser parser(value);

  const char buf[] = "{}";

  size_t consumed = parser.parse(buf, sizeof(buf)-1);

  REQUIRE( consumed == sizeof(buf)-1 );
  REQUIRE( parser.complete() == true );

  REQUIRE( value.is_object() == true );
}

// ----------------------------------------------------------------------------
TEST_CASE( "parse-simple-object", "[parser]" )
{
  json::value  value;
  json::parser parser(value);

  const char buf[] = "{ \"x\" : \"y\", \"n\" : 1.234, \"z\" : null }";

  size_t consumed = parser.parse(buf, sizeof(buf)-1);

  REQUIRE( consumed == sizeof(buf)-1 );
  REQUIRE( parser.complete() == true );

  REQUIRE( value.is_object() == true );

  json::object obj = value.as_object();

  REQUIRE( obj["x"].is_string() == true );
  REQUIRE( obj["x"].as_string() == "y" );

  REQUIRE( obj["n"].is_number() == true );
  REQUIRE( obj["n"].as_number() == 1.234 );

  REQUIRE( obj["z"].is_null() == true );
}

// ----------------------------------------------------------------------------
TEST_CASE( "parse-object-with-array-value", "[parser]" )
{
  json::value  value;
  json::parser parser(value);

  const char buf[] = "{ \"x\" : [ \"y\", 1.234 ] }";

  size_t consumed = parser.parse(buf, sizeof(buf)-1);

  REQUIRE( consumed == sizeof(buf)-1 );
  REQUIRE( parser.complete() == true );

  REQUIRE( value.is_object() == true );

  json::object obj = value.as_object();

  REQUIRE( obj["x"].is_array() == true );
  //REQUIRE( obj["x"].as_string() == "y" );
}

// ----------------------------------------------------------------------------
TEST_CASE( "parse-scattered-buffers", "[parser]" )
{
  json::value  value;
  json::parser parser(value);

  const char buf[] = "{ \"x\" : [ \"y\", 1.234 ] }";

  // Parser buffer one character at a time.
  for ( int i=0; i<sizeof(buf)-2; i++ )
  {
    size_t consumed = parser.parse(buf+i, 1);
    REQUIRE( consumed == 1 );
    REQUIRE( parser.complete() == false );
  }

  size_t consumed = parser.parse(buf+sizeof(buf)-2, 1);

  REQUIRE( consumed == 1 );
  REQUIRE( parser.complete() == true );

  REQUIRE( value.is_object() == true );

  json::object obj = value.as_object();

  REQUIRE( obj["x"].is_array() == true );
  //REQUIRE( obj["x"].as_string() == "y" );
}

// ----------------------------------------------------------------------------
TEST_CASE( "parse-raw-buffer", "[parser]" )
{
  json::value  doc;
  json::parser parser(doc);

  unsigned char s[] = {
    123, 34, 106, 115, 111, 110, 114, 112, 99, 34, 58, 34, 50, 46, 48,
    34, 44, 34, 109, 101, 116, 104, 111, 100, 34, 58, 34, 101, 99, 104,
    111, 34, 44, 34, 112, 97, 114, 97, 109, 115, 34, 58, 91, 34, 72, 101,
    108, 108, 195, 152, 32, 74, 83, 79, 78, 45, 82, 80, 67, 34, 93, 44,
    34, 105, 100, 34, 58, 49, 125
  };

  size_t consumed = parser.parse(reinterpret_cast<char *>(s), sizeof(s));

  REQUIRE( consumed == sizeof(s) );
  REQUIRE( parser.complete() == true );
}

// ----------------------------------------------------------------------------
TEST_CASE( "parse-string-escape", "[parser]" )
{
  json::value  value;
  json::parser parser(value);

  const char buf[] = "[ \"a\\\"b\\\"c\" ]";

  size_t consumed = parser.parse(buf, sizeof(buf)-1);

  REQUIRE( consumed == sizeof(buf)-1 );
  REQUIRE( parser.complete() == true );

  REQUIRE( value.is_array() == true );

  json::array arr = value.as_array();

  REQUIRE( arr[0].is_string() == true );
  REQUIRE( arr[0].as_string() == "a\"b\"c" );
}

// ----------------------------------------------------------------------------
TEST_CASE( "parse-error-intial", "[parser]" )
{
  json::value  value;
  json::parser parser(value);

  const char buf[] = "-";

  try
  {
    size_t consumed = parser.parse(buf, sizeof(buf)-1);
    FAIL("no exception?");
  }
  catch(const json::error& e)
  {
    CAPTURE( e.what() )
    SUCCEED( "" );
  }

  //REQUIRE_THROWS_AS(parser.parse(buf, sizeof(buf)-1), json::error);


}
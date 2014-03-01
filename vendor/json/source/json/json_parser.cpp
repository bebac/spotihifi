// ----------------------------------------------------------------------------
#include <json/json_parser.h>
#include <json/json_error.h>
// ----------------------------------------------------------------------------
#include <iostream>
#include <utility>
#include <stdexcept>
// ----------------------------------------------------------------------------
namespace json
{
  // Skip whitespace, return true if pb < pe (not at the end)
  static bool skip_ws(const char*& pb, const char* pe)
  {
    while ( pb < pe )
    {
      if ( *pb == ' ' || *pb == '\n' || *pb == '\t'  || *pb == '\r' )
        pb++;
      else
        return true;
    }
    return false;
  }

  static bool skip_comment(const char*& pb, const char* pe)
  {
    if ( pb+1 >= pe )
      return true;

    if ( *pb != '/' || *(pb+1) != '/' )
      return true;

    pb += 2;

    while ( pb < pe )
    {
      if ( *pb != '\n' && *pb != '\r' )
        pb++;
      else
        break;
    }
    return skip_ws(pb, pe);
  }

  static bool skip_ws_n_comments(const char*& pb, const char* pe)
  {
    if ( skip_ws(pb, pe) ) {
      return skip_comment(pb, pe);
    }
    else {
      return false;
    }
  }

  parser_state_base::parser_state_base(parser& parser, state initial_state)
    :
    parser_(parser),
    state_(initial_state)
  {
  }

  void parser_state_base::push_state(parser_state_base* state)
  {
    parser_.push_state(state);
  }

  class state_string : public parser_state_base
  {
  public:
    state_string(parser& parser) : parser_state_base(parser), value_()
    {
      value_.reserve(32);
    }
  public:
    size_t parse(const char* begin, const char* end);
  public:
    virtual json::value value() { return std::move(json::value(std::move(value_))); }
  private:
    std::string value_;
  };

  class state_number : public parser_state_base
  {
  public:
    state_number(parser& parser) : parser_state_base(parser), value_() {}
  public:
    size_t parse(const char* begin, const char* end);
  public:
    virtual json::value value() { return std::move(json::value(std::stod(value_))); }
  private:
    std::string value_;
  };

  class state_null : public parser_state_base
  {
  public:
    state_null(parser& parser) : parser_state_base(parser, state::null_n), value_() {}
  public:
    size_t parse(const char* begin, const char* end);
  public:
    virtual json::value value() { return std::move(value_); }
  private:
    json::value value_;
  };

  class state_true : public parser_state_base
  {
  public:
    state_true(parser& parser) : parser_state_base(parser, state::true_t), value_(true) {}
  public:
    size_t parse(const char* begin, const char* end);
  public:
    virtual json::value value() { return std::move(value_); }
  private:
    json::value value_;
  };

  class state_false : public parser_state_base
  {
  public:
    state_false(parser& parser) : parser_state_base(parser, state::false_f), value_(false) {}
  public:
    size_t parse(const char* begin, const char* end);
  public:
    virtual json::value value() { return std::move(value_); }
  private:
    json::value value_;
  };

  class state_value : public parser_state_base
  {
  public:
    state_value(parser& parser) : parser_state_base(parser), value_() {}
  public:
    size_t parse(const char* begin, const char* end);
  public:
    virtual json::value value() { return std::move(value_); }
  public:
    virtual void accept(json::value& v)
    {
      value_ = std::move(v);
      state_ = state::complete;
    }
  private:
    json::value value_;
  };

  class state_array : public parser_state_base
  {
  public:
    state_array(parser& parser) : parser_state_base(parser), value_(json::array()) {}
  public:
    size_t parse(const char* begin, const char* end);
  public:
    virtual json::value value() { return std::move(value_); }
  public:
    virtual void accept(json::value& v)
    {
      assert(value_.is_array());
      value_.as_array().push_back(std::move(v));
      state_ = state::array_next_value_or_end;
    }
  private:
    json::value value_;
  };

  class state_object : public parser_state_base
  {
  public:
    state_object(parser& parser) : parser_state_base(parser), value_(json::object()) {}
  public:
    size_t parse(const char* begin, const char* end);
  public:
    virtual json::value value() { return std::move(value_); }
  public:
    virtual void accept(json::value& v)
    {
      assert(value_.is_object());
      switch ( state_ )
      {
        case state::object_member_key:
          key_ = v.move_string();
          state_ = state::object_member_sep;
          break;
        case state::object_member_val:
          value_.as_object().member(std::move(key_), std::move(v));
          state_ = state::object_next_member_or_end;
          break;
        default:
          assert(false);
          break;
      }
    }
  private:
    std::string key_; // tmp storage for member key.
    json::value value_;
  };

  class state_initial : public parser_state_base
  {
  public:
    state_initial(parser& parser) : parser_state_base(parser), value_() {}
  public:
    virtual size_t parse(const char* pb, const char* pe);
  public:
    virtual void accept(json::value& v)
    {
      value_ = std::move(v);
      state_ = state::complete;
    }
  public:
    virtual json::value value() { return std::move(value_); }
  private:
    json::value value_;
  };

  //
  // Parser
  //

  parser::parser(json::value& value)
    :
    value_(value),
    stack_()
  {
    push_state(new state_initial(*this));
  }

  size_t parser::parse(const char* data, size_t data_len)
  {
    const char* end    = data+data_len;
    size_t      offset = 0;

    while ( data+offset < end )
    {
      parser_state_base* top = stack_.top().get();

      offset += top->parse(data+offset, end);

      if ( top->complete() )
      {
        if ( stack_.size() > 1 )
        {
          // Get value from top parser.
          json::value v = std::move(top->value());

          // pop off completed parser.
          stack_.pop();
          top = stack_.top().get();

          // pass completed value to new stack top.
          top->accept(v);
        }
        else {
          break;
        }
      }
    }

    // Set the root value on completion.
    if ( complete() ) {
      value_ = std::move(stack_.top().get()->value());
    }

    return offset;
  }

  bool parser::complete() const noexcept
  {
    return stack_.size() == 1 && stack_.top().get()->complete();
  }

  void parser::push_state(parser_state_base* state)
  {
    stack_.push(state_ptr(state));
  }

  //
  // String
  //

  size_t state_string::parse(const char* begin, const char* end)
  {
    const char* it = begin;

    while ( it < end && state_ != state::complete )
    {
      //std::cout << "state_string *it='" << *it << "'" << std::endl;
      switch ( state_ )
      {
        case state::initial:
          switch ( *it )
          {
            case '"':
              state_ = state::complete;
              break;
            case '\\':
              state_ = state::string_escape;
              break;
            default:
              value_.push_back(*it);
              break;
          }
          it++;
          break;
        case state::string_escape:
          switch ( *it )
          {
            case '"':
            case '\\':
            case '/':
            case '\b':
            case '\f':
            case '\n':
            case '\r':
            case '\t':
              value_.push_back(*it);
              state_ = state::initial;
              break;
            case 'u':
              assert(false);
              break;
            default:
              throw error("error reading string,  invalid escaping");
              break;
          }
          it++;
          break;
        case state::complete:
          break;
        default:
          assert(false);
          break;
      }
    }
    return it-begin;
  }

  //
  // number
  //

  size_t state_number::parse(const char* begin, const char* end)
  {
    const char* it = begin;

    while ( it < end && state_ != state::complete )
    {
      //std::cout << "state_number *it='" << *it << "'" << std::endl;
      switch ( state_ )
      {
        case state::initial:
          switch ( *it )
          {
            case 'E':
            case 'e':
            case '.':
            case '+':
            case '-':
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
              value_.push_back(*it++);
              break;
            case '\n':
            case '\r':
            case '\t':
            case ' ':
            case ',':
            case '}':
            case ']':
              state_ = state::complete;
              break;
            default:
              throw error("error reading number");
              break;
          }
          break;
        case state::complete:
          break;
        default:
          assert(false);
          break;
      }
    }
    return it-begin;
  }

  //
  // Null
  //

  size_t state_null::parse(const char* begin, const char* end)
  {
    const char* it = begin;

    while ( it < end && state_ != state::complete )
    {
      //std::cout << "state_null *it='" << *it << "'" << std::endl;
      switch ( state_ )
      {
        case state::null_n:
          if ( *it == 'n' )
            state_ = state::null_u;
          else
            throw error("error reading null, expected, 'n'");
          break;
        case state::null_u:
          if ( *it == 'u' )
            state_ = state::null_l1;
          else
            throw error("error reading null, expected, 'u'");
          break;
        case state::null_l1:
          if ( *it == 'l' )
            state_ = state::null_l2;
          else
            throw error("error reading null, expected, 'l'");
          break;
        case state::null_l2:
          if ( *it == 'l' )
            state_ = state::complete;
          else
            throw error("error reading null, expected, 'l'");
          break;
        case state::complete:
          break;
        default:
          assert(false);
          break;
      }
      it++;
    }
    return it-begin;
  }

  //
  // True
  //

  size_t state_true::parse(const char* begin, const char* end)
  {
    const char* it = begin;

    while ( it < end && state_ != state::complete )
    {
      //std::cout << "state_true *it='" << *it << "'" << std::endl;
      switch ( state_ )
      {
        case state::true_t:
          if ( *it == 't' )
            state_ = state::true_r;
          else
            throw error("error reading true, expected 't'");
          break;
        case state::true_r:
          if ( *it == 'r' )
            state_ = state::true_u;
          else
            throw error("error reading true, expected 'r'");
          break;
        case state::true_u:
          if ( *it == 'u' )
            state_ = state::true_e;
          else
            throw error("error reading true, expected 'u'");
          break;
        case state::true_e:
          if ( *it == 'e' )
            state_ = state::complete;
          else
            throw error("error reading true, expected 'e'");
          break;
        case state::complete:
          break;
        default:
          assert(false);
          break;
      }
      it++;
    }
    return it-begin;
  }

  //
  // False
  //

  size_t state_false::parse(const char* begin, const char* end)
  {
    const char* it = begin;

    while ( it < end && state_ != state::complete )
    {
      //std::cout << "state_false *it='" << *it << "'" << std::endl;
      switch ( state_ )
      {
        case state::false_f:
          if ( *it == 'f' )
            state_ = state::false_a;
          else
            throw error("error reading false, expected 'f'");
          break;
        case state::false_a:
          if ( *it == 'a' )
            state_ = state::false_l;
          else
            throw error("error reading false, expected 'a'");
          break;
        case state::false_l:
          if ( *it == 'l' )
            state_ = state::false_s;
          else
            throw error("error reading false, expected 'l'");
          break;
        case state::false_s:
          if ( *it == 's' )
            state_ = state::false_e;
          else
            throw error("error reading false, expected 's'");
          break;
        case state::false_e:
          if ( *it == 'e' )
            state_ = state::complete;
          else
            throw error("error reading false, expected 'e'");
          break;
        case state::complete:
          break;
        default:
          assert(false);
          break;
      }
      it++;
    }
    return it-begin;
  }
  //
  // Value
  //

  size_t state_value::parse(const char* begin, const char* end)
  {
    const char* it = begin;

    while ( it < end && state_ != state::complete )
    {
      switch ( state_ )
      {
        case state::initial:
          if ( skip_ws_n_comments(it, end) )
          {
            //std::cout << "state_value *it='" << *it << "'" << std::endl;
            switch ( *it )
            {
              case '[':
                push_state(new state_array(parser_));
                return it-begin+1;
              case '{':
                push_state(new state_object(parser_));
                return it-begin+1;
              case '"':
                push_state(new state_string(parser_));
                return it-begin+1;
              case 't':
                push_state(new state_true(parser_));
                return it-begin;
              case 'f':
                push_state(new state_false(parser_));
                return it-begin;
              case 'n':
                push_state(new state_null(parser_));
                return it-begin;
              case '-':
              case '0':
              case '1':
              case '2':
              case '3':
              case '4':
              case '5':
              case '6':
              case '7':
              case '8':
              case '9':
                push_state(new state_number(parser_));
                return it-begin;
              default:
                throw error("error reading value");
                break;
            }
            it++;
          }
          break;
        case state::complete:
          break;
        default:
          assert(false);
          break;
      }
    }
    return it-begin;
  }

  //
  // Array
  //

  size_t state_array::parse(const char* begin, const char* end)
  {
    const char* it = begin;

    while ( it < end && state_ != state::complete )
    {
      //std::cout << "state_array *it='" << *it << "'" << std::endl;
      switch ( state_ )
      {
        case state::initial:
          if ( skip_ws_n_comments(it, end) )
          {
            switch ( *it )
            {
              case ']': // empty array
                state_ = state::complete;
                break;
              default:
                push_state(new state_value(parser_));
                return it-begin;
            }
            it++;
          }
          break;
        case state::array_next_value_or_end:
          if ( skip_ws_n_comments(it, end) )
          {
            switch ( *it )
            {
              case ']': // end array
                state_ = state::complete;
                break;
              case ',':
                push_state(new state_value(parser_));
                return it-begin+1;
              default:
                throw error("error reading array, expected ']' or ','");
                break;
            }
            it++;
          }
          break;
        case state::complete:
          break;
        default:
          assert(false);
          break;
      }
    }
    return it-begin;
  }

  //
  // Object
  //

  size_t state_object::parse(const char* begin, const char* end)
  {
    const char* it = begin;

    while ( it < end && state_ != state::complete )
    {
      //std::cout << "state_object *it='" << *it << "'" << std::endl;
      switch ( state_ )
      {
        case state::initial:
          if ( skip_ws_n_comments(it, end) )
          {
            switch ( *it )
            {
              case '}': // empty object
                state_ = state::complete;
                break;
              case '"':
                push_state(new state_string(parser_));
                state_ = state::object_member_key;
                return it-begin+1;
              default:
                throw error("error reading object, expected '}' or '\"'");
                break;
            }
            it++;
          }
          break;
        case state::object_member_key:
          assert(false);
          break;
        case state::object_member_sep:
          if ( skip_ws_n_comments(it, end) )
          {
            switch ( *it )
            {
              case ':':
                push_state(new state_value(parser_));
                state_ = state::object_member_val;
                return it-begin+1;
              default:
                throw error("error reading object, expected ':'");
                break;
            }
            it++;
          }
          break;
        case state::object_member_val:
          assert(false);
          break;
        case state::object_next_member_or_end:
          if ( skip_ws_n_comments(it, end) )
          {
            switch ( *it )
            {
              case '}': // end object
                state_ = state::complete;
                break;
              case ',':
                state_ = state::object_member_key_begin;
                break;
              default:
                throw error("error reading object, expected '}' or ','");
                break;
            }
            it++;
          }
          break;
        case state::object_member_key_begin:
          if ( skip_ws_n_comments(it, end) )
          {
            switch ( *it )
            {
              case '"':
                push_state(new state_string(parser_));
                state_ = state::object_member_key;
                return it-begin+1;
              default:
                throw error("error reading object, expected '\"'");
                break;
            }
            it++;
          }
          break;
        case state::complete:
          break;
        default:
          assert(false);
          break;
      }
    }
    return it-begin;
  }

  //
  // Initial
  //

  size_t state_initial::parse(const char* begin, const char* end)
  {
    const char* it = begin;

    while ( it < end && state_ != state::complete )
    {
      //std::cout << "state_initial *it='" << *it << "'" << std::endl;
      switch ( state_ )
      {
        case state::initial:
          if ( skip_ws_n_comments(it, end) )
          {
            switch ( *it )
            {
            case '{':
              push_state(new state_object(parser_));
              return it-begin+1;
            case '[':
              push_state(new state_array(parser_));
              return it-begin+1;
            default:
              throw error("JSON text must start with '{' or '['" );
              break;
            }
            it++;
          }
          break;
        case state::complete:
          break;
      default:
        assert(false);
        break;
      }
    }
    return it-begin;
  }

}
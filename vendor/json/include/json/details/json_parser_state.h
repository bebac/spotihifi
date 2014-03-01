// ----------------------------------------------------------------------------
#ifndef __json__json_parser_state_h__
#define __json__json_parser_state_h__
// ----------------------------------------------------------------------------
#include <json/json_value.h>
// ----------------------------------------------------------------------------
namespace json
{
  class parser;

  class parser_state_base
  {
  protected:
    enum class state
    {
      initial,
      // string
      string_escape,
      // null
      null_n,
      null_u,
      null_l1,
      null_l2,
      // true
      true_t,
      true_r,
      true_u,
      true_e,
      // false
      false_f,
      false_a,
      false_l,
      false_s,
      false_e,
      // array
      array_next_value_or_end,
      // object
      object_member_key,
      object_member_sep,
      object_member_val,
      object_next_member_or_end,
      object_member_key_begin,
      complete
    };
  public:
    parser_state_base(parser& paser, state initial_state=state::initial);
  public:
    virtual ~parser_state_base() {}
  public:
    virtual size_t parse(const char* pb, const char* pe) = 0;
  public:
    virtual json::value value() = 0;
  public:
    virtual void accept(json::value& v) { assert(false); }
  public:
    bool complete() const noexcept { return state_ == state::complete; }
  protected:
    void push_state(parser_state_base* state);
  protected:
    parser& parser_;
    state state_;
  };
}
// ----------------------------------------------------------------------------
#endif // __json__json_parser_state_h__
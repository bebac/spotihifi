// ----------------------------------------------------------------------------
#ifndef __json__array_h__
#define __json__array_h__
// ----------------------------------------------------------------------------
#include <json/json_value.h>
// ----------------------------------------------------------------------------
#include <vector>
#include <iostream>
// ----------------------------------------------------------------------------
namespace json
{
  class array
  {
  public:
    using elements = std::vector<value>;
  public:
    array() : value_()
    {
      value_.reserve(5);
    }
  public:
    array(const array& other) : value_(other.value_) {}
  public:
    array(array&& other) : value_(std::move(other.value_)) {}
  public:
    array(std::initializer_list<elements::value_type> v) : value_(v) {}
  public:
    virtual ~array() {}
  public:
    size_t size() { return value_.size(); }
  public:
    template <typename V> void push_back(V v)
    {
      value_.push_back(std::forward<V>(v));
    }
  public:
    //value at(size_t index) { return value_[index]; }
    const value& operator[](size_t index) { return value_[index]; }
  public:
    elements::iterator begin() { return value_.begin(); }
    elements::iterator end()   { return value_.end(); }
  public:
    virtual void write(std::ostream& os) const;
  private:
    elements::const_iterator begin(const elements& v) const { return v.begin(); }
    elements::const_iterator end(const elements& v) const { return v.end(); }
  private:
    elements value_;
  };

  inline void array::write(std::ostream& os) const
  {
    os << "[";

    for ( elements::const_iterator it = begin(value_); it != end(value_); )
    {
      os << (*it);

      if ( ++it != end(value_) ) {
        os << ",";
      }
    }
    os << "]";
  }
}

// ----------------------------------------------------------------------------
inline std::ostream& operator<<(std::ostream& os, const json::array& v)
{
  v.write(os);
  return os;
}


// ----------------------------------------------------------------------------
#endif // __json__array_h__

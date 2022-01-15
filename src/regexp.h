#pragma once

#include "regexp_impl.h"
#include "regexp_impl_s.h"
#include <sstream>

namespace regexp
{
//! The regular expression over a type
template <typename T>
class RegExp
{
public:
  //! Zero
  static const RegExp zero;

  //! One
  static const RegExp one;

  //! Literal
  RegExp(const T &l)
      : rex_{std::make_shared<regexp_impl::Lit<T>>(l)} {};

  //! Product
  RegExp(const std::vector<T> &w)
      : rex_{std::make_shared<regexp_impl::Prd<T>>(w)} {};

  //! Sum
  RegExp(const std::unordered_set<T> &a)
      : rex_{std::make_shared<regexp_impl::Sum<T>>(a)} {};

  //! Copy constructor
  RegExp(const RegExp<T> &r)
      : rex_{r.rex_} {};

  //! Implementation pointer constructor
  RegExp(regexp_impl::rex_ptr_t<T> rex)
      : rex_{rex} {};

  //! Get pointer to implementation
  const regexp_impl::rex_ptr_t<T> getImplementationPointer() const
  {
    return rex_;
  };

  //! String representation
  void print(std::ostream &os) const { rex_->print(os); }

  //! String representation
  const std::string to_string() const
  {
    std::stringstream ss;
    rex_->print(ss);
    return ss.str();
  };

  //! Hash
  size_t hash() const { return rex_->hash(); };

  //! Assignment
  RegExp<T> &operator=(const RegExp<T> &r)
  {
    rex_ = r.rex_;
    return *this;
  };

  //! Equality
  bool operator==(const RegExp<T> &r) const { return rex_->equal(r.rex_); };
  bool operator!=(const RegExp<T> &r) const { return !rex_->equal(r.rex_); };

  //! Product (concatenation)
  const RegExp<T> operator*(const RegExp<T> &r) const
  {
    return RegExp<T>{regexp_impl::Prd<T>::make(rex_, r.rex_)};
  };

  //! Product (compound assignment)
  RegExp<T> &operator*=(const RegExp<T> &r)
  {
    rex_ = regexp_impl::Prd<T>::make(rex_, r.rex_);
    return *this;
  };

  //! Sum  (alternation,logical or)
  const RegExp<T> operator+(const RegExp<T> &r) const
  {
    return RegExp<T>{regexp_impl::Sum<T>::make(rex_, r.rex_)};
  };

  //! Sum (compound assignment)
  RegExp<T> &operator+=(const RegExp<T> &r)
  {
    rex_ = regexp_impl::Sum<T>::make(rex_, r.rex_);
    return *this;
  };

  //! Intersection (logical and)
  const RegExp<T> operator&(const RegExp<T> &r) const
  {
    return RegExp<T>{regexp_impl::And<T>::make(rex_, r.rex_)};
  };

  //! Repetition
  const RegExp<T> operator[](unsigned int n) const
  {
    RegExp<T> rep = RegExp<T>::one;
    for (unsigned int i = 0; i < n; i++)
      rep *= *this;
    return rep;
  };

  //! Kleene Star
  const RegExp<T> kstar() const
  {
    return RegExp<T>{regexp_impl::Kst<T>::make(rex_)};
  };

  // Complement
  //      const RegExp negate()  const { return
  //      RegExp{regexp_impl::Neg::make(rex_)}; };

  //! ν function
  const RegExp<T> nu() const
  {
    return rex_->nullable() ? RegExp<T>::one : RegExp<T>::zero;
  };

  //! Derivative with respect to a letter
  const RegExp<T> derivative(const T &x) const
  {
    return RegExp<T>{rex_->derivative(x)};
  };

  //! Derivative with respect to a word
  const RegExp<T> derivative(const std::vector<T> &w) const
  {
    if (w.empty()) return *this;
    RegExp<T> t{*this};
    for (const T &l : w)
      t = t.derivative(l);
    return t;
  };

  //! Match
  bool match(const std::vector<T> &w) { return derivative(w).nu() == one; };

  //! Alphabet
  const std::unordered_set<T> alphabet() const
  {
    std::unordered_set<T> a;
    rex_->traverse([&](const T &l) { a.insert(l); });
    return a;
  };

  //! Check if it is literal
  bool is_literal() const
  {
    return rex_->type() == 3;
  };

  //! Extract letter from literal
  const T letter() const
  {
    if (!is_literal()) throw std::logic_error("cannot extract letter from non literal");
    return std::static_pointer_cast<const regexp_impl::Lit<T>>(rex_)->letter();
  }

private:
  regexp_impl::rex_ptr_t<T> rex_;
};

//! Stream output
template <typename T>
inline std::ostream &operator<<(std::ostream &os, const RegExp<T> &r)
{
  r.print(os);
  return os;
};

//! Zero constant
template <typename T>
const RegExp<T> RegExp<T>::zero{regexp_impl::Zer<T>::Instance};

//! One constant
template <typename T>
const RegExp<T> RegExp<T>::one{regexp_impl::One<T>::Instance};

//! Type mapping functor
template <typename S, typename T>
RegExp<T> map_type(const RegExp<S> &r)
{
  return RegExp<T>{regexp_impl::map<S, T>(r.getImplementationPointer())};
};
}

//! Hash specialization
namespace std
{
template <typename T>
struct hash<regexp::RegExp<T>>
{
  size_t operator()(const regexp::RegExp<T> &r) const { return r.hash(); };
};
}

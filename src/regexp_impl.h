#pragma once

#include "hash_combine.h"
#include <algorithm>
#include <functional>
#include <iostream>
#include <memory>
#include <numeric>
#include <string>
#include <unordered_set>
#include <vector>

namespace regexp_impl
{
//! Regular expression underlying implementation
template <typename T>
class Rex;

using rex_t = uint;

template <typename T>
using rex_ptr_t = typename std::shared_ptr<Rex<T>>;

//! Regular Expression base virtual implementation class
template <typename T>
class Rex
{
public:
  virtual ~Rex(){};

  //! Regexp type (zero, one, literal, ...)
  virtual rex_t type() const = 0;

  //! String representation
  virtual void print(std::ostream &) const = 0;

  //! Equality
  virtual bool equal(rex_ptr_t<T>) const = 0;

  //! Hash
  virtual size_t hash() const = 0;

  //! Check if regexp is nullable
  virtual bool nullable() const = 0;

  //! Regexp derivative with respect to a letter
  virtual rex_ptr_t<T> derivative(const T &) const = 0;

  //! Traverse expression tree to literal
  virtual void traverse(std::function<void(const T &)>) const = 0;
};

template <typename T>
struct rex_ptr_hash : std::unary_function<rex_ptr_t<T>, std::size_t>
{
  size_t operator()(rex_ptr_t<T> r) const { return r->hash(); };
};

template <typename T>
struct rex_ptr_eq : std::binary_function<rex_ptr_t<T>, rex_ptr_t<T>, bool>
{
  bool operator()(rex_ptr_t<T> r, rex_ptr_t<T> s) const { return r->equal(s); };
};

template <typename T>
using rex_ptr_set_t = typename std::unordered_set<rex_ptr_t<T>, rex_ptr_hash<T>, rex_ptr_eq<T>>;

template <typename T>
using rex_ptr_vec_t = typename std::vector<rex_ptr_t<T>>;

//! Empty set: ∅
template <typename T>
class Zer : public Rex<T>
{
public:
  static rex_ptr_t<T> Instance;
  static const rex_t  Type = 1;

  Zer(){};

  rex_t type() const { return Type; };

  void print(std::ostream &os) const { os << "∅"; };

  bool equal(rex_ptr_t<T> r) const { return r->type() == Type; };

  size_t hash() const { return 0; };

  bool nullable() const { return false; };

  rex_ptr_t<T> derivative(const T &) const { return Instance; };

  void traverse(std::function<void(const T &)>) const {};
};

//! Empty string: ε
template <typename T>
class One : public Rex<T>
{
public:
  static rex_ptr_t<T> Instance;
  static const rex_t  Type = 2;

  One(){};

  rex_t type() const { return Type; };

  void print(std::ostream &os) const { os << "ε"; };

  bool equal(rex_ptr_t<T> r) const { return r->type() == Type; };

  size_t hash() const { return 1; };

  bool nullable() const { return true; };

  rex_ptr_t<T> derivative(const T &) const { return Zer<T>::Instance; };

  void traverse(std::function<void(const T &)>) const {};
};

//! Literal: a (a ∈ Σ)
template <typename T>
class Lit : public Rex<T>
{
public:
  static const rex_t Type = 3;

  Lit(T c)
      : c{c} {};

  rex_t type() const { return Type; };

  void print(std::ostream &os) const { os << c; };

  bool equal(rex_ptr_t<T> r) const
  {
    if (r->type() != Type) return false;
    return std::static_pointer_cast<const Lit<T>>(r)->letter() == c;
  };

  size_t hash() const { return std::hash<T>{}(c); };

  bool nullable() const { return false; };

  rex_ptr_t<T> derivative(const T &x) const
  {
    return x == c ? One<T>::Instance : Zer<T>::Instance;
  };

  void traverse(std::function<void(const T &)> f) const { f(c); };

  // return underlying letter
  const T letter() const { return c; };

private:
  const T c;
};

//! Sum: r + s + t ...
template <typename T>
class Sum : public Rex<T>
{
public:
  static const rex_t Type = 4;

  //! Sum: r + s + t
  /*! the following simplification rules are implemented:
   *
   * -     ∅ + r ≈ r
   * -     r + ∅ ≈ r
   * -     r + r ≈ r
   * -     r + s ≈ s + r
   * - r + s + t ≈ (r + s) + t
   *             ≈  r + (s + t)
   */
  static rex_ptr_t<T> make(rex_ptr_t<T> r, rex_ptr_t<T> s);

  Sum(rex_ptr_t<T> r, rex_ptr_t<T> s)
      : items_{r, s} {};

  Sum(const rex_ptr_set_t<T> &items)
      : items_{items} {
            // TBD: check size
        };

  rex_t type() const { return Type; };

  void print(std::ostream &os) const
  {
    os << "(";
    bool first = true;
    for (const auto &r : items_)
    {
      if (!first) os << "+";
      r->print(os);
      first = false;
    }
    os << ")";
  };

  bool equal(rex_ptr_t<T> r) const
  {
    if (r->type() != Type) return false;
    rex_ptr_set_t<T> r_items_ = std::static_pointer_cast<const Sum>(r)->items();
    if (items_.size() != r_items_.size()) return false;
    for (auto &p : items_)
      if (r_items_.find(p) == r_items_.end()) return false;
    return true;
  };

  size_t hash() const
  {
    size_t seed = 0;
    for (auto ptr : items_)
      hash_combine(seed, 0x426a3d31, ptr->hash());
    return seed;
  };

  bool nullable() const
  {
    return std::any_of(std::begin(items_), std::end(items_), [](rex_ptr_t<T> r) { return r->nullable(); });
  };

  // ∂a (r + s) ≡ ∂a r + ∂a s
  rex_ptr_t<T> derivative(const T &x) const
  {
    rex_ptr_set_t<T> ds;
    for (const auto &r : items_)
    {
      rex_ptr_t<T> d = r->derivative(x);
      if (d->type() != Zer<T>::Type)
        ds.insert(d);
    }
    if (ds.empty())
      return Zer<T>::Instance;

    if (ds.size() == 1)
      return *ds.begin();

    return std::accumulate(
        ds.begin(),
        ds.end(),
        Zer<T>::Instance,
        [](const rex_ptr_t<T> &a, const rex_ptr_t<T> &b) { return make(a, b); });
  };

  void traverse(std::function<void(const T &)> f) const
  {
    for (auto p : items_)
      p->traverse(f);
  };

  const rex_ptr_set_t<T> items() const { return items_; }

private:
  rex_ptr_set_t<T> items_;
};

//! And: r & s & t ...
template <typename T>
class And : public Rex<T>
{
public:
  static const rex_t Type = 5;

  //! And: r & s & t
  /*! the following simplification rules are implemented:
   *
   * -     ∅ & r ≈ ∅
   * -     r & ∅ ≈ ∅
   * -     r & r ≈ r
   * -     r & s ≈ s & r
   * - r & s & t ≈ (r & s) & t
   *             ≈  r & (s & t)
   */
  static rex_ptr_t<T> make(rex_ptr_t<T> r, rex_ptr_t<T> s);

  And(rex_ptr_t<T> r, rex_ptr_t<T> s)
      : items_{r, s} {};

  And(const rex_ptr_set_t<T> &items)
      : items_{items} {
            // TBD: check size
        };

  rex_t type() const { return Type; };

  void print(std::ostream &os) const
  {
    os << "(";
    bool first = true;
    for (const auto &r : items_)
    {
      if (!first) os << "&";
      r->print(os);
      first = false;
    }
    os << ")";
  };

  bool equal(rex_ptr_t<T> r) const
  {
    if (r->type() != Type) return false;
    rex_ptr_set_t<T> r_items_ = std::static_pointer_cast<const And>(r)->items();
    if (items_.size() != r_items_.size()) return false;
    for (auto &p : items_)
      if (r_items_.find(p) == r_items_.end()) return false;
    return true;
  };

  size_t hash() const
  {
    size_t seed = 0;
    for (auto ptr : items_)
      hash_combine(seed, 0x1ab34de1, ptr->hash());
    return seed;
  };

  bool nullable() const
  {
    return std::all_of(std::begin(items_), std::end(items_), [](rex_ptr_t<T> r) { return r->nullable(); });
  };

  // ∂a (r & s) ≡ ∂a r & ∂a s
  rex_ptr_t<T> derivative(const T &x) const
  {
    rex_ptr_set_t<T> ds;
    for (auto r : items_)
    {
      rex_ptr_t<T> d = r->derivative(x);
      if (d->type() == Zer<T>::Type)
        return d;
      ds.insert(d);
    }
    if (ds.empty())
      return Zer<T>::Instance;
    if (ds.size() == 1)
      return *ds.begin();
    return std::make_shared<And>(ds);
  };

  void traverse(std::function<void(const T &)> f) const
  {
    for (auto p : items_)
      p->traverse(f);
  };

  const rex_ptr_set_t<T> items() const { return items_; }

private:
  rex_ptr_set_t<T> items_;
};

//! Product: r · s · t ...
template <typename T>
class Prd : public Rex<T>
{
public:
  static const rex_t Type = 6;

  //! Concatenation: r · s · t
  /*! the following simplification rules are implemented:
   *
   * -     ∅ · r ≈ ∅
   * -     r · ∅ ≈ ∅
   * -     ε · r ≈ r
   * -     r · ε ≈ r
   * - r · s · t ≈ (r · s) · t
   *             ≈  r · (s · t)
   * -  r* · r*  ≈ r*
   *
   * also products are put in normal form:
   *
   * - (r + s) · (x + y) ≈ r·x + s·x + r·y + s·y
   * - (r + s) ·  x      ≈ r·x + s·x
   * -      r  · (x + y) ≈ r·x + r·y
   */
  static rex_ptr_t<T> make(rex_ptr_t<T> r, rex_ptr_t<T> s);

  Prd(rex_ptr_t<T> r, rex_ptr_t<T> s)
      : items_{r, s} {};

  Prd(const rex_ptr_vec_t<T> &items)
      : items_{items} {
            // TBD: check size
        };

  rex_t type() const { return Type; };

  void print(std::ostream &os) const
  {
    os << "(";
    bool first = true;
    for (const auto &r : items_)
    {
      if (!first) os << "·";
      r->print(os);
      first = false;
    }
    os << ")";
  };

  bool equal(rex_ptr_t<T> r) const
  {
    if (r->type() != Type) return false;
    rex_ptr_vec_t<T> r_items_ = std::static_pointer_cast<const Prd>(r)->items();
    if (items_.size() != r_items_.size()) return false;
    for (unsigned int i = 0; i < items_.size(); i++)
      if (!items_[i]->equal(r_items_[i])) return false;
    return true;
  };

  size_t hash() const
  {
    size_t seed = 0;
    for (auto ptr : items_)
      hash_combine(seed, 0x12b9b0a1, ptr->hash());
    return seed;
  };

  bool nullable() const
  {
    return std::all_of(std::begin(items_), std::end(items_), [](rex_ptr_t<T> r) { return r->nullable(); });
  };

  // ∂a (r · s) ≡ ∂a r · s + ν(r) · ∂a s
  rex_ptr_t<T> derivative(const T &x) const
  {
    rex_ptr_t<T> r = head();
    rex_ptr_t<T> s = tail();
    if (r->nullable())
      return Sum<T>::make(Prd<T>::make(r->derivative(x), s), s->derivative(x));
    else
      return Prd<T>::make(r->derivative(x), s);
  };

  void traverse(std::function<void(const T &)> f) const
  {
    for (auto p : items_)
      p->traverse(f);
  };

  const rex_ptr_vec_t<T> items() const { return items_; }

private:
  rex_ptr_vec_t<T> items_;

  rex_ptr_t<T> head() const { return items_[0]; };
  rex_ptr_t<T> tail() const
  {
    if (items_.size() == 2)
      return items_[1];
    rex_ptr_vec_t<T> t;
    t.insert(t.end(), items_.begin() + 1, items_.end());
    return std::make_shared<Prd<T>>(t);
  };
};

//! Kleene Star: r*
template <typename T>
class Kst : public Rex<T>
{
public:
  static const rex_t Type = 7;

  //! Kleene Star: r*
  /*! the following simplification rules are implemented:
   *
   * -     (r*)* ≈ r*
   * -        ε* ≈ ε
   * -        ∅* ≈ ε
   */
  static rex_ptr_t<T> make(rex_ptr_t<T> r);

  Kst(rex_ptr_t<T> r)
      : item_{r} {};

  rex_t type() const { return Type; };

  void print(std::ostream &os) const
  {
    os << "(";
    item_->print(os);
    os << ")*";
  };

  bool equal(rex_ptr_t<T> r) const
  {
    if (r->type() != Type)
      return false;
    return item_->equal(std::static_pointer_cast<const Kst>(r)->item());
  };

  size_t hash() const
  {
    size_t seed = 0;
    hash_combine(seed, 0x2439ab37, item_->hash());
    return seed;
  };

  bool nullable() const { return true; };

  // ∂a (r*) ≡ ∂a r · (r*)
  rex_ptr_t<T> derivative(const T &x) const
  {
    return Prd<T>::make(item_->derivative(x), make(item_));
  };

  void traverse(std::function<void(const T &)> f) const { item_->traverse(f); };

  const rex_ptr_t<T> item() const { return item_; };

private:
  rex_ptr_t<T> item_;
};

template <typename T>
rex_ptr_t<T> Zer<T>::Instance = std::make_shared<Zer<T>>();

template <typename T>
rex_ptr_t<T> One<T>::Instance = std::make_shared<One<T>>();

//! Type mapping functor
template <typename S, typename T>
rex_ptr_t<T> map(rex_ptr_t<S> r)
{
  switch (r->type())
  {
  case Zer<S>::Type:
    return Zer<T>::Instance;
    break;

  case One<S>::Type:
    return One<T>::Instance;
    break;

  case Lit<S>::Type:
  {
    const S l = std::static_pointer_cast<Lit<S>>(r)->letter();
    return std::make_shared<Lit<T>>(Lit<T>{T{l}});
    break;
  }

  case Sum<S>::Type:
  {
    rex_ptr_set_t<S> itms_S = std::static_pointer_cast<Sum<S>>(r)->items();
    rex_ptr_set_t<T> itms_T;
    for (const auto &itm : itms_S)
      itms_T.insert(map<S, T>(itm));
    return std::make_shared<Sum<T>>(Sum<T>{itms_T});
    break;
  }

  case And<S>::Type:
  {
    rex_ptr_set_t<S> itms_S = std::static_pointer_cast<And<S>>(r)->items();
    rex_ptr_set_t<T> itms_T;
    for (const auto &itm : itms_S)
      itms_T.insert(map<S, T>(itm));
    return std::make_shared<And<T>>(And<T>{itms_T});
    break;
  }

  case Prd<S>::Type:
  {
    rex_ptr_vec_t<S> itms_S = std::static_pointer_cast<Prd<S>>(r)->items();
    rex_ptr_vec_t<T> itms_T;
    for (const auto &itm : itms_S)
      itms_T.push_back(map<S, T>(itm));
    return std::make_shared<Prd<T>>(Prd<T>{itms_T});
    break;
  }

  case Kst<S>::Type:
  {
    rex_ptr_t<S> item = std::static_pointer_cast<Kst<S>>(r)->item();
    return std::make_shared<Kst<T>>(Kst<T>{map<S, T>(item)});
    break;
  }

  default:
    break;
  }
  return Zer<T>::Instance;
};
}

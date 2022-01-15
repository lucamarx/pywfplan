#include "regexp_impl.h"

namespace regexp_impl
{

template <typename T>
rex_ptr_t<T> Sum<T>::make(rex_ptr_t<T> r, rex_ptr_t<T> s)
{
  // ∅ + s ≈ s
  if (r->type() == Zer<T>::Type) return s;
  // r + ∅ ≈ r
  if (s->type() == Zer<T>::Type) return r;
  // r + r ≈ r
  if (r->equal(s)) return r;
  // (r + s) + t ≈ r + s + t
  if (r->type() == Type)
  {
    rex_ptr_set_t<T> ts;
    rex_ptr_set_t<T> rs = std::static_pointer_cast<const Sum>(r)->items();
    ts.insert(rs.begin(), rs.end());
    if (s->type() == Sum<T>::Type)
    {
      rex_ptr_set_t<T> ss = std::static_pointer_cast<const Sum>(s)->items();
      ts.insert(ss.begin(), ss.end());
    }
    else
      ts.insert(s);
    return std::make_shared<Sum>(ts);
  }
  // r + (s + t) ≈ r + s + t
  if (s->type() == Type)
  {
    rex_ptr_set_t<T> ts;
    rex_ptr_set_t<T> ss = std::static_pointer_cast<const Sum>(s)->items();
    ts.insert(ss.begin(), ss.end());
    ts.insert(r);
    return std::make_shared<Sum>(ts);
  }
  return std::make_shared<Sum>(r, s);
};

template <typename T>
rex_ptr_t<T> And<T>::make(rex_ptr_t<T> r, rex_ptr_t<T> s)
{
  // ∅ & s ≈ s
  if (r->type() == Zer<T>::Type) return r;
  // r & ∅ ≈ r
  if (s->type() == Zer<T>::Type) return s;
  // r & r ≈ r
  if (r->equal(s)) return r;
  // (r & s) & t ≈ r & s & t
  if (r->type() == And::Type)
  {
    rex_ptr_set_t<T> ts;
    rex_ptr_set_t<T> rs = std::static_pointer_cast<const And>(r)->items();
    ts.insert(rs.begin(), rs.end());
    if (s->type() == And::Type)
    {
      rex_ptr_set_t<T> ss = std::static_pointer_cast<const And>(s)->items();
      ts.insert(ss.begin(), ss.end());
    }
    else
      ts.insert(s);
    return std::make_shared<And>(ts);
  }
  // r & (s & t) ≈ r & s & t
  if (s->type() == And::Type)
  {
    rex_ptr_set_t<T> ts;
    rex_ptr_set_t<T> ss = std::static_pointer_cast<const And>(s)->items();
    ts.insert(ss.begin(), ss.end());
    ts.insert(r);
    return std::make_shared<And>(ts);
  }
  return std::make_shared<And>(r, s);
};

template <typename T>
rex_ptr_t<T> Prd<T>::make(rex_ptr_t<T> r, rex_ptr_t<T> s)
{
  // ∅ · r ≈ ∅, r · ε ≈ r
  if (r->type() == Zer<T>::Type || s->type() == One<T>::Type) return r;
  // r · ∅ ≈ ∅, ε · s ≈ s
  if (s->type() == Zer<T>::Type || r->type() == One<T>::Type) return s;
  // r* · r* ≈ r*
  if (r->type() == Kst<T>::Type && s->type() == Kst<T>::Type)
  {
    rex_ptr_t<T> ri = std::static_pointer_cast<const Kst<T>>(r)->item();
    rex_ptr_t<T> si = std::static_pointer_cast<const Kst<T>>(s)->item();
    if (ri->equal(si)) return r;
  }
  // (r + s) · (x + y) ≈ r·x + s·x + r·y + s·y
  // if (r->type() == Sum<T>::Type && s->type() == Sum<T>::Type)
  // {
  //   rex_ptr_set_t<T> rs = std::static_pointer_cast<const Sum<T>>(r)->items();
  //   rex_ptr_set_t<T> ss = std::static_pointer_cast<const Sum<T>>(s)->items();
  //   rex_ptr_set_t<T> ts;
  //   for (auto r_t : rs)
  //     for (auto s_t : ss)
  //       ts.insert(Prd<T>::make(r_t, s_t));
  //   return std::accumulate(
  //       ts.begin(),
  //       ts.end(),
  //       Zer<T>::Instance,
  //       [](const rex_ptr_t<T> &a, const rex_ptr_t<T> &b) { return Sum<T>::make(a, b); });
  // }
  // (r + s) · x ≈ r·x + s·x
  // if (r->type() == Sum<T>::Type)
  // {
  //   rex_ptr_set_t<T> ts;
  //   for (auto t : std::static_pointer_cast<const Sum<T>>(r)->items())
  //     ts.insert(Prd<T>::make(t, s));
  //   return std::accumulate(
  //       ts.begin(),
  //       ts.end(),
  //       Zer<T>::Instance,
  //       [](const rex_ptr_t<T> &a, const rex_ptr_t<T> &b) { return Sum<T>::make(a, b); });
  // }
  // r · (x + y) ≈ r·x + r·y
  // if (s->type() == Sum<T>::Type)
  // {
  //   rex_ptr_set_t<T> ts;
  //   for (auto t : std::static_pointer_cast<const Sum<T>>(s)->items())
  //     ts.insert(Prd<T>::make(r, t));
  //   return std::accumulate(
  //       ts.begin(),
  //       ts.end(),
  //       Zer<T>::Instance,
  //       [](const rex_ptr_t<T> &a, const rex_ptr_t<T> &b) { return Sum<T>::make(a, b); });
  // }
  // (r · s) · t ≈ r · s · t
  if (r->type() == Prd<T>::Type)
  {
    rex_ptr_vec_t<T> ts;
    rex_ptr_vec_t<T> rs = std::static_pointer_cast<const Prd<T>>(r)->items();
    ts.insert(ts.end(), rs.begin(), rs.end());
    if (s->type() == Prd::Type)
    {
      rex_ptr_vec_t<T> ss =
          std::static_pointer_cast<const Prd<T>>(s)->items();
      ts.insert(ts.end(), ss.begin(), ss.end());
    }
    else
      ts.push_back(s);
    return std::make_shared<Prd<T>>(ts);
  }
  // r · (s · t) ≈ r · s · t
  if (s->type() == Prd<T>::Type)
  {
    rex_ptr_vec_t<T> ts;
    ts.push_back(r);
    rex_ptr_vec_t<T> ss = std::static_pointer_cast<const Prd<T>>(s)->items();
    ts.insert(ts.end(), ss.begin(), ss.end());
    return std::make_shared<Prd<T>>(ts);
  }
  return std::make_shared<Prd<T>>(r, s);
};

template <typename T>
rex_ptr_t<T> Kst<T>::make(rex_ptr_t<T> r)
{
  // ε* ≈ ε, ∅* ≈ ε
  if (r->type() == One<T>::Type || r->type() == Zer<T>::Type) return One<T>::Instance;
  // r** ≈ r*
  if (r->type() == Type) return r;
  return std::make_shared<Kst>(r);
};
}

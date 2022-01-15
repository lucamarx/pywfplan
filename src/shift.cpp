#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include "config.h"

#include "shift.h"

namespace shift
{
  Shift::Shift()
    : work_{false}
    , code_{}
    , span_{} {};

  Shift::Shift(const std::string &code, const std::vector<span_t> &span)
    : work_{!span.empty()}
    , code_{code}
    , span_{span}
  {
    std::sort(span_.begin(), span_.end(), [](const Shift::span_t &a, const Shift::span_t &b) { return a.first < b.first; });
  };

  Shift::Shift(const std::string &code, const std::vector<std::vector<int>> &span)
    : work_{!span.empty()}
    , code_{code}
    , span_{}
  {
    for (const auto &s : span) {
      if (s.size() != 2)
        throw std::invalid_argument("invalid time span");
      if (s[0] < 0 || s[1] < 0)
        throw std::invalid_argument("time cannot be negative");
      span_.push_back(std::make_pair((unsigned int)s[0], (unsigned int)s[1]));
    }
    std::sort(span_.begin(), span_.end(), [](const Shift::span_t &a, const Shift::span_t &b) { return a.first < b.first; });
  };

  bool Shift::operator==(const Shift &oth) const
  {
    if (span_.empty() && oth.span_.empty()) return code_ == oth.code_;
    if (span_.size() != oth.span_.size()) return false;
    return std::equal(span_.begin(), span_.end(), oth.span_.begin());
  };
  bool Shift::operator!=(const Shift &oth) const
  {
    return !(*this == oth);
  };

  bool Shift::operator<(const Shift &oth) const
  {
    if (span_.empty() && oth.span_.empty()) return code_ < oth.code_;
    if (!span_.empty() && oth.span_.empty()) return true;
    if (span_.empty() && !oth.span_.empty()) return false;

    return span_.front().first < oth.span_.front().first;
  };
  bool Shift::operator>(const Shift &oth) const
  {
    if (span_.empty() && oth.span_.empty()) return code_ > oth.code_;
    if (!span_.empty() && oth.span_.empty()) return false;
    if (span_.empty() && !oth.span_.empty()) return true;

    return span_.front().first > oth.span_.front().first;
  };

  bool Shift::operator<=(const Shift &oth) const { return *this == oth || *this < oth; };
  bool Shift::operator>=(const Shift &oth) const { return *this == oth || *this > oth; };

  void Shift::print(std::ostream &os) const
  {
    os << code_;
  };

  const std::string Shift::to_string() const
  {
    return code_;
  };

  unsigned int Shift::t0() const
  {
    return span_.empty() ? 0 : span_[0].first;
  };

  unsigned int Shift::t1() const
  {
    return span_.empty() ? 24 * 60 : span_.back().second;
  };

  bool Shift::work() const { return work_; };

  const std::string Shift::code() const { return code_; };

  const std::vector<Shift::span_t> Shift::span() const { return span_; };

  void Shift::add_staff(unsigned int day, double c, std::vector<double> &stf) const
  {
    unsigned int sz = stf.size();
    for (const auto &s : span_)
      {
        unsigned int s0 = day * SLOTS_DAY + s.first / SLOT_LENGTH;
        unsigned int s1 = day * SLOTS_DAY + s.second / SLOT_LENGTH;
        for (unsigned int s = s0; s < s1 && s < sz; s++)
          stf[s] += c;
      }
  };

  unsigned int Shift::staff(unsigned int t) const
  {
    if (span_.empty() || t < span_.front().first || t > span_.back().second)
      return 0;

    for (const auto &s : span_)
      if (s.first <= t && t < s.second)
        return 1;

    return 0;
  };
}

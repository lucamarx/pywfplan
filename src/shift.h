#pragma once

#include <iostream>
#include <string>
#include <vector>

#include "config.h"

namespace shift
{

  //! Shift class
  class Shift
  {
  public:
    using span_t = std::pair<uint, uint>;

    Shift();

    Shift(const std::string &code, const std::vector<span_t> &span);
    Shift(const std::string &code, const std::vector<std::vector<int>> &span);

    bool operator==(const Shift &oth) const;
    bool operator!=(const Shift &oth) const;

    //! Ordering based on entry time for working shifts, rest shifts
    //! order is based on their code
    bool operator<(const Shift &oth) const;
    bool operator>(const Shift &oth) const;

    bool operator<=(const Shift &oth) const;
    bool operator>=(const Shift &oth) const;

    void print(std::ostream &os) const;

    const std::string to_string() const;

    //! Entry time in minutes
    unsigned int t0() const;

    //! Exit time in minutes
    unsigned int t1() const;

    //! Work/rest flag
    bool work() const;

    //! Shift code
    const std::string code() const;

    //! Shift working time spans
    const std::vector<span_t> span() const;

    //! Update staffing curve
    void add_staff(unsigned int day, double c, std::vector<double> &stf) const;

    //! Shift staffing for a specific time
    unsigned int staff(unsigned int t) const;

  private:
    bool                work_;
    std::string         code_;
    std::vector<span_t> span_;
  };

  //! Stream output
  inline std::ostream &operator<<(std::ostream &os, const Shift &s)
  {
    s.print(os);
    return os;
  };

  //! Shift adapter equi-probability partitioning
  /*! Implement the following partitions:
   *
   *  1. non-working shifts
   *  2. early-morning working shifts (less than 8:00)
   *  3. morning and afternoon shifts (less than 16:00)
   *  4. evening shifts
   *
   */
  struct shift_epp
  {
    unsigned int operator()(const Shift &s)
    {
      if (!s.work()) return 1;
      if (s.t0() <= 8 * 60) return 2;
      if (s.t0() <= 16 * 60) return 3;
      return 4;
    };
  };
}

// Hash specialization
namespace std
{
  template <>
  struct hash<shift::Shift>
  {
    size_t operator()(const shift::Shift &s) const
    {
      return std::hash<std::string>{}(s.code());
    };
  };
}

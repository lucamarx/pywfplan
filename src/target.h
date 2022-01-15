#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <stdexcept>

#include "config.h"

namespace target
{
  //! Target staffing curve
  class Target
  {
  public:
    //! Create target from data
    Target(unsigned int slot_length, unsigned int days, const std::vector<double> &target)
      : days_{days}
      , target_{}
      , shift_offset_{0}
      , staff_hours_{}
    {
      if (slot_length < 5)
        throw std::runtime_error{"invalid slot length, should be a multiple of 5 minutes"};

      if (slot_length % 5 != 0)
        {
          std::stringstream msg;
          msg << "invalid subsampling ratio " << slot_length << ", must be a multiple of 5 minutes";
          throw std::runtime_error{msg.str()};
        }

      unsigned int slots = days * (24 * 60 / slot_length);
      if (target.size() < slots)
        {
          std::stringstream msg;
          msg << "too few target points, should be at least " << slots << " for " << days << " days and " << slot_length << " minutes slots";
          throw std::runtime_error{msg.str()};
        }

      // subsample
      unsigned int ratio = slot_length / 5;
      for (double t : target)
        {
          for (unsigned int i = 0; i < ratio; i++)
            target_.push_back(t);
        }

      // pad target with zeros to the next planning day
      for (unsigned int n = target_.size() % SLOTS_DAY; n < SLOTS_DAY; n++)
        target_.push_back(0.0);
    };

    //! Get length in days
    unsigned int days() const
    {
      return days_;
    };

    //! Staffing hours
    /*!
     * @param offset shift starting time (in minutes)
     * @param day
     */
    double hours(unsigned int offset, unsigned int day) const
    {
      double h  = 0;
      unsigned int   i0 = day * SLOTS_DAY + offset / SLOT_LENGTH;
      unsigned int   i1 = i0 + SLOTS_DAY;
      for (unsigned int i = i0; i < i1 && i < target_.size(); i++)
        h += target_[i] * SLOT_LENGTH;
      return h / 60;
    };

    //! Get non-rescaled target
    const std::vector<double> getUnrescaledTarget() const
    {
      return target_;
    };

    //! Get target performing rescaling if necessary
    const std::vector<double> getTarget() const
    {
      if (staff_hours_.empty())
        return target_;
      std::vector<double> s{target_};
      for (unsigned int day = 0; day < days_; day++)
        {
          double h0 = hours(shift_offset_, day);
          double h1 = staff_hours_[day % staff_hours_.size()];
          unsigned int   i0 = day * SLOTS_DAY + shift_offset_ / SLOT_LENGTH;
          unsigned int   i1 = i0 + SLOTS_DAY;
          for (unsigned int i = i0; i < i1 && i < s.size(); i++)
            s[i]      = target_[i] * (h1 == 0 ? 1 : h1 / h0);
        }
      return s;
    };

    //! Staff rescaling based on daily staff hours
    /*!
     * @param offset shift starting time (in minutes)
     * @param staff_hours daily hours provided by staff
     */
    void rescaleStaff(int offset, const std::vector<double> &staff_hours) const
    {
      if (offset > 24 * 60) throw std::logic_error{"invalid offset (should be less than 24*60)"};
      shift_offset_ = static_cast<uint>(offset);
      staff_hours_  = staff_hours;
    };

    void print(std::ostream &os) const { os << "Target: days=" << days_; };

    const std::string to_string() const
    {
      std::stringstream ss;
      print(ss);
      return ss.str();
    };

  private:
    unsigned int        days_;
    std::vector<double> target_;

    mutable unsigned int        shift_offset_;
    mutable std::vector<double> staff_hours_;
  };

  // Stream output
  inline std::ostream &operator<<(std::ostream &os, const Target &t)
  {
    t.print(os);
    return os;
  };
}

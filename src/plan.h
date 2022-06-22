#pragma once

#include <algorithm>
#include <exception>
#include <functional>
#include <map>
#include <string>
#include <vector>
#include <fstream>
#include <iomanip>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include "config.h"
#include "shift.h"
#include "target.h"

namespace plan
{
  struct plan_hours_t
  {
    plan_hours_t(double trg, double stf, double prc)
      : target{trg}
      , staffing{stf}
      , difference{prc} {};

    const double target;
    const double staffing;
    const double difference;
  };

  //! The plan
  /*! The plan class contains:
   *
   *  - the target staffing curve
   *  - the current staffing curve
   *  - the shift schedule for each agent
   *
   *  it is meant to be used in conjunction with the staff planner
   *  class, it exposes its member directly to allow for direct
   *  manipulation.
   *
   */
  class Plan
  {
  public:
    using line_t = std::vector<shift::Shift>;

    //! Create an empty plan for agents and target
    Plan(unsigned int offset, const std::vector<std::string> &agents, const target::Target &target)
      : target_{target.getTarget()}
      , target_unrescaled_{target.getUnrescaledTarget()}
      , staffing_(target_.size(), 0.0)
      , plan_{agents.size(), line_t{}}
      , days_{target.days()}
      , offset_{0}
      , agent_idx_map_{}
    {
      if (agents.empty()) throw std::invalid_argument{"you must add agents to create a plan"};

      // for (const auto &sht : shifts)
      //   if (sht.t1().minutes() > offset) offset = sht.t1().minutes();

      if (offset > 0) offset_ = offset / SLOT_LENGTH;

      unsigned int i = 0;
      for (const auto &code : agents)
        {
          agent_idx_map_.insert(std::make_pair(code, i));
          for (unsigned int j = 0; j < target.days(); j++)
            plan_[i].push_back(shift::Shift{});
          i++;
        }
    };

    //! Target staffing curve (rescaled)
    std::vector<double> target_;

    //! Target staffing curve (unrescaled)
    std::vector<double> target_unrescaled_;

    //! Planned staffing curve
    std::vector<double> staffing_;

    //! Plan
    std::vector<line_t> plan_;

    //! Plan length in days
    unsigned int days() const
    {
      return days_;
    };

    //! Time slots for a day plan
    unsigned int daySlots() const
    {
      return SLOTS_DAY + offset_;
    };

    //! Time slots for a week plan
    unsigned int weekSlots() const
    {
      return 7 * SLOTS_DAY + offset_;
    };

    //! Total target / staffing hours
    /*! returns:
     *
     *  1. total target hours
     *  2. total staffing hours
     *  3. difference in percentage
     */
    const plan_hours_t hours() const
    {
      double s_trg = 0.0;
      double s_stf = 0.0;
      for (unsigned int i = 0; i < target_.size(); i++)
        {
          s_trg += target_[i] * 5;
          s_stf += staffing_[i] * 5;
        }
      return plan_hours_t{s_trg / 60, s_stf / 60, 100 * (s_trg - s_stf) / s_trg};
    };

    //! Weekly target / staffing hours
    /*! returns:
     *
     *  1. total target hours
     *  2. total staffing hours
     *  3. difference in percentage
     */
    const plan_hours_t hours_week(unsigned int week) const
    {
      if (week * 7 > days_) throw std::invalid_argument{"week exceeds plan length"};
      double s_trg = 0.0;
      double s_stf = 0.0;
      for (unsigned int i = week * 7 * SLOTS_DAY; i < (week + 1) * 7 * SLOTS_DAY && i < target_.size(); i++)
        {
          s_trg += target_[i] * 5;
          s_stf += staffing_[i] * 5;
        }
      return plan_hours_t{s_trg / 60, s_stf / 60, 100 * (s_trg - s_stf) / s_trg};
    };

    //! Daily target / staffing hours
    /*! returns:
     *
     *  1. total target hours
     *  2. total staffing hours
     *  3. difference in percentage
     */
    const plan_hours_t hours_day(unsigned int day) const
    {
      if (day > days_) throw std::invalid_argument{"day exceed plan length"};
      double s_trg = 0.0;
      double s_stf = 0.0;
      for (unsigned int i = day * SLOTS_DAY; i < (day + 1) * SLOTS_DAY && i < target_.size(); i++)
        {
          s_trg += target_[i] * 5;
          s_stf += staffing_[i] * 5;
        }
      return plan_hours_t{s_trg / 60, s_stf / 60, 100 * (s_trg - s_stf) / s_trg};
    };

    //! Daily energy (mean squared difference between target and staffing)
    double energy(unsigned int day) const
    {
      if (day > days_) throw std::invalid_argument{"day exceed plan length"};
      double e = 0.0;
      for (unsigned int i = day * SLOTS_DAY; i < (day + 1) * SLOTS_DAY && i < staffing_.size(); i++)
        e += (target_[i] - staffing_[i]) * (target_[i] - staffing_[i]);
      return e / SLOTS_DAY;
    };

    //! Get plan index of agent
    unsigned int getAgentIndex(const std::string &agent_code) const
    {
      const auto &agt = agent_idx_map_.find(agent_code);
      if (agt == agent_idx_map_.end())
        throw std::invalid_argument{"agent not found in plan"};
      return agt->second;
    };

    //! Update agent plan starting from day
    void updatePlan(unsigned int agent_idx, unsigned int day, const line_t &plan)
    {
      if (day > days_) throw std::invalid_argument{"day exceed plan length"};
      for (unsigned int i = 0; i < plan.size() && day + i < plan_.size(); i++)
        plan_[agent_idx][day + i] = plan[i];
    };

    //! Get plan for agent
    const line_t getAgentPlan(const std::string &agent_code) const
    {
      return plan_[getAgentIndex(agent_code)];
    };

    //! Save whole plan to file
    void savePlan(const std::string &file_name) const
    {
      std::ofstream f{file_name};
      for (const auto &a : agent_idx_map_)
        {
          f << a.first << ":";
          for (const auto &s : plan_[a.second])
            f << std::setw(10) << " " << s;
          f << "\n";
        }
      f.close();
    };

    //! Get the (rescaled) target staffing curve
    std::vector<double> getTargetStaffing() const
    {
      return target_;
    };

    //! Get the final staffing curve
    std::vector<double> getPlannedStaffing() const
    {
      return staffing_;
    };

    //! Save staffing curves to file
    void saveStaffing(const std::string &file_name) const
    {
      std::ofstream f{file_name};
      for (unsigned int i = 0; i < target_.size() && i < target_unrescaled_.size() && i < staffing_.size(); i++)
        f << i
          << " "
          << std::setprecision(4) << target_[i]
          << " "
          << std::setprecision(4) << target_unrescaled_[i]
          << " "
          << std::setprecision(4) << staffing_[i]
          << "\n";
      f.close();
    };

    void print(std::ostream &os) const { os << "Plan: days=" << days_; };

    const std::string to_string() const
    {
      std::stringstream ss;
      print(ss);
      return ss.str();
    };

  private:
    unsigned int days_;
    unsigned int offset_;

    std::map<std::string, uint> agent_idx_map_;
  };

  // Stream output
  inline std::ostream &operator<<(std::ostream &os, const Plan &p)
  {
    p.print(os);
    return os;
  };
}

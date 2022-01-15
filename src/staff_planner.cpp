#include <chrono>
#include <exception>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "config.h"

#include "plan.h"
#include "shift.h"
#include "target.h"

#include "fsm.h"
#include "regexp.h"

#include "anneal.h"

#include "staff_energy.h"
#include "staff_state.h"

#include "staff_planner.h"

namespace staff_planner
{
  //! Create a planner
  /*!
   * @param description    just for reporting
   * @param plan           a plan
   * @param temp_sched     simulated annealing temperature schedule
   * @param comfort_weight comfort energy weight relative to staffing energy
   */
  StaffPlanner::StaffPlanner(const std::string &description, const plan::Plan &plan, double temp_sched, double comfort_weight)
    : temp_sched_{temp_sched}
    , comfort_weight_{comfort_weight}
    , week_{0}
    , plan_{plan}
    , samplers_(plan_.plan_.size(), sampler_t{regexp::RegExp<shift::Shift>::zero})
    , report_{}
    , description_{description}
  {
    if (temp_sched_ < 0.5 || temp_sched_ >= 1.0) throw std::invalid_argument{"invalid temperature schedule (must be between 0.5 and 1.0)"};
    if (comfort_weight_ < 0.0) throw std::invalid_argument{"comfort energy weight must be positive"};
  };

  //! String conversion (for chai)
  const std::string StaffPlanner::to_string() const
  {
    std::stringstream ss;
    ss
      << "Planner:\n"
      << "           description: " << description_ << "\n"
      << "        turning length: " << plan_.days() << "\n"
      << "           slot length: " << SLOT_LENGTH << " minutes\n"
      << "             agents n°: " << samplers_.size() << "\n"
      << "       target staffing: " << std::fixed << std::setprecision(2) << plan_.hours().target << " hrs\n"
      << " comfort energy weight: " << std::setprecision(5) << comfort_weight_ << "\n"
      << "  temperature schedule: " << std::fixed << std::setprecision(2) << temp_sched_ << "\n";
    return ss.str();
  };

  //! Set week to plan
  void StaffPlanner::setWeek(int week)
  {
    unsigned int w = static_cast<uint>(week);
    if (w * 7 > plan_.days() - 7) throw std::invalid_argument{"week exceed plan length"};
    week_ = w;
  };

  //! Set a sampler for an agent
  /*! The agent's planning is defined by a regular expression over the
   *  Shift class which is not suitable for sampling. Thus we map the
   *  regular expression type to ShiftAdapter then we build the Fsm
   *  and add it to the samplers.
   */
  void StaffPlanner::setAgentSampler(const std::string &agent, const regexp::RegExp<shift::Shift> &regexp)
  {
    samplers_[plan_.getAgentIndex(agent)] = sampler_t{regexp};
  };

  //! Run simulation
  void StaffPlanner::run()
  {
    using planner_state_t = State<staffing_energy, comfort_energy>;
    using clock_t         = std::chrono::high_resolution_clock;
    using sec_t           = std::chrono::seconds;

    clock_t::time_point t0 = clock_t::now();
    // --------------------------------------------------------------------------------
    // create state
    planner_state_t state{samplers_, week_, plan_};

    // calibrate energy weights
    state.calibrate(comfort_weight_);

    // create annealer
    // TBD: IMPROVE HOW NOVER IS COMPUTED
    unsigned int nover = 10 * NOVER * static_cast<uint>(samplers_.size());

    anneal::Anneal<planner_state_t> anneal{nover, state};

    // calibrate temperature
    double ti = anneal.calibrateTi();
    double tf = anneal.calibrateTf();

    double e0_tot = state.energy();
    double e0_stf = state.staffing_energy();
    double e0_cmf = state.comfort_energy();

    // anneal
    anneal.anneal(ti, tf, temp_sched_);

    double e1_tot = state.energy();
    double e1_stf = state.staffing_energy();
    double e1_cmf = state.comfort_energy();

    // --------------------------------------------------------------------------------
    clock_t::time_point t1 = clock_t::now();

    double elapsed = std::chrono::duration_cast<sec_t>(t1 - t0).count();

    std::stringstream ss;
    ss
      << "===========================================================================\n"
      << description_ << "\n"
      << "          turning length: " << plan_.days() << "\n"
      << "                 week n°: " << week_ << "\n"
      << "             slot length: " << SLOT_LENGTH << " minutes\n"
      << "               agents n°: " << samplers_.size() << "\n"
      << "         target staffing: " << std::fixed << std::setprecision(2) << plan_.hours_week(week_).target << " hrs\n"
      << "      simulated staffing: " << std::fixed << std::setprecision(2) << plan_.hours_week(week_).staffing << " hrs\n"
      << "\n"
      << "   comfort energy weight: " << std::setprecision(5) << comfort_weight_ << "\n"
      << "\n"
      << "         annealing steps: " << static_cast<uint>(round((log(tf) - log(ti)) / log(temp_sched_))) << "\n"
      << "       temperature range: " << std::fixed << std::setprecision(5) << ti << " -> " << std::fixed << std::setprecision(5) << tf << "\n"
      << "    temperature schedule: " << std::fixed << std::setprecision(2) << temp_sched_ << "\n"
      << "       optimization time: " << std::fixed << std::setprecision(1) << (elapsed / 60) << " minutes\n"
      << "\n"
      << "         staffing energy: " << std::fixed << std::setprecision(5) << e0_stf << " -> " << std::fixed << std::setprecision(5) << e1_stf << "\n"
      << "          comfort energy: " << std::fixed << std::setprecision(5) << e0_cmf << " -> " << std::fixed << std::setprecision(5) << e1_cmf << "\n"
      << "            TOTAL ENERGY: " << std::fixed << std::setprecision(5) << e0_tot << " -> " << std::fixed << std::setprecision(5) << e1_tot << "\n"
      << "\n"
      << "     day by day staffing:\n";

    double trg_tot = 0.0;
    double stf_tot = 0.0;
    for (unsigned int day = week_ * 7; day < (week_ + 1) * 7; day++)
      {
        auto hrs = plan_.hours_day(day);
        ss
          << "                 day " << std::setw(3) << (day + 1) << ": "
          << std::fixed << std::setprecision(2) << hrs.staffing << " hrs"
          << " ("
          << "target " << std::fixed << std::setprecision(2) << hrs.target << " hrs"
          << " error " << std::fixed << std::setprecision(2) << hrs.difference << "%"
          << ")\n";
        trg_tot += hrs.target;
        stf_tot += hrs.staffing;
      }
    ss
      << "                   TOTAL: "
      << std::fixed << std::setprecision(2) << stf_tot << " hrs"
      << " ("
      << "target " << std::fixed << std::setprecision(2) << trg_tot << " hrs"
      << " error " << std::fixed << std::setprecision(2) << (100 * (trg_tot - stf_tot) / trg_tot) << "%"
      << ")\n"
      << "\n"
      << "       day by day energy:\n";

    for (unsigned int day = week_ * 7; day < (week_ + 1) * 7; day++)
      ss
        << "                 day " << std::setw(3) << (day + 1) << ": "
        << std::fixed << std::setprecision(2) << plan_.energy(day) << "\n";
    ss
      << "---------------------------------------------------------------------------\n";

    report_ = ss.str();
  };

  //! Retrieve the optimized plan
  plan::Plan StaffPlanner::getPlan() const
  {
    return plan_;
  };

  //! Get the planning report
  const std::string StaffPlanner::getReport() const
  {
    return report_;
  };

  //! Save sampler in dot format and convert to png
  void StaffPlanner::printSampler(const std::string &code) const
  {
    std::ofstream f{code + ".dot"};
    samplers_[plan_.getAgentIndex(code)].print(f);
    f.close();
    std::string cmd = "dot -Tpng " + code + ".dot > " + code + ".png";
    system(cmd.c_str());
  };
}

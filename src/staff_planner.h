#pragma once

#include <string>
#include <vector>

#include "plan.h"
#include "shift.h"
#include "target.h"

#include "regexp.h"

#include "staff_energy.h"
#include "staff_state.h"

namespace staff_planner
{
  //! Staff planning process
  /*! The staff planner class takes:
   *
   *  - the shifts
   *  - the agents
   *  - the staffing target
   *  - a configuration (temperature schedule, energy weights, ...)
   *
   * and performs the planning process:
   *
   *  1. instantiates a planner state with the correct energy terms
   *  2. performs energy weights calibration
   *  3. instantiates an annealer
   *  4. performs initial/final temperature calibration
   *  5. performs simulated annealing on the state
   *  6. saves the result
   *
   */
  class StaffPlanner
  {
  public:
    //! Create a planner
    /*!
     * @param description    just for reporting
     * @param plan           a plan
     * @param temp_sched     simulated annealing temperature schedule
     * @param comfort_weight comfort energy weight relative to staffing energy
     */
    StaffPlanner(const std::string &description, const plan::Plan &plan, double temp_sched, double comfort_weight);

    //! String conversion
    const std::string to_string() const;

    //! Set week to plan
    void setWeek(int week);

    //! Set a sampler for an agent
    /*! The agent's planning is defined by a regular expression over the
     *  Shift class which is not suitable for sampling. Thus we map the
     *  regular expression type to ShiftAdapter then we build the Fsm
     *  and add it to the samplers.
     */
    void setAgentSampler(const std::string &agent, const regexp::RegExp<shift::Shift> &regexp);

    //! Run simulation
    void run();

    //! Retrieve the optimized plan
    plan::Plan getPlan() const;

    //! Get the planning report
    const std::string getReport() const;

    //! Save sampler in dot format and convert to png
    void printSampler(const std::string &code) const;

  protected:
    const double           temp_sched_;
    const double           comfort_weight_;
    unsigned int           week_;
    plan::Plan             plan_;
    std::vector<sampler_t> samplers_;
    std::string            report_;
    std::string            description_;
  };
}

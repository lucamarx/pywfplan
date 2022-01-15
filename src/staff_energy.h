#pragma once

#include <vector>

#include "plan.h"

namespace staff_planner
{
  //! Staffing energy term
  /*! Sum of squared difference between current and target staffing
   *  curves
   *
   *  E = Sum_i (target_i - staffing_i)^2
   *
   */
  struct staffing_energy
  {
    staffing_energy(const plan::Plan &plan, unsigned int week);

    double energy() const;

    double delta(const std::vector<double> &prev_stf, const std::vector<double> &mutd_stf) const;

    double fitness(unsigned int day, const shift::Shift &sh0, const shift::Shift &sh1) const;

    const plan::Plan&  plan_;
    const unsigned int slot0_;
    const unsigned int slot1_;
  };

  //! Spread of entry times across plan
  struct comfort_energy
  {
    comfort_energy(const plan::Plan &plan, unsigned int week);

    double energy() const;

    double delta(unsigned int mutd_idx, const plan::Plan::line_t &mutd_pln) const;

    double fitness(const plan::Plan::line_t &pln, const shift::Shift &sh0, const shift::Shift &sh1) const;

    const plan::Plan&  plan_;
    const unsigned int week_;
  };
};

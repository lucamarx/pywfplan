#pragma once

#include <iomanip>
#include <iostream>
#include <random>
#include <vector>

#include "config.h"
#include "fsm.h"
#include "staff_energy.h"

namespace staff_planner
{

  using sampler_t = fsm::Fsm<shift::Shift, shift::shift_epp>;

  //! The planner state implements a sampler for the set of all possible plannings
  /*! The planner state consists in:
   *
   *  - a *sampler* (a fsm on shift adapters) for each agent
   *  - the *plan*
   *
   *  and implements a **mutate** method that:
   *
   *  - chooses an agent at random
   *  - either resamples the plan for the agent
   *  - or refines the plan choosing the best shifts
   *
   *  The mutated plan is kept along the current plan in order to
   *  evaluate its fitness (with a separate energy class) if the
   *  mutation is deemed useful it can be applied to the current plan
   *  with the **apply_mutation** method.
   *
   *  The planner state class is used in conjunction with:
   *
   *  - an energy class (to evaluate the plan mutations)
   *  - an annealing class to drive the optimization process
   *
   */
  template <typename ESTF, typename ECMF>
  class State
  {
  public:
    //! Takes a sampler for each agent and the target staffing curve
    State(const std::vector<sampler_t> &samplers, unsigned int week, plan::Plan &plan)
      : rne_{}
      , samplers_{samplers}
      , week_{week}
      , plan_{plan}
      , mutd_idx_{0}
      , mutd_pln_{}
      , prev_stf_(plan_.weekSlots(), 0.0)
      , mutd_stf_(plan_.weekSlots(), 0.0)
      , w1_{1.0}
      , staffing_energy_{plan_, week_}
      , comfort_energy_{plan_, week_}
    {
      if (samplers_.empty()) throw std::runtime_error{"you must provide some samplers"};

      std::random_device device;
      rne_.seed((static_cast<uint64_t>(device()) << 32) | device());

      for (unsigned int i = 0; i < samplers_.size(); i++)
        {
          plan::Plan::line_t pln = samplers_[i].sample();
          plan_.updatePlan(i, week_ * 7, pln);
          for (unsigned int day = 0; day < pln.size(); day++)
            pln[day].add_staff(week_ * 7 + day, +1, plan_.staffing_);
        }
      mutate();
    };

    //! Get the energy of the current state
    double energy() const
    {
      return staffing_energy_.energy() + w1_ * comfort_energy_.energy();
    };

    //! Get the energy delta of the mutated state
    double delta_energy() const
    {
      return staffing_energy_.delta(prev_stf_, mutd_stf_) + w1_ * comfort_energy_.delta(mutd_idx_, mutd_pln_);
    };

    //! Get the staffing energy contribution
    double staffing_energy() const
    {
      return staffing_energy_.energy();
    };

    //! Get the staffing energy delta of the mutated state
    double staffing_delta_energy() const
    {
      return staffing_energy_.delta(prev_stf_, mutd_stf_);
    };

    //! Get the comfort energy contribution
    double comfort_energy() const
    {
      return comfort_energy_.energy();
    };

    //! Get the comfort energy delta of the mutated state
    double comfort_delta_energy() const
    {
      return comfort_energy_.delta(mutd_idx_, mutd_pln_);
    };

    //! Calibrate energy weights
    void calibrate(double w1)
    {
      if (w1 == 0.0)
        {
          w1_ = 0.0;
          return;
        }
      unsigned int n = 200000;

      std::cout << "calibrating energy weights (" << n << " iterations)\n" << std::flush;

      double sum0    = 0.0;
      double sum_sq0 = 0.0;
      double sum1    = 0.0;
      double sum_sq1 = 0.0;

      for (unsigned int i = 1; i < n; i++)
        {
          mutate();
          apply_mutation();

          double e0 = staffing_energy_.energy();
          sum0 += e0;
          sum_sq0 += e0 * e0;

          double e1 = comfort_energy_.energy();
          sum1 += e1;
          sum_sq1 += e1 * e1;
        }

      double mean0   = sum0 / n;
      double stddev0 = sqrt((sum_sq0 - sum0 * sum0 / n) / (n - 1));

      double mean1   = sum1 / n;
      double stddev1 = sqrt((sum_sq1 - sum1 * sum1 / n) / (n - 1));

      std::cout
        << "staffing energy: mean=" << std::setprecision(4) << mean0 << " stddev=" << std::setprecision(4) << stddev0
        << "\n"
        << " comfort energy: mean=" << std::setprecision(4) << mean1 << " stddev=" << std::setprecision(4) << stddev1
        << "\n"
        << std::flush;

      w1_ = w1 * mean0 / mean1;

      std::cout << "updating ratio: " << std::setprecision(4) << w1 << " -> " << std::setprecision(4) << w1_ << "\n" << std::flush;
    };

    //! Mutate state by choosing one sampler and generating its plan
    /*! Two distinct moves are implemented:
     *
     *  1. sample the plan (with 90% probability)
     *  2. resample the plan choosing the *best* shifts (with 10% probability)
     *
     */
    void mutate()
    {
      mutd_idx_ = dist_int_t{0, samplers_.size() - 1}(rne_);

      if (dist_dbl_t{0.0, 1.0}(rne_) < 0.8)
        mutd_pln_ = samplers_[mutd_idx_].sample();
      else
        mutd_pln_ = samplers_[mutd_idx_].resample([&](unsigned int day, const plan::Plan::line_t &pln, const shift::Shift &sht) {
          return staffing_energy_.fitness(week_ * 7 + day, plan_.plan_[mutd_idx_][week_ * 7 + day], sht) + w1_ * comfort_energy_.fitness(pln, plan_.plan_[mutd_idx_][week_ * 7 + day], sht);
        });
      // TBD: CHECK CORRECTNESS OF FITNESS USE

      for (unsigned int i    = 0; i < mutd_stf_.size(); i++)
        prev_stf_[i] = mutd_stf_[i] = 0.0;

      for (unsigned int day = 0; day < 7; day++)
        {
          plan_.plan_[mutd_idx_][week_ * 7 + day].add_staff(day, +1, prev_stf_);
          mutd_pln_[day].add_staff(day, +1, mutd_stf_);
        }
    };

    //! Apply mutation to state and staffing
    void apply_mutation()
    {
      plan_.updatePlan(mutd_idx_, week_ * 7, mutd_pln_);

      for (unsigned int i = 0; i < plan_.weekSlots(); i++)
        plan_.staffing_[week_ * 7 * SLOTS_DAY + i] += mutd_stf_[i] - prev_stf_[i];
    };

  private:
    using dist_int_t = std::uniform_int_distribution<size_t>;
    using dist_dbl_t = std::uniform_real_distribution<double>;

    std::mt19937_64 rne_;

    // state setup
    std::vector<sampler_t> samplers_;
    unsigned int           week_;
    plan::Plan&            plan_;

    // mutated plan and staffing
    unsigned int        mutd_idx_;
    plan::Plan::line_t  mutd_pln_;
    std::vector<double> prev_stf_;
    std::vector<double> mutd_stf_;

    // comfort energy weight
    double w1_;

    // energy terms
    const ESTF staffing_energy_;
    const ECMF comfort_energy_;
  };

  //! Stream output
  template <typename ESTF, typename ECMF>
  inline std::ostream &operator<<(std::ostream &os, const State<ESTF, ECMF> &p)
  {
    p.print(os);
    return os;
  };
}

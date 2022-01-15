#include "staff_energy.h"
#include "config.h"

namespace staff_planner
{
  staffing_energy::staffing_energy(const plan::Plan &plan, unsigned int week)
    : plan_{plan}
    , slot0_{week * 7 * SLOTS_DAY}
    , slot1_{slot0_ + plan_.weekSlots()} {};

  double staffing_energy::energy() const
  {
    double tmpE = 0.0;
    for (unsigned int i = slot0_; i < slot1_; i++)
      {
        double e = plan_.staffing_[i] - plan_.target_[i];
        tmpE += e * e;
      }
    return tmpE / (slot1_ - slot0_);
  };

  double staffing_energy::delta(const std::vector<double> &prev_stf, const std::vector<double> &mutd_stf) const
  {
    unsigned int n     = plan_.weekSlots();
    double       tmpDe = 0.0;
    for (unsigned int i = 0; i < n; i++)
      {
        double e1 = mutd_stf[i] - prev_stf[i];
        double e2 = mutd_stf[i] - prev_stf[i] + 2 * plan_.staffing_[slot0_ + i] - 2 * plan_.target_[slot0_ + i];
        tmpDe += e1 * e2;
      }
    return tmpDe / n;
  };

  double staffing_energy::fitness(unsigned int day, const shift::Shift &sh0, const shift::Shift &sh1) const
  {
    double       fit = 0.0;
    unsigned int off = day * SLOTS_DAY;
    for (unsigned int i = 0; i < 2 * SLOTS_DAY && off + i < plan_.staffing_.size(); i++)
      {
        double f = plan_.target_[off + i] - (plan_.staffing_[off + i] - sh0.staff(i * SLOT_LENGTH) + sh1.staff(i * SLOT_LENGTH));
        fit += f * f;
      }
    return fit / SLOTS_DAY;
  };

  comfort_energy::comfort_energy(const plan::Plan &plan, unsigned int week)
    : plan_{plan}
    , week_{week} {};

  double comfort_energy::energy() const
  {
    double tmpE = 0.0;
    for (const auto &pln : plan_.plan_)
      {
        for (unsigned int i = week_ * 7 + 1; i < (week_ + 1) * 7; i++)
          if (pln[i - 1].work() && pln[i].work())
            {
              double d = (static_cast<double>(pln[i].t0()) - static_cast<double>(pln[i - 1].t0())) / SLOT_LENGTH;
              tmpE += d * d;
            }
      }
    return tmpE / 7;
  };

  double comfort_energy::delta(unsigned int mutd_idx, const plan::Plan::line_t &mutd_pln) const
  {
    unsigned int day1      = week_ * 7 + 1;
    unsigned int day7      = (week_ + 1) * 7;
    double       tmpE_curr = 0.0;
    for (unsigned int i = day1; i < day7; i++)
      {
        const auto &sht0 = plan_.plan_[mutd_idx][i - 1];
        const auto &sht1 = plan_.plan_[mutd_idx][i];
        if (sht0.work() && sht1.work())
          {
            double d = (static_cast<double>(sht1.t0()) - static_cast<double>(sht0.t0())) / SLOT_LENGTH;
            tmpE_curr += d * d;
          }
      }
    double tmpE_mutd = 0.0;
    for (unsigned int i = 1; i < 7; i++)
      {
        const auto &sht0 = mutd_pln[i - 1];
        const auto &sht1 = mutd_pln[i];
        if (sht0.work() && sht1.work())
          {
            double d = (static_cast<double>(sht1.t0()) - static_cast<double>(sht0.t0())) / SLOT_LENGTH;
            tmpE_mutd += d * d;
          }
      }
    return (tmpE_mutd - tmpE_curr) / 7;
  };

  double comfort_energy::fitness(const plan::Plan::line_t &pln, const shift::Shift &sh0, const shift::Shift &sh1) const
  {
    if (pln.empty()) return 0.0;
    const auto &shp = pln.back();
    double      fit = 0.0;
    if (shp.work() && sh0.work())
      {
        double d = (static_cast<double>(sh0.t0()) - static_cast<double>(shp.t0())) / SLOT_LENGTH;
        fit -= d * d;
      }
    if (shp.work() && sh1.work())
      {
        double d = (static_cast<double>(sh1.t0()) - static_cast<double>(shp.t0())) / SLOT_LENGTH;
        fit += d * d;
      }
    return fit;
  };
}

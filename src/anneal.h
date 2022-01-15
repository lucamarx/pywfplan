#pragma once

#include <cmath>
#include <iomanip>
#include <iostream>
#include <random>

namespace anneal
{
  //! Simulated annealing algorithm over a state
  template <typename S>
  class Anneal
  {
  public:
    Anneal(unsigned int nover, S &state)
      : rne_{}
      , urd_{0.0, 1.0}
      , nover_{nover}
      , state_{state}
    {
      std::random_device device;
      rne_.seed((static_cast<uint64_t>(device()) << 32) | device());
    };

    //! Calibrate initial temperature
    double calibrateTi()
    {
      std::cout << "performing initial temperature calibration ...\n" << std::flush;
      double t0  = 2.0;
      double chi = 0.0;
      while (chi < CHI0)
        {
          unsigned int a = 0;
          unsigned int n = 1;
          for (unsigned int i = 0; i < nover_ / 50; i++)
            {
              state_.mutate();
              n++;
              if (metropolis(state_.delta_energy(), t0))
                {
                  state_.apply_mutation();
                  a++;
                }
            }
          chi = static_cast<double>(a) / static_cast<double>(n);
          t0 *= 2.0;
        }
      std::cout
        << "initial temperature: " << std::setiosflags(std::ios::fixed)
        << std::setprecision(6) << t0
        << "\n"
        << std::flush;
      return t0;
    };

    //! Calibrate final temperature
    double calibrateTf()
    {
      std::cout << "performing final temperature calibration ...\n" << std::flush;
      double de_min = state_.energy();
      for (unsigned int n = 0; n < STATE_SETUP_TRIES; n++)
        {
          state_.mutate();
          double de = fabs(state_.delta_energy());
          if (de > 0.0 && de < de_min)
            de_min = de;
        }
      std::cout
        << "final temperature: " << std::setiosflags(std::ios::fixed)
        << std::setprecision(6) << de_min
        << "\n"
        << std::flush;
      return de_min;
    };

    //! Perform annealing
    void anneal(double ti, double tf, double delta_t)
    {
      if (ti <= 0)
        throw std::invalid_argument{"ti > 0"};
      if (tf <= 0)
        throw std::invalid_argument{"tf > 0"};
      if (ti <= tf)
        throw std::invalid_argument{"ti > tf"};
      if (delta_t >= 1 || delta_t < 0)
        throw std::invalid_argument{"0 < delta_t < 1"};

      double temp   = ti;
      double e      = state_.energy();
      unsigned int   steps  = static_cast<uint>(round((log(tf) - log(ti)) / log(delta_t)));
      unsigned int   nlimit = nover_ / 50;

      std::cout
        << "starting " << steps << " simulated annealing steps"
        << " from temperature " << std::setiosflags(std::ios::fixed)
        << std::setprecision(4) << temp
        << " (delta=" << std::setiosflags(std::ios::fixed)
        << std::setprecision(4) << delta_t << ") ..."
        << "\n"
        << std::flush;
      for (unsigned int n = 1; n <= steps; n++)
        {
          unsigned int l = 0;
          unsigned int k = 0;
          for (k = 0; k < nover_; k++)
            {
              // mutate configuration
              state_.mutate();
              // compute delta energy
              double de = state_.delta_energy();
              if (metropolis(de, temp))
                {
                  // apply mutation to current configuration
                  state_.apply_mutation();
                  e += de;
                  l++;
                }
              if (l > nlimit) break;
            }
          // fix final energy to avoid accumulation of numerical errors in de
          e = state_.energy();

          std::cout
            << std::setw(3) << (100 * n / steps) << "%"
            << " T=" << std::fixed << std::setprecision(4) << temp
            << " E=" << std::fixed << std::setprecision(4) << e
            << " (" << l << " " << k << ") ..."
            << "\n"
            << std::flush;

          temp *= delta_t;
          if (l < 10)
            break;
        }
    };

  private:
    const double       CHI0                 = 0.9;
    const unsigned int STATE_SETUP_TRIES    = 10000;
    const unsigned int MAX_MUTATION_RETRIES = 1000000;

    std::mt19937_64                        rne_;
    std::uniform_real_distribution<double> urd_;

    unsigned int nover_;
    S &          state_;

    inline bool metropolis(double delta, double temp)
    {
      return delta < 0.0 || urd_(rne_) < exp(-delta / temp);
    };
  };
}

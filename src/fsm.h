#pragma once

#include <algorithm>
#include <functional>
#include <map>
#include <random>
#include <set>
#include <sstream>
#include <unordered_map>
#include <vector>

#include "regexp.h"

namespace fsm
{
  //! Default equi-probable partitioning on type T
  template <typename T>
  struct default_epp
  {
    unsigned int operator()(const T &) { return 1; };
  };

  //! Finite State Machine
  /*! In order to generate the *minimal* DFA for a given regular
   *  expression we repeatedly derive the regexp with respect to each
   *  letter of the alphabet, associating:
   *
   *  - each unique derivative with a **state**
   *  - each letter used in the derivative with a **transition**
   *
   *  The resulting fsm is *minimal*.
   */
  template <typename T, typename Epp = default_epp<T>>
  class Fsm
  {
  public:
    Fsm(){};

    //! Use regexp derivatives to build the fsm
    Fsm(const regexp::RegExp<T> &r)
      : rne_{}
    {
      std::random_device device;
      rne_.seed((static_cast<uint64_t>(device()) << 32) | device());

      unsigned int c = 0;
      for (const auto &l : r.alphabet())
        {
          alphabet_.push_back(l);
          alphabet_map_.insert(std::make_pair(l, c));
          c++;
        }
      // regexp -> state index
      regexp_map_t states{std::make_pair(r, 1)};
      build(r, 1, states);

      std::map<std::pair<std::pair<states_idx_t, states_idx_t>, uint>, uint> epp_m;
      for (const auto &t : trans_state_map_)
        {
          states_idx_t q0_idx = t.first.first;
          letter_idx_t l_idx  = t.first.second;
          letter_t     l      = alphabet_[l_idx];
          states_idx_t q1_idx = t.second;
          unsigned int         epp    = Epp{}(l);

          // insert state
          if (state_states_map_.find(q0_idx) == state_states_map_.end())
            state_states_map_.insert(std::make_pair(q0_idx, std::vector<states_idx_t>{q1_idx}));
          else
            state_states_map_.at(q0_idx).push_back(q1_idx);

          // insert letter
          auto trn_k = std::make_pair(q0_idx, q1_idx);
          auto epp_k = std::make_pair(trn_k, epp);
          if (trans_letters_map_.find(trn_k) == trans_letters_map_.end())
            {
              std::vector<std::vector<letter_idx_t>> lts_v{{l_idx}};
              trans_letters_map_.insert(std::make_pair(trn_k, lts_v));
              epp_m.insert(std::make_pair(epp_k, 0));
            }
          else
            {
              auto &lts_v = trans_letters_map_.at(trn_k);
              if (epp_m.find(epp_k) == epp_m.end())
                {
                  lts_v.push_back(std::vector<letter_idx_t>{l_idx});
                  epp_m.insert(std::make_pair(epp_k, lts_v.size() - 1));
                }
              else
                lts_v[epp_m.at(epp_k)].push_back(l_idx);
            }
        }

      // sort letters
      for (auto &t : trans_letters_map_)
        for (auto &p : t.second)
          std::sort(p.begin(), p.end(), [&](unsigned int a, unsigned int b) { return alphabet_[a] < alphabet_[b]; });
    };

    //! Print in Graphviz dot format
    void print(std::ostream &os) const
    {
      os << "digraph FSM {\n"
         << "  rankdir = LR;\n"
         << "  node [shape = plain];\n"
         << "  start;"
         << "  node [shape = doublecircle];\n";
      for (auto k : finals_)
        os << "  " << k << ";\n";
      os << "  node [shape = circle];\n"
         << "  start -> 1;\n";
      for (const auto &t : trans_letters_map_)
        {
          if (t.second.size() == 1)
            for (auto l_idx : t.second[0])
              os << "  "
                 << t.first.first
                 << " -> "
                 << t.first.second
                 << " [label=\"" << alphabet_[l_idx] << "\"];\n";
          else if (t.second.size() > 1)
            {
              for (size_t epp_idx = 0; epp_idx < t.second.size(); epp_idx++)
                os << "# epp " << epp_idx << "\n"
                   << "  "
                   << t.first.first
                   << " -> "
                   << t.first.second
                   << " [label=\"" << alphabet_[t.second[epp_idx][0]] << "... (" << t.second[epp_idx].size() << ")\"];\n";
            }
        }
      os << "}\n";
    };

    const std::string to_string() const
    {
      std::stringstream ss;
      print(ss);
      return ss.str();
    };

    //! Walk a random path through the fsm and generate a word
    const std::vector<T> sample() const
    {
      using dist_t = std::uniform_int_distribution<size_t>;
      std::vector<T> res;
      states_idx_t   q0 = 1;
      states_trace_.clear();
      states_trace_.push_back(q0);
      while (true)
        {
          bool stop = finals_.find(q0) != finals_.end();
          if (stop && dist_t{0, 1}(rne_) == 0) break;

          const auto &q1_i = state_states_map_.find(q0);
          if (q1_i == state_states_map_.end() || q1_i->second.empty())
            {
              if (stop) break;
              // dangling states should have been pruned
              throw std::runtime_error{"dangling state in fsm"};
            }
          const auto & q1_v  = q1_i->second;
          states_idx_t q1    = q1_v.size() > 1 ? q1_v[dist_t{0, q1_v.size() - 1}(rne_)] : q1_v[0];
          auto         trn_k = std::make_pair(q0, q1);
          const auto & epp_i = trans_letters_map_.find(trn_k);
          if (epp_i == trans_letters_map_.end() || epp_i->second.empty())
            {
              if (stop) break;
              // dangling states should have been pruned
              throw std::runtime_error{"dangling state in fsm"};
            }
          const auto &epp_v = epp_i->second;
          const auto &lts_v = epp_v.size() > 1 ? epp_v[dist_t{0, epp_v.size() - 1}(rne_)] : epp_v[0];
          auto        lt    = lts_v.size() > 1 ? lts_v[dist_t{0, lts_v.size() - 1}(rne_)] : lts_v[0];
          res.push_back(alphabet_[lt]);
          q0 = q1;
          states_trace_.push_back(q1);
        }
      return res;
    };

    //! Walk the same path of a previous sample but choosing different letters
    const std::vector<T> resample() const
    {
      if (states_trace_.size() < 2) return sample();
      using dist_t = std::uniform_int_distribution<size_t>;
      std::vector<T> res;
      for (auto q0_i = states_trace_.begin(); q0_i != states_trace_.end() - 1; ++q0_i)
        {
          auto        trn_k = std::make_pair(*q0_i, *(q0_i + 1));
          const auto &epp_i = trans_letters_map_.find(trn_k);
          if (epp_i == trans_letters_map_.end() || epp_i->second.empty())
            throw std::runtime_error{"dangling state in fsm resampling"};
          const auto &epp_v = epp_i->second;
          const auto &lts_v = epp_v.size() > 1 ? epp_v[dist_t{0, epp_v.size() - 1}(rne_)] : epp_v[0];
          auto        lt    = lts_v.size() > 1 ? lts_v[dist_t{0, lts_v.size() - 1}(rne_)] : lts_v[0];
          res.push_back(alphabet_[lt]);
        }
      return res;
    };

    //! Walk the same path of a previous sample but choosing the best
    //! letter according to the specified fitness function
    const std::vector<T> resample(std::function<double(uint, const std::vector<T> &, const T &)> fitness) const
    {
      if (states_trace_.size() < 2) return sample();
      std::vector<T> res;
      unsigned int   i = 0;
      for (auto q0_i = states_trace_.begin(); q0_i != states_trace_.end() - 1; ++q0_i)
        {
          auto        trn_k = std::make_pair(*q0_i, *(q0_i + 1));
          const auto &epp_i = trans_letters_map_.find(trn_k);
          if (epp_i == trans_letters_map_.end() || epp_i->second.empty())
            throw std::runtime_error{"dangling state in fsm resampling"};
          const auto &epp_v   = epp_i->second;
          double      fit_min = 0.0;
          int         fit_idx = -1;
          for (const auto &lts_v : epp_v)
            for (const auto &lt : lts_v)
              {
                double f = fitness(i, res, alphabet_[lt]);
                if (f < fit_min || fit_idx == -1)
                  {
                    fit_min = f;
                    fit_idx = static_cast<int>(lt);
                  }
              }
          if (fit_idx == -1) throw std::runtime_error{"could not determine fittest letter in resampling"};
          res.push_back(alphabet_[fit_idx]);
          i++;
        }
      return res;
    };

    //! Match a word against the fsm
    bool match(const std::vector<T> &w) const
    {
      states_idx_t s = 1;
      for (const auto &l : w)
        {
          const auto l_i = alphabet_map_.find(l);
          if (l_i == alphabet_map_.end())
            return false;
          const auto t_i = trans_state_map_.find(std::make_pair(s, l_i->second));
          if (t_i == trans_state_map_.end())
            return false;
          s = t_i->second;
        }
      return finals_.find(s) != finals_.end();
    };

  private:
    mutable std::mt19937_64 rne_;

    using letter_t     = T;
    using letter_idx_t = uint;
    using regexp_t     = regexp::RegExp<letter_t>;
    using states_idx_t = uint;
    using finals_t     = std::set<states_idx_t>;
    using regexp_map_t = std::unordered_map<regexp_t, states_idx_t>;

    // transition suitable for matching (q0, letter)
    // (q0 == letter) => q1
    using trans_t = std::pair<states_idx_t, letter_idx_t>;

    // states map used in matching
    // (q0 == letter) => q1
    // (q0_idx, letter_idx) => q1_idx
    using trans_state_map_t = std::map<trans_t, states_idx_t>;

    // states map used in sampling
    // q0_idx => {q1_idx,q1_idx',q1_idx'',...}
    using state_states_map_t = std::map<states_idx_t, std::vector<states_idx_t>>;

    // letters associated to each transition
    // (q0_idx, q1_idx) => {{l,l',l'',...}
    //                      {m,m',m'',...}
    //                      ...}
    // partitioned over equi-probable letter sets
    using trans_letters_map_t = std::map<std::pair<states_idx_t, states_idx_t>, std::vector<std::vector<letter_idx_t>>>;

    // alphabet
    std::vector<letter_t> alphabet_;
    std::unordered_map<letter_t, letter_idx_t> alphabet_map_;

    // final states
    finals_t finals_;

    // transitions in a form suitable for matching
    trans_state_map_t trans_state_map_;

    // transitions in a form suitable for sampling
    state_states_map_t  state_states_map_;
    trans_letters_map_t trans_letters_map_;

    // state trace
    mutable std::vector<states_idx_t> states_trace_;

    // add transition starting from q0 with letter l
    void build(const regexp_t &q0, states_idx_t q0_idx, const letter_t &l, letter_idx_t l_idx, regexp_map_t &regexp_map)
    {
      const regexp_t q1{q0.derivative(l)};
      if (q1 == regexp::RegExp<T>::zero) return;
      trans_t trans{q0_idx, l_idx};
      auto    q1_itr = regexp_map.find(q1);
      if (q1_itr == regexp_map.end())
        {
          // new state: add state and transition
          states_idx_t q1_idx = regexp_map.size() + 1;
          regexp_map.insert(std::make_pair(q1, q1_idx));
          if (q1.nu() == regexp::RegExp<T>::one)
            finals_.insert(q1_idx);
          trans_state_map_.insert(std::make_pair(trans, q1_idx));
          build(q1, q1_idx, regexp_map);
        }
      else
        {
          // existing state: add transition
          states_idx_t q1_idx = q1_itr->second;
          if (q1.nu() == regexp::RegExp<T>::one)
            finals_.insert(q1_idx);
          trans_state_map_.insert(std::make_pair(trans, q1_idx));
        }
    };

    // add every transition starting from q0
    void build(const regexp_t &q0, states_idx_t q0_idx, regexp_map_t &regexp_map)
    {
      for (const auto &l : alphabet_map_)
        build(q0, q0_idx, l.first, l.second, regexp_map);
    };
  };

  //! Stream output
  template <typename T, typename E>
  inline std::ostream &operator<<(std::ostream &os, const Fsm<T, E> &m)
  {
    m.print(os);
    return os;
  };
}

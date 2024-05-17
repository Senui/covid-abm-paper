#ifndef UPDATE_STATISTICS_OP_H_
#define UPDATE_STATISTICS_OP_H_

#include <numeric>

#include "core/operation/operation.h"
#include "core/operation/operation_registry.h"
#include "core/simulation.h"
#include "core/util/thread_info.h"

#include "model_facts.h"
#include "person.h"
#include "sim_param.h"

namespace bdm {

class UpdateStatisticsOp : public StandaloneOperationImpl {
 public:
  BDM_OP_HEADER(UpdateStatisticsOp);

  void Reset() {
    const auto max_threads = ThreadInfo::GetInstance()->GetMaxThreads();

#pragma omp parallel for
    for (auto m = 0; m < kNumMunicipalities; m++) {
      infected_home_tl_[m] = SharedData<uint64_t>(max_threads, 0);
      for (auto d = 0; d < kNumDemographies; d++) {
        infected_tl_[m][d] = SharedData<uint64_t>(max_threads, 0);
        total_tl_[m][d] = SharedData<uint64_t>(max_threads, 0);
      }
    }
  }

  // Calculates the fractions: infected persons divided by total numer of
  // persons per municipality for each demograpic group
  void CalculateFractions() {
    auto update_counts = L2F([&, this](Agent* agent) {
      auto tid = ThreadInfo::GetInstance()->GetMyThreadId();
      auto* person = bdm_static_cast<Person*>(agent);
      auto municipality = person->location_;
      auto home = person->home_location_;
      auto demography = person->demography_;
      total_tl_[municipality][demography][tid]++;
      if (person->state_ == State::kInfectious) {
        infected_tl_[municipality][demography][tid]++;
        infected_home_tl_[home][tid]++;
      }
    });

    auto* rm = Simulation::GetActive()->GetResourceManager();
    rm->ForEachAgentParallel(update_counts);

    auto combine_tl_results = [](const SharedData<uint64_t>& tl_results) {
      uint64_t result = 0;
      for (auto& el : tl_results) {
        result += el;
      }
      return result;
    };

#pragma omp parallel for
    for (auto m = 0; m < kNumMunicipalities; m++) {
      auto infected_home = combine_tl_results(infected_home_tl_[m]);
      infected_home_[m] = infected_home;
      for (auto d = 0; d < kNumDemographies; d++) {
        auto total = combine_tl_results(total_tl_[m][d]);
        auto infected = combine_tl_results(infected_tl_[m][d]);
        total_[m][d] = total;
        infected_[m][d] = infected;
        if (total != 0) {
          fractions_[m][d] = static_cast<real_t>(infected) / total;
        }
      }
    }
  
    auto* sparam = Simulation::GetActive()->GetParam()->Get<SimParam>();
    auto f = sparam->export_infected_per_timestep_frequency;
    auto timestep = Simulation::GetActive()->GetScheduler()->GetSimulatedSteps();
    std::vector<real_t> total_inf_at_timestep(infected_home_.begin(), infected_home_.end());
    total_infected_per_municipality_.push_back(total_inf_at_timestep);
    if (f != 0 && (timestep % f == 0)) {
      std::vector<real_t> total_at_timestep(kNumMunicipalities);
#pragma omp parallel for
      for (auto m = 0; m < kNumMunicipalities; m++) {
        total_at_timestep[m] = std::accumulate(total_[m].begin(), total_[m].end(), 0);
      }
      total_per_municipality_.push_back(total_at_timestep);
    }
  }

  void operator()() override {
    Reset();
    CalculateFractions();
  }

  // The fraction of infected / total number of people per demography, per municipality at given timestep
  std::array<std::array<real_t, kNumDemographies>, kNumMunicipalities>
      fractions_;
  // The total number of infected people per demography, per municipality at given timestep
  std::array<std::array<uint64_t, kNumDemographies>, kNumMunicipalities>
      infected_;
  // The total number of infected people per home municipality at given timestep
  std::array<uint64_t, kNumMunicipalities> infected_home_;
  // The total number of people per demography, per municipality at given timestep
  std::array<std::array<uint64_t, kNumDemographies>, kNumMunicipalities> total_;
  // The total number of infected people per home municipality (summed over all demographic groups) at each timestep
  std::vector<std::vector<real_t>> total_infected_per_municipality_;
  // The total number of people per municipality (summed over all demographic groups) at each timestep
  std::vector<std::vector<real_t>> total_per_municipality_;

 private:
  std::array<std::array<SharedData<uint64_t>, kNumDemographies>,
             kNumMunicipalities>
      infected_tl_;
  std::array<SharedData<uint64_t>, kNumMunicipalities> infected_home_tl_;
  std::array<std::array<SharedData<uint64_t>, kNumDemographies>,
             kNumMunicipalities>
      total_tl_;
};

}  // namespace bdm

#endif  // UPDATE_STATISTICS_OP_H_

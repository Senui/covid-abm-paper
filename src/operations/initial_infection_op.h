#ifndef INITIAL_INFECTION_OP_H_
#define INITIAL_INFECTION_OP_H_

#include <algorithm>
#include <fstream>

#include "core/operation/operation.h"
#include "core/operation/operation_registry.h"
#include "core/simulation.h"

#include "csv_helper.h"
#include "model_facts.h"
#include "operations/update_statistics_op.h"
#include "person.h"
#include "sim_param.h"

namespace bdm {

class InitialInfectionOp : public StandaloneOperationImpl {
 public:
  // Estimated number of initial infections per municipality per day of the first two weeks of the first wave based on RIVM data
  std::vector<int> initial_infected_;
  std::vector<real_t> infection_fraction_list_;
  bool initialized_ = false;
  BDM_OP_HEADER(InitialInfectionOp);

  void Initialize() {
    real_t agent_to_person_ratio = GetAgentToPersonRatio();
    std::string data_dir = GetDataDir();
    std::string init_infected_file =
        data_dir + "/initial_infected_per_municipality_per_day.csv";

    CsvToVector<int>(init_infected_file, &initial_infected_, 0, 0);
    infection_fraction_list_.resize(kNumMunicipalities);
    initialized_ = true;
  }

  void operator()() override {
    auto* sim = Simulation::GetActive();
    auto* sparam = sim->GetParam()->Get<SimParam>();
    auto* rm = bdm_static_cast<RandomizedRm<ResourceManager>*>(
      sim->GetResourceManager());
    if (!initialized_) {
      Initialize();
      // Reorder agent order so that ForEachAgent randomly iterates through agents
      rm->RandomizeAgentsOrder();
    }
    real_t agent_to_person_ratio = GetAgentToPersonRatio();
    auto timestep = sim->GetScheduler()->GetSimulatedSteps();
    // Only add new infections per day as given by the RIVM
    if ((timestep % kHoursPerDay != 0)) {
      return;
    }
    auto day = static_cast<int>(timestep / kHoursPerDay);
    if (day > initial_infected_.size() / kNumMunicipalities) {
      return;
    }

    // Introduce a delay between the exposed (see below) and the infection initialization
    if (timestep > sparam->incubation_scale_param) {
      // Initialize initial infected people
#pragma omp parallel for
      for (size_t m = 0; m < kNumMunicipalities; m++) {
        int infection_count = 0;
        auto agents_to_infect = initial_infected_[day * kNumMunicipalities + m] / agent_to_person_ratio + infection_fraction_list_[m];
        auto integer_part = static_cast<int>(std::floor(agents_to_infect));
        auto fraction_part = agents_to_infect - integer_part;
        infection_fraction_list_[m] = fraction_part;
        if (integer_part == 0) {
          continue;
        }

        auto is_in_municipality = L2F([&](Agent* a) {
          return bdm_static_cast<Person*>(a)->home_location_ == m;
        });
        rm->ForEachAgent(
            [&](Agent* a) {  // NOLINT
              auto* person = bdm_static_cast<Person*>(a);
              if ((infection_count < integer_part) &&
                  (person->state_ == State::kSusceptible)) {
                person->state_ = State::kInfectious;
                person->RandomlyInitializeStateThreshold();
                infection_count++;
              }
            },
            &is_in_municipality);
      }
    }

    // Initialize initial exposed people
    // Assumption: linear relation between infection and exposure cases: E =
    // initial_exposed_infected_ratio * I
    real_t initial_exposed_infected_ratio =
        sparam->initial_exposed_infected_ratio;
#pragma omp parallel for
    for (size_t m = 0; m < kNumMunicipalities; m++) {
      int exposed_count = 0;
      auto agents_to_infect = initial_infected_[day * kNumMunicipalities + m] / agent_to_person_ratio + infection_fraction_list_[m];
      auto integer_part = static_cast<int>(std::floor(agents_to_infect));
      auto fraction_part = agents_to_infect - integer_part;
      infection_fraction_list_[m] = fraction_part;
      if (integer_part == 0) {
        continue;
      }

      auto is_in_municipality = L2F([&](Agent* a) {
        return bdm_static_cast<Person*>(a)->home_location_ == m;
      });
      rm->ForEachAgent(
          [&](Agent* a) {  // NOLINT
            auto* person = bdm_static_cast<Person*>(a);
            if ((exposed_count < integer_part * initial_exposed_infected_ratio) &&
                (person->state_ == State::kSusceptible)) {
              person->state_ = State::kExposed;
              person->RandomlyInitializeStateThreshold();
              exposed_count++;
            }
          },
          &is_in_municipality);
    }
  }
};

}  // namespace bdm

#endif  // INITIAL_INFECTION_OP_H_

#ifndef INTERVENTIONS_H_
#define INTERVENTIONS_H_

#include "core/randomized_rm.h"
#include "sim_param.h"

namespace bdm {

inline void MobilityReductionPhase2() {
  auto* sim = Simulation::GetActive();
  auto* rm = bdm_static_cast<RandomizedRm<ResourceManager>*>(
      sim->GetResourceManager());
  auto* sparam = sim->GetParam()->Get<SimParam>();
  auto num_homeworkers =
      sparam->phase_2_mobility_reduction * rm->GetNumAgents();
  std::cout << "Intervention: " << sparam->phase_2_mobility_reduction
            << "\% of working class start working from home" << std::endl;
  uint32_t counter = 0;
  std::array<Demographic, 2> working_class = {kHigherAgeWorking,
                                              kMiddleAgeWorking};
  auto put_at_home = [&](Agent* agent) {
    if (counter < num_homeworkers) {
      auto* person = bdm_static_cast<Person*>(agent);
      auto* is_found = std::find(std::begin(working_class),
                                 std::end(working_class), person->demography_);
      if (is_found != working_class.end()) {
        person->home_stay_ = true;
        counter++;
      }
    }
  };
  rm->RandomizeAgentsOrder();
  rm->ForEachAgent(put_at_home);
}

inline void MobilityReductionPhase3() {
  auto* sim = Simulation::GetActive();
  auto* rm = bdm_static_cast<RandomizedRm<ResourceManager>*>(
      sim->GetResourceManager());
  auto* sparam = sim->GetParam()->Get<SimParam>();
  // only add the difference between phase 1 and 2 to homeworking state
  auto num_homeworkers =
      sparam->phase_3_mobility_reduction * rm->GetNumAgents() -
      sparam->phase_2_mobility_reduction * rm->GetNumAgents();
  std::cout << "Intervention: " << sparam->phase_3_mobility_reduction
            << "\% of working class start working from home" << std::endl;
  uint32_t counter = 0;
  std::array<Demographic, 2> working_class = {kHigherAgeWorking,
                                              kMiddleAgeWorking};
  auto put_at_home = [&](Agent* agent) {
    if (counter < num_homeworkers) {
      auto* person = bdm_static_cast<Person*>(agent);
      auto* is_found = std::find(std::begin(working_class),
                                 std::end(working_class), person->demography_);
      // don't put people that are already homestaying at home
      if (is_found != working_class.end() && !person->home_stay_) {
        person->home_stay_ = true;
        counter++;
      }
    }
  };
  rm->RandomizeAgentsOrder();
  rm->ForEachAgent(put_at_home);
}

inline void SchoolClosure() {
  auto* sim = Simulation::GetActive();
  auto* rm = bdm_static_cast<RandomizedRm<ResourceManager>*>(
      sim->GetResourceManager());
  auto* sparam = sim->GetParam()->Get<SimParam>();
  std::cout << "Intervention: "
            << "all schools are closed" << std::endl;
  std::array<Demographic, 3> children_demography = {
      kPreSchoolChildren, kPrimarySchoolChildren, kSecondarySchoolChildren};
  auto put_at_home = L2F([&](Agent* agent, AgentHandle ah) {
    auto* person = bdm_static_cast<Person*>(agent);
    auto* is_found =
        std::find(std::begin(children_demography),
                  std::end(children_demography), person->demography_);
    if (is_found != children_demography.end()) {
      person->home_stay_ = true;
    }
  });
  rm->RandomizeAgentsOrder();
  rm->ForEachAgentParallel(put_at_home);

  auto homeschooling_parents =
      sparam->phase_2_homeschooling_parents * rm->GetNumAgents();
  uint32_t counter = 0;
  auto put_at_home2 = [&](Agent* agent) {
    if (counter < homeschooling_parents) {
      auto* person = bdm_static_cast<Person*>(agent);
      // Check if person is not already homestaying (avoid double-counting
      // homestayers)
      if (person->demography_ == kMiddleAgeWorking && !person->home_stay_) {
        counter++;
        person->home_stay_ = true;
      }
    }
  };
  rm->RandomizeAgentsOrder();
  rm->ForEachAgent(put_at_home2);
}

inline void AdjustMixingMatrices(uint8_t phase) {
  auto* sim = Simulation::GetActive();
  auto* env = bdm_static_cast<CovidEnvironment*>(sim->GetEnvironment());
  
  std::string data_dir = GetDataDir();
  std::string full_path;
  if (phase < 2) {
    full_path = Concat(data_dir, "/", "mixmat_phase2.csv");
  } else {
    full_path = Concat(data_dir, "/", "mixmat_phase4.csv");
  }

  std::array<std::array<real_t, kNumDemographies>, kNumDemographies> reduction_matrix;
  CsvTo2DMatrix<real_t, kNumDemographies>(full_path, &reduction_matrix);

  env->ApplyReduction(reduction_matrix);
}

}  // namespace bdm

#endif  // INTERVENTIONS_H_

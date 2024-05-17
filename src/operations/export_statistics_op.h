#ifndef EXPORT_STATISTICS_H_
#define EXPORT_STATISTICS_H_

#include <fstream>

#include "core/operation/operation.h"
#include "core/operation/operation_registry.h"
#include "core/simulation.h"

#include "model_facts.h"
#include "behaviors/infection_behavior.h"
#include "person.h"

namespace bdm {

inline real_t CountInteractions(Person* person) {
  real_t ret = 0;

  auto* sim = Simulation::GetActive();
  auto* env = bdm_static_cast<CovidEnvironment*>(sim->GetEnvironment());

  auto demography = person->demography_;
  const auto mix_mat = env->GetMixingMatrix(person->situation_);

  // fetch contact matrix
  for (auto other_demo = 0; other_demo < kNumDemographies; other_demo++) {
    ret += mix_mat[demography][other_demo];
  }
  return ret;
}

class ExportStatisticsOp : public StandaloneOperationImpl {
 public:
  std::vector<real_t> avg_person_interactions_over_time;
  BDM_OP_HEADER(ExportStatisticsOp);

  void operator()() override {
    auto* sim = Simulation::GetActive();
    auto* rm = sim->GetResourceManager();
    real_t total_interactions = 0;
    auto get_mixsum = [&](Agent* a) {
      auto* person = bdm_static_cast<Person*>(a);
      auto num_interactions = CountInteractions(person);
      total_interactions += num_interactions;
    };
    rm->ForEachAgent(get_mixsum);
    
    auto scaling = GetAgentToPersonRatio();
    auto num_agents = rm->GetNumAgents();
    avg_person_interactions_over_time.push_back((total_interactions / num_agents) * scaling);
  }
};

}  // namespace bdm

#endif  // EXPORT_STATISTICS_H_

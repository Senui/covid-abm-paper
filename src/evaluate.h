#ifndef EVALUATE_H_
#define EVALUATE_H_

#include <cmath>
#include <vector>

#include "biodynamo.h"
#include "person.h"

#include "model_facts.h"
#include "operations/update_statistics_op.h"
#include "sim_param.h"

using namespace bdm::experimental;

namespace bdm {

inline void SetupResultCollection(Simulation* sim) {
  auto* ts = sim->GetTimeSeries();
  // auto susceptible = [](Agent* a) {
  //   return bdm_static_cast<Person*>(a)->state_ == State::kSusceptible;
  // };
  auto exposed = [](Agent* a) {
    return bdm_static_cast<Person*>(a)->state_ == State::kExposed;
  };
  auto infectious = [](Agent* a) {
    return bdm_static_cast<Person*>(a)->state_ == State::kInfectious;
  };
  auto hospitalized = [](Agent* a) {
    return bdm_static_cast<Person*>(a)->hospitalized_;
  };

  // Eindhoven = 93, Groningen = 118, Den Haag = 117
  auto hospitalized_eindhoven = [](Agent* a) {
    auto *p = bdm_static_cast<Person*>(a);
    return p->hospitalized_ && p->home_location_ == 93;
  };
  auto hospitalized_groningen = [](Agent* a) {
    auto *p = bdm_static_cast<Person*>(a);
    return p->hospitalized_ && p->home_location_ == 118;
  };
  auto hospitalized_denhaag = [](Agent* a) {
    auto *p = bdm_static_cast<Person*>(a);
    return p->hospitalized_ && p->home_location_ == 117;
  };

  // We count the number of agents here, so we need to multiple by a scaling
  // factor to get back the number of people for our final results
  auto agent_to_person_scaling = [](real_t total_agent_count) {
    auto scaling = GetAgentToPersonRatio();
    return total_agent_count * scaling;
  };

  bool export_affected = sim->GetParam()->Get<SimParam>()->export_affected;
  if (export_affected) {
    for (int i = kPreSchoolChildren; i != kEldest + 1; i++) {
      auto collector_demo = L2F([=](Agent* a) {
        auto* person = bdm_static_cast<Person*>(a);
        std::array<State, 3> affected = {kExposed, kInfectious, kRecovered};
        auto* is_affected =
            std::find(std::begin(affected), std::end(affected), person->state_);
        return (person->demography_ == static_cast<Demographic>(i)) &&
              (is_affected != affected.end());
      });

      auto post_process = L2F([=](real_t total_affected) {
        auto stats = Simulation::GetActive()
                        ->GetScheduler()
                        ->GetOps("update statistics")[0]
                        ->GetImplementation<UpdateStatisticsOp>();
        uint64_t population_per_demography = 0;
        for (size_t m = 0; m < stats->total_.size(); m++) {
          population_per_demography += stats->total_[m][i];
        }
        return static_cast<real_t>(total_affected) / population_per_demography;
      });

      ts->AddCollector(
          Concat("ts_affected_", DemographicToString[i]),
          new CounterT<decltype(collector_demo), decltype(post_process), real_t>(
              collector_demo, post_process));
    }
  }

  // auto recovered = [](Agent* a) {
  //   return bdm_static_cast<Person*>(a)->state_ == State::kRecovered;
  // };
  // ts->AddCollector("susceptible", new Counter<real_t>(susceptible));
  ts->AddCollector("ts_exposed",
                   new Counter<real_t>(exposed, agent_to_person_scaling));
  ts->AddCollector("ts_infectious",
                   new Counter<real_t>(infectious, agent_to_person_scaling));
  ts->AddCollector("ts_hospitalized",
                   new Counter<real_t>(hospitalized, agent_to_person_scaling));
  ts->AddCollector("ts_hospitalized_eindhoven",
                   new Counter<real_t>(hospitalized_eindhoven, agent_to_person_scaling));
  ts->AddCollector("ts_hospitalized_groningen",
                   new Counter<real_t>(hospitalized_groningen, agent_to_person_scaling));
  ts->AddCollector("ts_hospitalized_denhaag",
                   new Counter<real_t>(hospitalized_denhaag, agent_to_person_scaling));
  // ts->AddCollector("recovered", new Counter<real_t>(recovered));
}

}  // namespace bdm

#endif  // EVALUATE_H_

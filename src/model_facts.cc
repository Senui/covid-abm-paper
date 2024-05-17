#include "model_facts.h"
#include "person.h"
#include "sim_param.h"

using namespace bdm;

real_t bdm::GetAgentToPersonRatio() {
  auto* sim = Simulation::GetActive();
  if (sim->GetParam()->Get<SimParam>()->custom_agent_to_person_ratio != 0) {
    return sim->GetParam()->Get<SimParam>()->custom_agent_to_person_ratio;
  }
  auto num_agents = sim->GetResourceManager()->GetNumAgents();
  auto num_persons = kTotalPopulationSize;
  if (num_agents == num_persons) {
    return 1;
  }
  return static_cast<real_t>(num_persons) / num_agents;
}

void PrintStateDistribution() {
  std::array<int, 4> states{};
  Simulation::GetActive()->GetResourceManager()->ForEachAgent([&](Agent* a) {
    auto* person = bdm_static_cast<Person*>(a);
    if (person->state_ == State::kSusceptible) {
      states[0]++;
    } else if (person->state_ == State::kExposed) {
      states[1]++;
    } else if (person->state_ == State::kInfectious) {
      states[2]++;
    } else if (person->state_ == State::kRecovered) {
      states[3]++;
    }
  });

  std::cout << "=============================" << std::endl;
  std::cout << "Susceptible\t: " << states[0] << std::endl
            << "Exposed\t\t: " << states[1] << std::endl
            << "Infectious\t: " << states[2] << std::endl
            << "Recovered\t: " << states[3] << std::endl;
  std::cout << "=============================" << std::endl;
}

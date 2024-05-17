#include "person.h"
#include "behaviors/infection_behavior.h"

using namespace bdm;

void Person::RandomlyInitializeStateThreshold() {
  auto behaviors = this->GetAllBehaviors();
  for (auto* bh : behaviors) {
    if (InfectionBehavior* inf_bh = dynamic_cast<InfectionBehavior*>(bh)) {
      inf_bh->DrawFromDistributions(this->state_);
    }
  }
}

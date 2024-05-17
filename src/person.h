#ifndef PERSON_H_
#define PERSON_H_

#include "biodynamo.h"

#include "model_facts.h"

class InfectionBehavior;

namespace bdm {

class Person : public Agent {
  BDM_AGENT_HEADER(Person, Agent, 1);

 public:
  Person(Demographic demography = Demographic::kElderly, uint8_t age = 0,
         Gender gender = Gender::kMale, uint16_t location = 0,
         uint16_t home_location = 0, State state = State::kSusceptible)
      : demography_(demography),
        age_(age),
        gender_(gender),
        location_(location),
        home_location_(home_location),
        state_(state) {
    weekly_travel_schedule_.resize(kHoursPerDay * kDaysPerWeek);
    traveler_type_ = kDemographyToTravelType[demography];
  }

  // Overwrite abstract methods from Base class
  Shape GetShape() const override { return Shape::kSphere; }
  real_t GetDiameter() const override { return 0; }
  const Real3& GetPosition() const override {
    static Real3 kDefault{0, 0, 0};
    return kDefault;
  }
  void SetDiameter(real_t diameter) override {}
  void SetPosition(const Real3& position) override {}
  Real3 CalculateDisplacement(const InteractionForce* force,
                              real_t squared_radius, real_t dt) override {
    return {0, 0, 0};
  }
  void ApplyDisplacement(const Real3& displacement) override {}
  // Overwrite end

  uint16_t GetHomeLocation() { return home_location_; }

  void Travel(uint16_t destination) { location_ = destination; }
  std::vector<uint16_t>* GetWeeklyTravelSchedule() {
    return &weekly_travel_schedule_;
  }

  void RandomlyInitializeStateThreshold();

  //  private:
  friend class MobilityData;
  friend struct InfectionBehavior;
  friend struct ChangeSituationBehavior;
  friend class UpdateStatisticsOp;
  Demographic demography_;
  uint8_t age_;
  Gender gender_;
  uint16_t location_;
  uint16_t home_location_;
  TravelerType traveler_type_;
  State state_;
  Situation situation_;
  bool hospitalized_ = false;
  bool home_stay_ = false;
  std::vector<uint16_t> weekly_travel_schedule_;
};

}  // namespace bdm

#endif  // PERSON_H_

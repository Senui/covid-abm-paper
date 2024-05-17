#ifndef MODEL_FACTS_H_
#define MODEL_FACTS_H_

#include <stdint.h>
#include <array>
#include <utility>

#include "core/container/math_array.h"
#include "core/resource_manager.h"
#include "core/simulation.h"

using std::make_pair;

namespace bdm {

inline void RemoveQuotes(std::string* s) {
  s->erase(remove(s->begin(), s->end(), '\"'), s->end());
}

inline std::string GetDataDir() {
  std::string data_dir = DATA_DIR;
  RemoveQuotes(&data_dir);
  return data_dir;
}

static const uint16_t kNumDemographies = 11u;
static const uint16_t kNumMunicipalities = 380u;
static const uint16_t kHoursPerDay = 24u;
static const uint16_t kDaysPerWeek = 7u;
static const uint64_t kTotalPopulationSize = 17181084;

// Returns how many persons one agent represents
real_t GetAgentToPersonRatio();

void PrintStatesDistribution();

enum Demographic {
  kPreSchoolChildren,
  kPrimarySchoolChildren,
  kSecondarySchoolChildren,
  kStudents,
  kNonStudyingAdolescents,
  kMiddleAgeWorking,
  kMiddleAgeUnemployed,
  kHigherAgeWorking,
  kHigherAgeUnemployed,
  kElderly,
  kEldest
};

static std::array<std::string, kNumDemographies> DemographicToString = {
    "PreSchoolChildren",
    "PrimarySchoolChildren",
    "SecondarySchoolChildren",
    "Students",
    "NonStudyingAdolescents",
    "MiddleAgeWorking",
    "MiddleAgeUnemployed",
    "HigherAgeWorking",
    "HigherAgeUnemployed",
    "Elderly",
    "Eldest"};

enum Gender { kMale, kFemale };
enum TravelerType { kFrequent, kIncidental };
enum Situation { kHome, kWork, kSchool, kWorkSchool, kOther };
enum State { kSusceptible, kExposed, kInfectious, kRecovered };

static std::array<std::string, kNumDemographies> StateToString = {
    "susceptible", "exposed", "infectious", "recovered"};

static uint8_t kActivePhase = 0;

const static std::array<TravelerType, kNumDemographies>
    kDemographyToTravelType = {kIncidental, kFrequent,   kFrequent,   kFrequent,
                               kIncidental, kFrequent,   kIncidental, kFrequent,
                               kIncidental, kIncidental, kIncidental};

// The mixing situation in the home municipality
const static std::array<Situation, kNumDemographies> kDayTimeMixingHome = {
    kHome, kSchool, kSchool, kWorkSchool, kWork, kWork,
    kHome, kWork,   kHome,   kHome,       kHome};

// The mixing situation in municipalities other than the home municipality
const static std::array<Situation, kNumDemographies> kDayTimeMixingOther = {
    kOther, kSchool, kSchool, kWorkSchool, kWork, kWork,
    kOther, kWork,   kOther,  kOther,      kOther};

const static std::array<real_t, kNumDemographies> kDemographyHomeStayScaling = {
    1.5, 1.5, 1.5, 1, 1, 1, 1.5, 1, 1.5, 1.5, 1.5};

// The population fraction per demographic group
static const std::array<real_t, kNumDemographies>
    kNationalFractionPerDemography = {0.049, 0.075, 0.057, 0.062, 0.036, 0.318,
                                      0.071, 0.093, 0.064, 0.122, 0.046};
// Probability of hospitalization per demographic group
static const std::array<real_t, kNumDemographies>
    kHospitalizationPerDemography = {0.0,    0.0,    0.0018, 0.0006,
                                     0.0006, 0.0081, 0.0081, 0.0276,
                                     0.0276, 0.0494, 0.0641};
// The (minimum age, maximum age) per demographic group
static const std::array<std::pair<uint8_t, uint8_t>, kNumDemographies>
    kAgeLimitsPerDemography{
        make_pair(0, 4),   make_pair(5, 11),  make_pair(12, 16),
        make_pair(17, 24), make_pair(17, 24), make_pair(25, 54),
        make_pair(25, 54), make_pair(55, 67), make_pair(55, 67),
        make_pair(68, 80), make_pair(80, 110)};

// Weights indicating at which hours a person is likely to be awake during a day
static MathArray<real_t, kHoursPerDay> kAwake = {
    0, 0, 0, 0, 0, 0, 0.25, 0.5, 0.75, 1,   1,   1,
    1, 1, 1, 1, 1, 1, 1,    0.8, 0.6,  0.4, 0.2, 0};

static const MathArray<real_t, kHoursPerDay> kDailySleepPattern =
    kAwake / kAwake.Sum();

static MathArray<float, kNumDemographies> kAbsoluteSusceptibility = {
    1.0, 2.0, 3.051, 5.751, 5.751, 3.6, 3.6, 5.0, 5.0, 5.3, 7.2};

static const MathArray<float, kNumDemographies> kSusceptibility =
    (kAbsoluteSusceptibility / kAbsoluteSusceptibility.Sum()) *
    kNumDemographies;

}  // namespace bdm

#endif  // MODEL_FACTS_H_

#include "initialization.h"

namespace bdm {

void InitializeMobilityData() {
  auto mobility_data = MobilityData::GetInstance();
  std::string data_dir = GetDataDir();

  // Read in M_freq.csv
  std::string full_path = data_dir + "/M_freq.csv";
  CsvTo2DMatrix(full_path, &(mobility_data->m_freq_));

  // Read in M_inc.csv
  full_path = data_dir + "/M_inc.csv";
  CsvTo2DMatrix(full_path, &(mobility_data->m_inc_));

  // Read in municipality population
  full_path = data_dir + "/inwoners_gemeente_2018.csv";
  CsvToVector(full_path, &(mobility_data->municipality_population_), 1, 0);

  // Read in municipality names
  full_path = data_dir + "/Gemeenten2018.csv";
  CsvToVector(full_path, &(mobility_data->municipality_codes_), 0, 0);
}

void InitializeWeeklyTravelSchedule(Person* person) {
  auto mobility_data = MobilityData::GetInstance();
  auto* schedule = person->GetWeeklyTravelSchedule();

  auto* simulation = Simulation::GetActive();
  auto* random = simulation->GetRandom();
  auto* sparam = simulation->GetParam()->Get<SimParam>();
  uint8_t homestay_hours =
      std::round(random->Gaus(sparam->homestay_mean, sparam->homestay_sigma));
  // Confine to 1 - 23 of homestay hours
  homestay_hours = homestay_hours > 23 ? 23 : homestay_hours;
  homestay_hours = homestay_hours < 1 ? 1 : homestay_hours;
  uint8_t away_hours = kHoursPerDay - homestay_hours;

  // A person is at the home municipality from hour 0 - `first_half_home` and
  // from `second_half_home` to hour 23. A person can be in any other
  // municipality in between these periods (but also home)
  uint8_t first_half_home = std::floor(homestay_hours / 2.0f);
  uint8_t second_half_home = kHoursPerDay - std::ceil(homestay_hours / 2.0f);

  assert(second_half_home <= kHoursPerDay);
  assert(kHoursPerDay - second_half_home + first_half_home == homestay_hours);

  size_t idx = 0;
  for (size_t day = 0; day < kDaysPerWeek; day++) {
    std::vector<uint16_t> other_locations(away_hours);
    mobility_data->DrawDirichlet(person, &other_locations);
    for (size_t hour = 0; hour < kHoursPerDay; hour++) {
      if (hour < first_half_home || hour >= second_half_home) {
        (*schedule)[idx] = person->GetHomeLocation();
      } else {  // Else a person is in a location based on the mobility data
        (*schedule)[idx] = other_locations[hour - first_half_home];
      }
      idx++;
    }
  }
}

// Determine the demographic group based on the workstatus of a person
Demographic WorkstatusToDemographic(int workstatus, uint8_t age) {
  Demographic d;
  if (age <= 4) {
    d = kPreSchoolChildren;
  } else if (age > 4 && age <= 11) {
    d = kPrimarySchoolChildren;
  } else if (age > 11 && age <= 16) {
    d = kSecondarySchoolChildren;
  } else if (age > 16 && age <= 24) {
    // TODO: check if this is correct
    if (workstatus == 26 || workstatus == 31) {
      d = kStudents;
    } else {
      d = kNonStudyingAdolescents;
    }
  } else if (age > 24 && age <= 54) {
    if (workstatus >= 11 && workstatus <= 15) {
      d = kMiddleAgeWorking;
    } else {
      d = kMiddleAgeUnemployed;
    }
  } else if (age > 54 && age <= 67) {
    if (workstatus >= 11 && workstatus <= 15) {
      d = kHigherAgeWorking;
    } else {
      d = kHigherAgeUnemployed;
    }
  } else if (age > 67 && age <= 80) {
    d = kElderly;
  } else {
    d = kEldest;
  }
  return d;
}

uint32_t MunicipalityToLocation(
    uint32_t municipality, const std::vector<uint32_t>& municipality_codes) {
  auto itr = std::find(municipality_codes.begin(), municipality_codes.end(),
                       municipality);
  if (itr == municipality_codes.cend()) {
    Log::Fatal("Could not find municipality '", municipality,
               "' in list of valid municipalities");
  }
  return std::distance(municipality_codes.begin(), itr);
}

Person* CreatePerson(Gender gender, uint8_t age, Demographic d,
                     uint16_t municipality) {
  Person* person = new Person(d, age, gender, municipality, municipality,
                              State::kSusceptible);

  person->AddBehavior(new ChangeSituationBehavior());
  person->AddBehavior(new TravelBehavior());
  person->AddBehavior(new InfectionBehavior());

  InitializeWeeklyTravelSchedule(person);
  return person;
}

void InitializePopulation(std::string pop_dir_file, bool randinit) {
  auto* sim = Simulation::GetActive();
  auto* rm = bdm_static_cast<RandomizedRm<ResourceManager>*>(
      sim->GetResourceManager());
  auto* param = sim->GetParam();
  const auto* sparam = param->Get<SimParam>();

  if (sparam->no_fixed_seed) {
    rand.SetSeed(0);
    for (auto& r : MobilityData::GetInstance()->r_RNG) {
      gsl_rng_set(r, time(NULL));
    }
    generator.seed(time(NULL));
  }

  InitializeMobilityData();

  // A random population initialization (for local testing)
  if (randinit) {
    std::cout << "Initializing with random population" << std::endl;
    size_t pop_count = 0;
    for (size_t d = 0; d < kNumDemographies; d++) {
      size_t pop_per_demo = std::round(kNationalFractionPerDemography[d] *
                                       sparam->population_size);
      // Add the remaining population to the last group to get exactly the
      // request population_size
      if (d == kNumDemographies - 1) {
        pop_per_demo = sparam->population_size - pop_count;
      }
      pop_count += pop_per_demo;
#pragma omp parallel
      {
        auto* ctxt = sim->GetExecutionContext();
#pragma omp for
        for (size_t p = 0; p < pop_per_demo; p++) {
          auto age_limit = kAgeLimitsPerDemography[d];
          int age = rand.Uniform(age_limit.first, age_limit.second);
          Gender gender = static_cast<Gender>(std::round(rand.Uniform(0, 1)));
          uint16_t location = rand.Uniform(0, kNumMunicipalities);
          location = location > kNumMunicipalities - 1 ? kNumMunicipalities - 1
                                                       : location;
          auto* new_person =
              CreatePerson(gender, age, static_cast<Demographic>(d), location);
          ctxt->AddAgent(new_person);
        }
      }
      // Adds agents to ResourceManager
      sim->GetScheduler()->FinalizeInitialization();
    }
    std::cout << "agent count = " << pop_count << std::endl;
    std::cout << "rm->GetNumAgents = " << rm->GetNumAgents() << std::endl;

    auto pop_per_municipality =
        MobilityData::GetInstance()->municipality_population_;
    std::vector<uint32_t> agents_per_municipality;
    auto scaling = GetAgentToPersonRatio();
    std::for_each(
        pop_per_municipality.begin(), pop_per_municipality.end(),
        [&](uint32_t val) {
          agents_per_municipality.push_back(std::round(val / scaling));
        });
    auto diff = std::accumulate(agents_per_municipality.begin(),
                                agents_per_municipality.end(), 0) -
                pop_count;
    // Deduct excess agents from Amsterdam
    agents_per_municipality[15] -= diff;
    if (std::accumulate(agents_per_municipality.begin(),
                        agents_per_municipality.end(), 0) -
            pop_count !=
        0) {
      Log::Fatal(
          "InitializePopulation",
          "agents_per_municipality was not equal to pop_count. Diff = ", diff);
    }
    int i = 0;
    std::vector<uint32_t> cumsum(agents_per_municipality.size());
    std::partial_sum(agents_per_municipality.begin(), agents_per_municipality.end(), cumsum.begin());
    for (auto apm : agents_per_municipality) {
#pragma omp parallel for
      for (size_t idx = 0; idx < apm; idx++) {
        auto offset = i == 0 ? 0 : cumsum[i - 1];
        AgentUid uid(offset + idx);
        bdm_static_cast<Person*>(rm->GetAgent(uid))->location_ = i;
        bdm_static_cast<Person*>(rm->GetAgent(uid))->home_location_ = i;
      }
      i++;
    }
  }

  auto mobility_data = MobilityData::GetInstance();
  const auto& municipality_codes = mobility_data->municipality_codes_;

  if (!randinit) {
    if (!fs::exists(pop_dir_file)) {
      Log::Fatal("CsvTo2DMatrix", "File not found: ", pop_dir_file);
    }
    rapidcsv::ConverterParams converter(true);
    rapidcsv::SeparatorParams separator;
    rapidcsv::LabelParams labels(0);  // no header

    std::cout << "Reading in register data..." << std::endl;

    auto doc = rapidcsv::Document(pop_dir_file, labels, separator, converter);

    uint32_t num_agents = 0;
    if (sparam->population_size != 0) {
      num_agents = sparam->population_size;
    } else {
      num_agents = doc.GetRowCount();
    }

    std::cout << "Initializing population of " << num_agents
              << " agents with register data..." << std::endl;

    std::vector<size_t> random_indices(doc.GetRowCount());
    // Generate values in range 0, 1, 2, .., (doc.GetRowCount() - 1)
    std::iota(std::begin(random_indices), std::end(random_indices), 0);
    // Shuffle to create random indices order
    auto rng = std::default_random_engine{};
    std::shuffle(random_indices.begin(), random_indices.end(), rng);

#pragma omp parallel
    {
      auto* ctxt = sim->GetExecutionContext();
#pragma omp for
      for (size_t r = 0; r < num_agents; r++) {
        auto idx = random_indices[r];
        std::vector<int> row = doc.GetRow<int>(idx);
        auto municipality = row[2];
        auto workstatus = row[4];
        auto age = row[9];
        auto gender = row[8];
        Demographic d = WorkstatusToDemographic(workstatus, age);
        uint32_t location =
            MunicipalityToLocation(municipality, municipality_codes);
        Gender g = Gender(gender - 1);
        auto* new_person = CreatePerson(g, age, d, location);
        ctxt->AddAgent(new_person);
      }
    }

    // Adds agents to ResourceManager
    sim->GetScheduler()->FinalizeInitialization();
  }

  std::cout << "num agents = " << rm->GetNumAgents() << std::endl;
  std::cout << "1 agent = " << GetAgentToPersonRatio() << " persons"
            << std::endl;

//   real_t agent_to_person_ratio = GetAgentToPersonRatio();
//   real_t initial_infection_scaling = sparam->initial_infection_scaling;
//   real_t initial_exposed_infected_ratio =
//       sparam->initial_exposed_infected_ratio;
//   std::vector<int> initial_infected;
//   std::string data_dir = GetDataDir();
//   std::string init_infected_file =
//       data_dir + "/initial_infected_per_municipality.csv";

//   CsvToVector<int>(init_infected_file, &initial_infected, 1, 0);
//   assert(initial_infected.size() == kNumMunicipalities);

//   // Reorder agent order so that ForEachAgent randomly iterates through agents
//   rm->RandomizeAgentsOrder();

//   // Initialize initial infected people
// #pragma omp parallel for
//   for (size_t municipality = 0; municipality < kNumMunicipalities;
//        municipality++) {
//     int infection_count = 0;
//     auto is_in_municipality = L2F([&](Agent* a) {
//       return bdm_static_cast<Person*>(a)->home_location_ == municipality;
//     });
//     rm->ForEachAgent(
//         [&](Agent* a) {  // NOLINT
//           auto* person = bdm_static_cast<Person*>(a);
//           if (infection_count <
//               static_cast<int>(
//                   std::round(initial_infected[municipality] /
//                              (initial_infection_scaling * agent_to_person_ratio)))) {
//             person->state_ = State::kInfectious;
//             person->RandomlyInitializeStateThreshold();
//             infection_count++;
//           }
//         },
//         &is_in_municipality);
//   }

  // Initialize initial exposed people
  // Assumption: linear relation between infection and exposure cases: E = initial_exposed_infected_ratio * I
// #pragma omp parallel for
//   for (size_t municipality = 0; municipality < kNumMunicipalities;
//        municipality++) {
//     int exposed_count = 0;
//     auto is_in_municipality = L2F([&](Agent* a) {
//       return bdm_static_cast<Person*>(a)->home_location_ == municipality;
//     });
//     rm->ForEachAgent(
//         [&](Agent* a) {  // NOLINT
//           auto* person = bdm_static_cast<Person*>(a);
//           if (exposed_count <
//                   initial_exposed_infected_ratio *
//                       static_cast<int>(std::round(
//                           initial_infected[municipality] /
//                           (initial_infection_scaling * agent_to_person_ratio))) &&
//               person->state_ != kInfectious) {
//             person->state_ = State::kExposed;
//             person->RandomlyInitializeStateThreshold();
//             exposed_count++;
//           }
//         },
//         &is_in_municipality);
//   }

  // Print average percentage of persons not staying at home during the day
  std::array<int, kHoursPerDay> people_not_home{};
  rm->ForEachAgent([&](Agent* a) {  // NOLINT
    auto* person = bdm_static_cast<Person*>(a);
    const auto& weekly_schedule = person->weekly_travel_schedule_;
    // Just consider the first day of the week
    for (size_t h = 0; h < kHoursPerDay; h++) {
      if (weekly_schedule[h] != person->home_location_) {
        people_not_home[h]++;
      }
    }
  });

  for (auto& el : people_not_home) {
    std::cout << static_cast<real_t>(el) / rm->GetNumAgents() << " ";
  }
  std::cout << std::endl;
}

}  // namespace bdm

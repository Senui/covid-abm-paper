#ifndef INITIALIZATION_H_
#define INITIALIZATION_H_

#include <algorithm>
#include <array>
#include <numeric>
#include <utility>
#include <vector>

#include "behaviors/change_situation_behavior.h"
#include "behaviors/infection_behavior.h"
#include "behaviors/travel_behavior.h"
#include "mobility_data.h"
#include "model_facts.h"
#include "operations/update_statistics_op.h"
#include "person.h"
#include "sim_param.h"

#include "biodynamo.h"
#include "core/randomized_rm.h"
#include "core/util/csv_reader.h"

namespace bdm {

static Random rand;

void InitializeMobilityData();

void InitializeWeeklyTravelSchedule(Person* person);

// Determine the demographic group based on the workstatus of a person
Demographic WorkstatusToDemographic(int workstatus, uint8_t age);

uint32_t MunicipalityToLocation(
    uint32_t municipality, const std::vector<uint32_t>& municipality_codes);

Person* CreatePerson(Gender gender, uint8_t age, Demographic d,
                            uint16_t municipality);

void InitializePopulation(std::string pop_dir_file, bool randinit = 0);

}  // namespace bdm

#endif  // INITIALIZATION_H_

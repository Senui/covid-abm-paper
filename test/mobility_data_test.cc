#include <gtest/gtest.h>
#include "biodynamo.h"

#include "initialization.h"
#include "mobility_data.h"
#include "person.h"

#define TEST_NAME typeid(*this).name()

namespace bdm {

TEST(MobilityData, DrawDirichlet) {
  Simulation simulation(TEST_NAME);
  InitializeMobilityData();
  Person person;

  std::vector<uint16_t> other_locations(10);
  auto mobility_data = MobilityData::GetInstance();
  mobility_data->DrawDirichlet(&person, &other_locations);
  for (const auto& loc : other_locations) {
    EXPECT_NE(person.GetHomeLocation(), loc);
  }
}

}  // namespace bdm

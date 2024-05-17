#include <gtest/gtest.h>
#include "biodynamo.h"

#include "initialization.h"
#include "person.h"

#define TEST_NAME typeid(*this).name()

namespace bdm {

TEST(Initialization, InitializeWeeklyTravelSchedule) {
  Param::RegisterParamGroup(new SimParam());
  Simulation simulation(TEST_NAME);
  InitializeMobilityData();
  Person person;

  InitializeWeeklyTravelSchedule(&person);
  auto* schedule = person.GetWeeklyTravelSchedule();

  EXPECT_EQ(static_cast<size_t>(kHoursPerDay * kDaysPerWeek), schedule->size());

  size_t idx = 0;
  for (size_t day = 0; day < kDaysPerWeek; day++) {
    for (size_t hour = 0; hour < kHoursPerDay; hour++) {
      if (hour < 2 || hour > 22) {  // These hours a person is home
        EXPECT_EQ(person.GetHomeLocation(), (*schedule)[idx]);
      }
      idx++;
    }
  }
}

TEST(Initialization, WorkstatusToDemographic) {
  EXPECT_EQ(kPreSchoolChildren, WorkstatusToDemographic(0, 0));
  EXPECT_EQ(kPreSchoolChildren, WorkstatusToDemographic(26, 4));
  EXPECT_EQ(kPrimarySchoolChildren, WorkstatusToDemographic(0, 5));
  EXPECT_EQ(kPrimarySchoolChildren, WorkstatusToDemographic(24, 11));
  EXPECT_EQ(kSecondarySchoolChildren, WorkstatusToDemographic(0, 12));
  EXPECT_EQ(kSecondarySchoolChildren, WorkstatusToDemographic(24, 16));
  EXPECT_EQ(kStudents, WorkstatusToDemographic(26, 17));
  EXPECT_EQ(kStudents, WorkstatusToDemographic(26, 24));
  EXPECT_EQ(kNonStudyingAdolescents, WorkstatusToDemographic(54, 17));
  EXPECT_EQ(kNonStudyingAdolescents, WorkstatusToDemographic(11, 20));
  EXPECT_EQ(kMiddleAgeWorking, WorkstatusToDemographic(11, 25));
  EXPECT_EQ(kMiddleAgeWorking, WorkstatusToDemographic(15, 36));
  EXPECT_EQ(kMiddleAgeUnemployed, WorkstatusToDemographic(10, 35));
  EXPECT_EQ(kMiddleAgeUnemployed, WorkstatusToDemographic(26, 54));
  EXPECT_EQ(kHigherAgeWorking, WorkstatusToDemographic(11, 55));
  EXPECT_EQ(kHigherAgeWorking, WorkstatusToDemographic(15, 67));
  EXPECT_EQ(kHigherAgeUnemployed, WorkstatusToDemographic(10, 58));
  EXPECT_EQ(kHigherAgeUnemployed, WorkstatusToDemographic(16, 62));
  EXPECT_EQ(kElderly, WorkstatusToDemographic(16, 68));
  EXPECT_EQ(kElderly, WorkstatusToDemographic(16, 80));
  EXPECT_EQ(kEldest, WorkstatusToDemographic(16, 81));
  EXPECT_EQ(kEldest, WorkstatusToDemographic(16, 100));
}

TEST(Initialization, MunicipalityToLocation) {
}

}  // namespace bdm

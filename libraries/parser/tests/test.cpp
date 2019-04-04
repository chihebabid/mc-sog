#define CATCH_CONFIG_MAIN
#include "Net.hpp"
#include "catch.hpp"

TEST_CASE("Parsing simple file", "[Simple]") {
  net model("ring2.net", "Obs2_ring");
  REQUIRE(model.nbPlace() == 16);
  REQUIRE(model.nbTransition() == 16);
}

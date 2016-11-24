/// Causal Dynamical Triangulations in C++ using CGAL
///
/// Copyright (c) 2016 Adam Getchell
///
/// Full run-through with default options used to calculate
/// optimal values for thermalization, etc. A simpler version
/// that encompasses the entire lifecycle. Also suitable for
/// scripting parallel runs.
///
/// \done Invoke Metropolis algorithm
/// \todo Print out graph of time-value vs. volume vs. pass number

/// @file cdt-opt.cpp
/// @brief Outputs values to determine optimizations
/// @author Adam Getchell

#include <utility>
#include "Measurements.h"
#include "Metropolis.h"
#include "Simulation.h"

int main() {
  std::cout << "cdt-opt running ..." << std::endl;
  constexpr unsigned long long simplices  = 64000;  // NOLINT
  constexpr unsigned long long timeslices = 25;     // NOLINT
  constexpr long double        alpha      = 1.1;
  constexpr long double        k          = 2.2;
  constexpr long double        lambda     = 3.3;
  constexpr unsigned long long passes     = 1000;  // NOLINT
  constexpr unsigned long long checkpoint = 10;    // NOLINT

  // Initialize simulation
  Simulation my_simulation;

  // Initialize the Metropolis algorithm
  Metropolis my_algorithm(alpha, k, lambda, passes, checkpoint);

  // Make a triangulation
  SimplicialManifold universe(simplices, timeslices);

  // Queue up simulation with desired algorithm
  my_simulation.queue(
      [&my_algorithm](SimplicialManifold s) { return my_algorithm(s); });
  // Measure results
  my_simulation.queue(
      [](SimplicialManifold s) { return VolumePerTimeslice(s); });
  // my_simulation.queue(print_results())

  // Run it
  universe = my_simulation.start(std::forward<SimplicialManifold>(universe));

  return 0;
}

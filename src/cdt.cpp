/// Causal Dynamical Triangulations in C++ using CGAL
///
/// Copyright © 2014-2019 Adam Getchell
///
/// A program that generates spacetimes
///
/// Inspired by https://github.com/ucdavis/CDT
///
/// @todo Invoke complete set of ergodic (Pachner) moves
/// @todo Fix write_file() to include cell->info() and vertex->info()

/// @file cdt.cpp
/// @brief The main executable
/// @author Adam Getchell

// CGAL headers
#include <CGAL/Real_timer.h>

// C++ headers
#include <gsl/gsl>
#include <vector>

// Docopt
#include <docopt.h>

// CDT headers
#include <Metropolis.hpp>
#include <Simulation.hpp>

using Timer = CGAL::Real_timer;

using namespace std;

/// Help message parsed by docopt into options
static const char USAGE[]{
    R"(Causal Dynamical Triangulations in C++ using CGAL.

Copyright (c) 2014-2018 Adam Getchell

A program that generates d-dimensional triangulated spacetimes
with a defined causal structure and evolves them according
to the Metropolis algorithm. Specify the number of passes to control
how much evolution is desired. Each pass attempts a number of ergodic
moves equal to the number of simplices in the simulation.

Usage:./cdt (--spherical | --toroidal) -n SIMPLICES -t TIMESLICES [-d DIM] -k K --alpha ALPHA --lambda LAMBDA [-p PASSES] [-c CHECKPOINT]

Examples:
./cdt --spherical -n 32000 -t 11 --alpha 0.6 -k 1.1 --lambda 0.1 --passes 1000
./cdt --s -n32000 -t11 -a.6 -k1.1 -l.1 -p1000

Options:
  -h --help                   Show this message
  --version                   Show program version
  -n SIMPLICES                Approximate number of simplices
  -t TIMESLICES               Number of timeslices
  -d DIM                      Dimensionality [default: 3]
  -a --alpha ALPHA            Negative squared geodesic length of 1-d
                              timelike edges
  -k K                        K = 1/(8*pi*G_newton)
  -l --lambda LAMBDA          K * Cosmological constant
  -p --passes PASSES          Number of passes [default: 100]
  -c --checkpoint CHECKPOINT  Checkpoint every n passes [default: 10]
)"};

/// @brief The main path of the CDT++ program
///
/// @param[in,out]  argc  Argument count = 1 + number of arguments
/// @param[in,out]  argv  Argument vector (array) to be passed to docopt
/// @returns        Integer value 0 if successful, 1 on failure
int main(int argc, char* const argv[]) try
{
  // https://stackoverflow.com/questions/9371238/why-is-reading-lines-from-stdin-much-slower-in-c-than-python?rq=1
  ios_base::sync_with_stdio(false);

  // Start running time
  Timer t;
  t.start();

  // docopt option parser
  gsl::cstring_span<>        usage_string = gsl::ensure_z(USAGE);
  map<string, docopt::value> args =
      docopt::docopt(gsl::to_string(usage_string), {argv + 1, argv + argc},
                     true,          // print help message automatically
                     "CDT 0.1.8");  // Version

  // Debugging
  // for (auto const& arg : args) {
  //   std::cout << arg.first << " " << arg.second << "\n";
  // }

  // Parse docopt::values in args map
  auto simplices  = stoi(args["-n"].asString());
  auto timeslices = stoi(args["-t"].asString());
  auto dimensions = stoi(args["-d"].asString());
  auto alpha      = stold(args["--alpha"].asString());
  auto k          = stold(args["-k"].asString());
  auto lambda     = stold(args["--lambda"].asString());
  auto passes     = stoi(args["--passes"].asString());
  auto checkpoint = stoi(args["--checkpoint"].asString());

  // Topology of simulation
  topology_type topology;
  if (args["--spherical"].asBool()) { topology = topology_type::SPHERICAL; }
  else
  {
    topology = topology_type::TOROIDAL;
  }

  // Display job parameters
  cout << "Topology is " << topology << "\n";
  cout << "Number of dimensions = " << dimensions << "\n";
  cout << "Number of simplices = " << simplices << "\n";
  cout << "Number of timeslices = " << timeslices << "\n";
  cout << "Alpha = " << alpha << "\n";
  cout << "K = " << k << "\n";
  cout << "Lambda = " << lambda << "\n";
  cout << "Number of passes = " << passes << "\n";
  cout << "Checkpoint every n passes = " << checkpoint << "\n";
  cout << "User = " << getEnvVar("USER") << "\n";
  cout << "Hostname = " << hostname() << "\n";

  if (simplices < 2 || timeslices < 2)
  {
    t.stop();
    throw invalid_argument(
        "Simplices and timeslices should be greater or equal to 2.");
  }

  // Initialize simulation
  Simulation my_simulation;

  // Initialize the Metropolis algorithm
  // \todo: add strong exception-safety guarantee on Metropolis functor
  Metropolis my_algorithm(alpha, k, lambda, passes, checkpoint);

  // Initialize triangulation
  SimplicialManifold universe;

  // Queue up simulation with desired algorithm
  my_simulation.queue(
      [&my_algorithm](SimplicialManifold s) { return my_algorithm(s); });

  // Measure results
  my_simulation.queue(
      [](SimplicialManifold s) { return VolumePerTimeslice(s); });

  // Ensure Triangle inequalities hold
  // See http://arxiv.org/abs/hep-th/0105267 for details
  if (dimensions == 3 && abs(alpha) < 0.5)
  {
    t.stop();  // End running time counter
    throw domain_error("Alpha in 3D should be greater than 1/2.");
  }

  switch (topology)
  {
    case topology_type::SPHERICAL:
      if (dimensions == 3)
      {
        SimplicialManifold populated_universe(simplices, timeslices);
        // SimplicialManifold swapperator for no-throw
        swap(universe, populated_universe);
      }
      else
      {
        t.stop();  // End running time counter
        throw invalid_argument("Currently, dimensions cannot be >3.");
      }
      break;
    case topology_type::TOROIDAL:
      t.stop();  // End running time counter
      throw invalid_argument("Toroidal triangulations not yet supported.");
    default:
      throw logic_error("Simulation topology not parsed.");
  }

  if (!fix_timeslices(universe.triangulation))
  {
    t.stop();  // End running time counter
    throw logic_error("Delaunay triangulation not correctly foliated.");
  }

  cout << "Universe has been initialized ...\n";
  cout << "Now performing " << passes << " passes of ergodic moves.\n";

  // The main work of the program
  universe = my_simulation.start(move(universe));

  // Output results
  t.stop();  // End running time counter
  cout << "Final Delaunay triangulation has ";
  print_results(universe, t);

  // Write results to file
  // Strong exception-safety guarantee
  // \todo: Fixup so that cell->info() and vertex->info() values
  //                   are written
  write_file(universe, topology, dimensions,
             universe.triangulation->number_of_finite_cells(), timeslices);

  return 0;
}
catch (domain_error& DomainError)
{
  cerr << DomainError.what() << "\n";
  cerr << "Triangle inequalities violated ... Exiting.\n";
  return 1;
}
catch (invalid_argument& InvalidArgument)
{
  cerr << InvalidArgument.what() << "\n";
  cerr << "Invalid parameter ... Exiting.\n";
  return 1;
}
catch (logic_error& LogicError)
{
  cerr << LogicError.what() << "\n";
  cerr << "Simulation startup failed ... Exiting.\n";
  return 1;
}
catch (...)
{
  cerr << "Something went wrong ... Exiting.\n";
  return 1;
}

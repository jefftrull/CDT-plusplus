cmake_minimum_required(VERSION 3.12...3.17)

add_executable(
  CDT_test
  ${PROJECT_SOURCE_DIR}/tests/main.cpp
  Apply_move_test.cpp
  Ergodic_moves_3_test.cpp
  Foliated_triangulation_test.cpp
  Geometry_test.cpp
  Manifold_test.cpp
  Move_command_test.cpp
  # MoveManagerTest.cpp
  Move_strategies_test.cpp
  S3Action_test.cpp
  Sphere_test.cpp
  Tetrahedron_test.cpp
  Torus_test.cpp
  Utilities_test.cpp
  Vertex_test.cpp
    Settings_test.cpp)

target_link_libraries(CDT_test project_options project_warnings Catch2::Catch2
                      fmt::fmt-header-only CGAL::CGAL)

# Activate C++17
target_compile_features(CDT_test PRIVATE cxx_std_17)

# Run unit tests
add_test(NAME CDT-unit-tests COMMAND $<TARGET_FILE:CDT_test>)

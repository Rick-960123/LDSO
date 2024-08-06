# TUM_MONO dataset
add_executable( run_dso_tum_mono run_dso_tum_mono.cc )
target_link_libraries( run_dso_tum_mono
  ldso ${THIRD_PARTY_LIBS} )

# EUROC dataset
add_executable( run_dso_euroc run_dso_euroc.cc )
target_link_libraries( run_dso_euroc
  ldso ${THIRD_PARTY_LIBS} )

# Kitti dataset
add_executable( run_dso_kitti run_dso_kitti.cc )
target_link_libraries( run_dso_kitti
  ldso ${THIRD_PARTY_LIBS} )

FIND_PACKAGE(TBB REQUIRED)
FIND_PACKAGE(OpenMP REQUIRED)
set(CMAKE_CXX_STANDARD 17)
add_executable( run_test test.cpp )
target_link_libraries( run_test
OpenMP::OpenMP_CXX
TBB::tbb )
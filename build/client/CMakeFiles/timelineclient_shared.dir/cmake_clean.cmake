file(REMOVE_RECURSE
  "libtimelineclient.pdb"
  "libtimelineclient.so"
)

# Per-language clean rules from dependency scanning.
foreach(lang CXX)
  include(CMakeFiles/timelineclient_shared.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()

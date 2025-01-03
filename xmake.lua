add_rules("mode.debug", "mode.release")

add_requires("zlib")
add_requires("python 3.x")

target("_fastseqio")
  add_rules("python.library")
  add_files("seqio.c")
  add_files("python/fastseqio.cc", {moduletype = "python"})
  add_includedirs(".", "python/pybind11/include")
  add_packages("zlib")
  add_packages("python")

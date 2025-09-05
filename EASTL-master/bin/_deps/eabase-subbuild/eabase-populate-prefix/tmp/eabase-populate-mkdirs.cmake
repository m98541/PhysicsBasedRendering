# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "E:/directx_practice/d3d11_setup/EASTL-master/bin/_deps/eabase-src")
  file(MAKE_DIRECTORY "E:/directx_practice/d3d11_setup/EASTL-master/bin/_deps/eabase-src")
endif()
file(MAKE_DIRECTORY
  "E:/directx_practice/d3d11_setup/EASTL-master/bin/_deps/eabase-build"
  "E:/directx_practice/d3d11_setup/EASTL-master/bin/_deps/eabase-subbuild/eabase-populate-prefix"
  "E:/directx_practice/d3d11_setup/EASTL-master/bin/_deps/eabase-subbuild/eabase-populate-prefix/tmp"
  "E:/directx_practice/d3d11_setup/EASTL-master/bin/_deps/eabase-subbuild/eabase-populate-prefix/src/eabase-populate-stamp"
  "E:/directx_practice/d3d11_setup/EASTL-master/bin/_deps/eabase-subbuild/eabase-populate-prefix/src"
  "E:/directx_practice/d3d11_setup/EASTL-master/bin/_deps/eabase-subbuild/eabase-populate-prefix/src/eabase-populate-stamp"
)

set(configSubDirs Debug)
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "E:/directx_practice/d3d11_setup/EASTL-master/bin/_deps/eabase-subbuild/eabase-populate-prefix/src/eabase-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "E:/directx_practice/d3d11_setup/EASTL-master/bin/_deps/eabase-subbuild/eabase-populate-prefix/src/eabase-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()

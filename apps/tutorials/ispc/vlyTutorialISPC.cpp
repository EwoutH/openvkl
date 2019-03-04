// ======================================================================== //
// Copyright 2018 Intel Corporation                                         //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include <iostream>
#include <vector>
#include <volley/volley.h>

#include "vlyTutorialISPC_ispc.h"

int main()
{
  vlyLoadModule("scalar_driver");

  VLYDriver driver = vlyNewDriver("scalar_driver");
  vlyCommitDriver(driver);
  vlySetCurrentDriver(driver);

  VLYVolume volume = vlyNewVolume("wavelet_analytical_volume");
  vlyCommit(volume);

  vly_vec3f objectCoordinates{0.f, 1.f, 2.f};

  float sample = vlyComputeSample(volume, &objectCoordinates);

  std::cout << sample << std::endl;

  ispc::sample_ispc((ispc::VolleyVolume *)volume);

  return 0;
}

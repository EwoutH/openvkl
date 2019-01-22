// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

#include "VolleyVolumeWrapper.h"
#include "../common/Data.h"

namespace ospray {
  namespace scalar_volley_device {

    void VolleyVolumeWrapper::commit()
    {
      Volume::commit();

      static bool volleyInitialized = false;

      if (!volleyInitialized) {
        std::cout << "initializing Volley" << std::endl;

        vlyLoadModule("scalar_driver");

        VLYDriver driver = vlyNewDriver("scalar_driver");
        vlyCommitDriver(driver);
        vlySetCurrentDriver(driver);

        volleyInitialized = true;
      }

      if (!vlyVolume) {
        vlyVolume = vlyNewVolume("simple_procedural_volume");
      }

      vlySamplingType =
          VLYSamplingType(getParam<int>("vlySamplingType", VLY_SAMPLE_LINEAR));

      // TODO: pass through any values that need to be committed to the Volley
      // volume

      vlyCommit(vlyVolume);
    }

    bool VolleyVolumeWrapper::intersect(Ray &ray) const
    {
      vly_range1f range;

      vlyIntersectVolume(vlyVolume,
                         1,
                         (const vly_vec3f *)&ray.org,
                         (const vly_vec3f *)&ray.dir,
                         &range);

      if (range.lower < range.upper) {  // should be nan check
        ray.t0 = range.lower;
        ray.t  = range.upper;
        return true;
      } else {
        return false;
      }
    }

    float VolleyVolumeWrapper::computeSample(
        const vec3f &worldCoordinates) const
    {
      float sample;

      vlySampleVolume(vlyVolume,
                      vlySamplingType,
                      1,
                      (vly_vec3f *)&worldCoordinates,
                      &sample);

      return sample;
    }

    std::vector<float> VolleyVolumeWrapper::computeSamples(
        const std::vector<vec3f> &worldCoordinates) const
    {
      std::vector<float> samples;
      samples.resize(worldCoordinates.size());

      vlySampleVolume(vlyVolume,
                      vlySamplingType,
                      worldCoordinates.size(),
                      (vly_vec3f *)worldCoordinates.data(),
                      (float *)samples.data());

      return samples;
    }

    void VolleyVolumeWrapper::advance(Ray &ray) const
    {
      vlyAdvanceRays(vlyVolume,
                     samplingRate,
                     1,
                     (const vly_vec3f *)&ray.org,
                     (const vly_vec3f *)&ray.dir,
                     &ray.t0);
    }

    int VolleyVolumeWrapper::setRegion(const void *,
                                       const vec3i &,
                                       const vec3i &)
    {
      // no-op
      return 0;
    }

    VLYVolume VolleyVolumeWrapper::getVLYVolume()
    {
      return vlyVolume;
    }

    VLYSamplingType VolleyVolumeWrapper::getVLYSamplingType()
    {
      return vlySamplingType;
    }

  }  // namespace scalar_volley_device
}  // namespace ospray

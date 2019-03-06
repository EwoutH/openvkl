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

#include "VolumeRendererRayIterator.h"
#include "../volume/VolleyVolumeWrapper.h"

// ospcommon
#include <ospray/ospcommon/tasking/parallel_for.h>

namespace ospray {
  namespace scalar_volley_device {

    void VolumeRendererRayIterator::renderTile(Tile &tile)
    {
      VolleyVolumeWrapper *volleyVolumeWrapper =
          dynamic_cast<VolleyVolumeWrapper *>(volume);

      if (!volleyVolumeWrapper) {
        throw std::runtime_error(
            "only Volley-based volumes supported in this renderer");
      }

      VLYVolume vlyVolume = volleyVolumeWrapper->getVLYVolume();

      auto &transferFunction = volume->getTransferFunction();

      tasking::parallel_for(tile.size.y, [&](int y) {
        for (int x = 0; x < tile.size.x; x++) {
          // generate camera sample in [0-1] screen space
          CameraSample cameraSample{vec2f{
              (tile.origin.x + x) * rcp(float(currentFrameBuffer->size().x)),
              (tile.origin.y + y) * rcp(float(currentFrameBuffer->size().y))}};

          // generate ray from camera sample
          Ray ray;
          currentCamera->getRay(cameraSample, ray);

          // the integrated volume value
          vec4f color{0.f, 0.f, 0.f, 0.f};

          // create volume ray iterator
          vly_range1f tRange{ray.t0, ray.t};
          VLYSamplesMask samplesMask;

          VLYRayIterator rayIterator = vlyNewRayIterator(vlyVolume,
                                                         (vly_vec3f *)&ray.org,
                                                         (vly_vec3f *)&ray.dir,
                                                         &tRange,
                                                         samplesMask);

          // the current ray interval
          VLYRayInterval rayInterval;

          while (vlyIterateInterval(rayIterator, &rayInterval) &&
                 color.w < 0.99f) {
            const float nominalSamplingDt =
                rayInterval.nominalDeltaT / volume->getSamplingRate();

            // initial sub interval, based on our renderer-defined sampling rate
            // and the volume's nominal dt
            range1f subInterval(
                rayInterval.tRange.lower,
                std::min(rayInterval.tRange.lower + nominalSamplingDt,
                         rayInterval.tRange.upper));

            // integrate as long as we have valid sub intervals and are not
            // fully opaque
            while (subInterval.size() > 0.f && color.w < 0.99f) {
              const float t  = subInterval.center();
              const float dt = subInterval.size();

              // get volume sample
              const float sample = volume->computeSample(ray.org + t * ray.dir);

              // apply transfer function
              vec4f sampleColor = transferFunction.getColorAndOpacity(sample);

              // accumulate contribution
              // TODO: eliminate magic scaling; this is due to units of transfer
              // function now being "per unit world length"
              const float clampedOpacity = clamp(10.f * sampleColor.w * dt);

              sampleColor *= clampedOpacity;
              sampleColor.w = clampedOpacity;

              color += (1.f - color.w) * sampleColor;

              // compute next sub interval
              subInterval.lower = subInterval.upper;
              subInterval.upper =
                  std::min(subInterval.lower + nominalSamplingDt,
                           rayInterval.tRange.upper);
            }
          }

          tile.colorBuffer[tile.indexOf(vec2i{x, y})] = color;

          vlyRelease(rayIterator);
        }
      });
    }

  }  // namespace scalar_volley_device
}  // namespace ospray
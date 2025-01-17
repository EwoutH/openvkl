// ======================================================================== //
// Copyright 2019 Intel Corporation                                         //
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

#include <array>
#include "../../external/catch.hpp"
#include "aos_soa_conversion.h"
#include "iterator_utility.h"
#include "openvkl_testing.h"
#include "ospcommon/utility/multidim_index_sequence.h"

using namespace ospcommon;
using namespace openvkl::testing;

TEST_CASE("Vectorized interval iterator", "[interval_iterators]")
{
  vklLoadModule("ispc_driver");

  VKLDriver driver = vklNewDriver("ispc");
  vklCommitDriver(driver);
  vklSetCurrentDriver(driver);

  int nativeSIMDWidth = vklGetNativeSIMDWidth();

  WARN(
      "only performing SIMD vectorized interval iteration tests for width == "
      "native width: "
      << nativeSIMDWidth);

  // for a unit cube physical grid [(0,0,0), (1,1,1)]
  constexpr int DIMENSION      = 128;
  constexpr int MACROCELL_SIZE = 16;

  const vec3i dimensions(DIMENSION);
  const vec3f gridOrigin(0.f);
  const vec3f gridSpacing(1.f / (128.f - 1.f));

  std::unique_ptr<WaveletProceduralVolume> v(
      new WaveletProceduralVolume(dimensions, gridOrigin, gridSpacing));

  VKLVolume vklVolume = v->getVKLVolume();

  vkl_box3f bbox = vklGetBoundingBox(vklVolume);

  std::random_device rd;
  std::mt19937 eng(rd());

  std::uniform_real_distribution<float> distX(bbox.lower.x, bbox.upper.x);
  std::uniform_real_distribution<float> distY(bbox.lower.y, bbox.upper.y);

  const int maxWidth = 16;

  std::array<int, 3> nativeWidths{4, 8, 16};

  SECTION("randomized interval continuity with no value selector")
  {
    for (int width = 1; width < maxWidth; width++) {
      std::vector<vec3f> origins(width);
      std::vector<vec3f> directions(width);
      std::vector<vkl_range1f> tRanges(width);

      for (int i = 0; i < width; i++) {
        origins[i]    = vec3f(distX(eng), distY(eng), -1.f);
        directions[i] = vec3f(0.f, 0.f, 1.f);
        tRanges[i]    = vkl_range1f{0.f, inf};
      }

      for (auto callingWidth : nativeWidths) {
        if (width > callingWidth || callingWidth != nativeSIMDWidth) {
          continue;
        }

        std::vector<int> valid(callingWidth, 0);
        std::fill(valid.begin(), valid.begin() + width, 1);

        std::vector<float> originsSOA = AOStoSOA_vec3f(origins, callingWidth);
        std::vector<float> directionsSOA =
            AOStoSOA_vec3f(directions, callingWidth);
        std::vector<float> tRangesSOA = AOStoSOA_range1f(tRanges, callingWidth);

        if (callingWidth == 4) {
          VKLIntervalIterator4 iterator;
          vklInitIntervalIterator4(valid.data(),
                                   &iterator,
                                   vklVolume,
                                   (const vkl_vvec3f4 *)originsSOA.data(),
                                   (const vkl_vvec3f4 *)directionsSOA.data(),
                                   (const vkl_vrange1f4 *)tRangesSOA.data(),
                                   nullptr);

          VKLInterval4 intervalPrevious, intervalCurrent;
          int result[4];

          int counter = 0;

          while (true) {
            vklIterateInterval4(
                valid.data(), &iterator, &intervalCurrent, result);

            int resultSum = 0;

            for (int i = 0; i < width; i++) {
              resultSum += result[i];
            }

            // we should have the same result for all active lanes
            REQUIRE((resultSum == width || resultSum == 0));

            if (!result[0]) {
              break;
            }

            for (int i = 0; i < width; i++) {
              INFO("interval iteration "
                   << counter << " lane[" << i
                   << "] tRange = " << intervalCurrent.tRange.lower[i] << ", "
                   << intervalCurrent.tRange.upper[i]
                   << " nominalDeltaT = " << intervalCurrent.nominalDeltaT[i]);

              if (counter == 0) {
                // first interval at expected beginning
                REQUIRE(intervalCurrent.tRange.lower[i] == 1.f);
              } else {
                // interval continuity
                REQUIRE(intervalCurrent.tRange.lower[i] ==
                        intervalPrevious.tRange.upper[i]);
              }
            }

            intervalPrevious = intervalCurrent;

            counter++;
          }

          // expect a fixed number of intervals to be returned based on
          // structured volume acceleration structure
          REQUIRE(counter == DIMENSION / MACROCELL_SIZE);

          // last interval at expected ending
          for (int i = 0; i < width; i++) {
            REQUIRE(intervalPrevious.tRange.upper[i] == 2.f);
          }
        }

        else if (callingWidth == 8) {
          VKLIntervalIterator8 iterator;
          vklInitIntervalIterator8(valid.data(),
                                   &iterator,
                                   vklVolume,
                                   (const vkl_vvec3f8 *)originsSOA.data(),
                                   (const vkl_vvec3f8 *)directionsSOA.data(),
                                   (const vkl_vrange1f8 *)tRangesSOA.data(),
                                   nullptr);

          VKLInterval8 intervalPrevious, intervalCurrent;
          int result[8];

          int counter = 0;

          while (true) {
            vklIterateInterval8(
                valid.data(), &iterator, &intervalCurrent, result);

            int resultSum = 0;

            for (int i = 0; i < width; i++) {
              resultSum += result[i];
            }

            // we should have the same result for all active lanes
            REQUIRE((resultSum == width || resultSum == 0));

            if (!result[0]) {
              break;
            }

            for (int i = 0; i < width; i++) {
              INFO("interval iteration "
                   << counter << " lane[" << i
                   << "] tRange = " << intervalCurrent.tRange.lower[i] << ", "
                   << intervalCurrent.tRange.upper[i]
                   << " nominalDeltaT = " << intervalCurrent.nominalDeltaT[i]);

              if (counter == 0) {
                // first interval at expected beginning
                REQUIRE(intervalCurrent.tRange.lower[i] == 1.f);
              } else {
                // interval continuity
                REQUIRE(intervalCurrent.tRange.lower[i] ==
                        intervalPrevious.tRange.upper[i]);
              }
            }

            intervalPrevious = intervalCurrent;

            counter++;
          }

          // expect a fixed number of intervals to be returned based on
          // structured volume acceleration structure
          REQUIRE(counter == DIMENSION / MACROCELL_SIZE);

          // last interval at expected ending
          for (int i = 0; i < width; i++) {
            REQUIRE(intervalPrevious.tRange.upper[i] == 2.f);
          }
        }

        else if (callingWidth == 16) {
          VKLIntervalIterator16 iterator;
          vklInitIntervalIterator16(valid.data(),
                                    &iterator,
                                    vklVolume,
                                    (const vkl_vvec3f16 *)originsSOA.data(),
                                    (const vkl_vvec3f16 *)directionsSOA.data(),
                                    (const vkl_vrange1f16 *)tRangesSOA.data(),
                                    nullptr);

          VKLInterval16 intervalPrevious, intervalCurrent;
          int result[16];

          int counter = 0;

          while (true) {
            vklIterateInterval16(
                valid.data(), &iterator, &intervalCurrent, result);

            int resultSum = 0;

            for (int i = 0; i < width; i++) {
              resultSum += result[i];
            }

            // we should have the same result for all active lanes
            REQUIRE((resultSum == width || resultSum == 0));

            if (!result[0]) {
              break;
            }

            for (int i = 0; i < width; i++) {
              INFO("interval iteration "
                   << counter << " lane[" << i
                   << "] tRange = " << intervalCurrent.tRange.lower[i] << ", "
                   << intervalCurrent.tRange.upper[i]
                   << " nominalDeltaT = " << intervalCurrent.nominalDeltaT[i]);

              if (counter == 0) {
                // first interval at expected beginning
                REQUIRE(intervalCurrent.tRange.lower[i] == 1.f);
              } else {
                // interval continuity
                REQUIRE(intervalCurrent.tRange.lower[i] ==
                        intervalPrevious.tRange.upper[i]);
              }
            }

            intervalPrevious = intervalCurrent;

            counter++;
          }

          // expect a fixed number of intervals to be returned based on
          // structured volume acceleration structure
          REQUIRE(counter == DIMENSION / MACROCELL_SIZE);

          // last interval at expected ending
          for (int i = 0; i < width; i++) {
            REQUIRE(intervalPrevious.tRange.upper[i] == 2.f);
          }
        }

        else {
          throw std::runtime_error("unsupported calling width");
        }
      }
    }
  }

  SECTION("randomized interval value ranges with no value selector")
  {
    for (int width = 1; width < maxWidth; width++) {
      std::vector<vec3f> origins(width);
      std::vector<vec3f> directions(width);
      std::vector<vkl_range1f> tRanges(width);

      for (int i = 0; i < width; i++) {
        origins[i]    = vec3f(distX(eng), distY(eng), -1.f);
        directions[i] = vec3f(0.f, 0.f, 1.f);
        tRanges[i]    = vkl_range1f{0.f, inf};
      }

      for (auto callingWidth : nativeWidths) {
        if (width > callingWidth || callingWidth != nativeSIMDWidth) {
          continue;
        }

        std::vector<int> valid(callingWidth, 0);
        std::fill(valid.begin(), valid.begin() + width, 1);

        std::vector<float> originsSOA = AOStoSOA_vec3f(origins, callingWidth);
        std::vector<float> directionsSOA =
            AOStoSOA_vec3f(directions, callingWidth);
        std::vector<float> tRangesSOA = AOStoSOA_range1f(tRanges, callingWidth);

        if (callingWidth == 4) {
          VKLIntervalIterator4 iterator;
          vklInitIntervalIterator4(valid.data(),
                                   &iterator,
                                   vklVolume,
                                   (const vkl_vvec3f4 *)originsSOA.data(),
                                   (const vkl_vvec3f4 *)directionsSOA.data(),
                                   (const vkl_vrange1f4 *)tRangesSOA.data(),
                                   nullptr);

          VKLInterval4 interval;
          int result[4];

          int intervalCount = 0;

          while (true) {
            vklIterateInterval4(valid.data(), &iterator, &interval, result);

            int resultSum = 0;

            for (int i = 0; i < width; i++) {
              resultSum += result[i];
            }

            // we should have the same result for all active lanes
            REQUIRE((resultSum == width || resultSum == 0));

            if (!result[0]) {
              break;
            }

            for (int i = 0; i < width; i++) {
              INFO("interval iteration "
                   << intervalCount << " lane[" << i
                   << "] tRange = " << interval.tRange.lower[i] << ", "
                   << interval.tRange.upper[i]
                   << " valueRange = " << interval.valueRange.lower[i] << ", "
                   << interval.valueRange.upper[i]);

              vkl_range1f sampledValueRange = computeIntervalValueRange(
                  vklVolume,
                  reinterpret_cast<vkl_vec3f &>(origins[i]),
                  reinterpret_cast<vkl_vec3f &>(directions[i]),
                  vkl_range1f{interval.tRange.lower[i],
                              interval.tRange.upper[i]});

              INFO("sampled value range = " << sampledValueRange.lower << ", "
                                            << sampledValueRange.upper);

              // the sampled value range should be completely within the
              // returned interval value range
              REQUIRE(
                  (sampledValueRange.lower >= interval.valueRange.lower[i] &&
                   sampledValueRange.upper <= interval.valueRange.upper[i]));
            }

            intervalCount++;
          }

          // make sure we had at least one interval...
          REQUIRE(intervalCount > 0);
        }

        else if (callingWidth == 8) {
          VKLIntervalIterator8 iterator;
          vklInitIntervalIterator8(valid.data(),
                                   &iterator,
                                   vklVolume,
                                   (const vkl_vvec3f8 *)originsSOA.data(),
                                   (const vkl_vvec3f8 *)directionsSOA.data(),
                                   (const vkl_vrange1f8 *)tRangesSOA.data(),
                                   nullptr);

          VKLInterval8 interval;
          int result[8];

          int intervalCount = 0;

          while (true) {
            vklIterateInterval8(valid.data(), &iterator, &interval, result);

            int resultSum = 0;

            for (int i = 0; i < width; i++) {
              resultSum += result[i];
            }

            // we should have the same result for all active lanes
            REQUIRE((resultSum == width || resultSum == 0));

            if (!result[0]) {
              break;
            }

            for (int i = 0; i < width; i++) {
              INFO("interval iteration "
                   << intervalCount << " lane[" << i
                   << "] tRange = " << interval.tRange.lower[i] << ", "
                   << interval.tRange.upper[i]
                   << " valueRange = " << interval.valueRange.lower[i] << ", "
                   << interval.valueRange.upper[i]);

              vkl_range1f sampledValueRange = computeIntervalValueRange(
                  vklVolume,
                  reinterpret_cast<vkl_vec3f &>(origins[i]),
                  reinterpret_cast<vkl_vec3f &>(directions[i]),
                  vkl_range1f{interval.tRange.lower[i],
                              interval.tRange.upper[i]});

              INFO("sampled value range = " << sampledValueRange.lower << ", "
                                            << sampledValueRange.upper);

              // the sampled value range should be completely within the
              // returned interval value range
              REQUIRE(
                  (sampledValueRange.lower >= interval.valueRange.lower[i] &&
                   sampledValueRange.upper <= interval.valueRange.upper[i]));
            }

            intervalCount++;
          }

          // make sure we had at least one interval...
          REQUIRE(intervalCount > 0);
        }

        else if (callingWidth == 16) {
          VKLIntervalIterator16 iterator;
          vklInitIntervalIterator16(valid.data(),
                                    &iterator,
                                    vklVolume,
                                    (const vkl_vvec3f16 *)originsSOA.data(),
                                    (const vkl_vvec3f16 *)directionsSOA.data(),
                                    (const vkl_vrange1f16 *)tRangesSOA.data(),
                                    nullptr);

          VKLInterval16 interval;
          int result[16];

          int intervalCount = 0;

          while (true) {
            vklIterateInterval16(valid.data(), &iterator, &interval, result);

            int resultSum = 0;

            for (int i = 0; i < width; i++) {
              resultSum += result[i];
            }

            // we should have the same result for all active lanes
            REQUIRE((resultSum == width || resultSum == 0));

            if (!result[0]) {
              break;
            }

            for (int i = 0; i < width; i++) {
              INFO("interval iteration "
                   << intervalCount << " lane[" << i
                   << "] tRange = " << interval.tRange.lower[i] << ", "
                   << interval.tRange.upper[i]
                   << " valueRange = " << interval.valueRange.lower[i] << ", "
                   << interval.valueRange.upper[i]);

              vkl_range1f sampledValueRange = computeIntervalValueRange(
                  vklVolume,
                  reinterpret_cast<vkl_vec3f &>(origins[i]),
                  reinterpret_cast<vkl_vec3f &>(directions[i]),
                  vkl_range1f{interval.tRange.lower[i],
                              interval.tRange.upper[i]});

              INFO("sampled value range = " << sampledValueRange.lower << ", "
                                            << sampledValueRange.upper);

              // the sampled value range should be completely within the
              // returned interval value range
              REQUIRE(
                  (sampledValueRange.lower >= interval.valueRange.lower[i] &&
                   sampledValueRange.upper <= interval.valueRange.upper[i]));
            }

            intervalCount++;
          }

          // make sure we had at least one interval...
          REQUIRE(intervalCount > 0);
        }

        else {
          throw std::runtime_error("unsupported calling width");
        }
      }
    }
  }

  SECTION("randomized interval value ranges with value selector")
  {
    VKLValueSelector valueSelector = vklNewValueSelector(vklVolume);

    // will trigger intervals covering individual ranges separately
    std::vector<vkl_range1f> valueRanges{{0.9f, 1.f}, {1.9f, 2.f}};

    vklValueSelectorSetRanges(
        valueSelector, valueRanges.size(), valueRanges.data());

    vklCommit(valueSelector);

    for (int width = 1; width < maxWidth; width++) {
      std::vector<vec3f> origins(width);
      std::vector<vec3f> directions(width);
      std::vector<vkl_range1f> tRanges(width);

      for (int i = 0; i < width; i++) {
        origins[i]    = vec3f(distX(eng), distY(eng), -1.f);
        directions[i] = vec3f(0.f, 0.f, 1.f);
        tRanges[i]    = vkl_range1f{0.f, inf};
      }

      for (auto callingWidth : nativeWidths) {
        if (width > callingWidth || callingWidth != nativeSIMDWidth) {
          continue;
        }

        std::vector<int> valid(callingWidth, 0);
        std::fill(valid.begin(), valid.begin() + width, 1);

        std::vector<float> originsSOA = AOStoSOA_vec3f(origins, callingWidth);
        std::vector<float> directionsSOA =
            AOStoSOA_vec3f(directions, callingWidth);
        std::vector<float> tRangesSOA = AOStoSOA_range1f(tRanges, callingWidth);

        if (callingWidth == 4) {
          VKLIntervalIterator4 iterator;
          vklInitIntervalIterator4(valid.data(),
                                   &iterator,
                                   vklVolume,
                                   (const vkl_vvec3f4 *)originsSOA.data(),
                                   (const vkl_vvec3f4 *)directionsSOA.data(),
                                   (const vkl_vrange1f4 *)tRangesSOA.data(),
                                   valueSelector);

          VKLInterval4 interval;
          int result[4];

          int intervalCount = 0;

          while (true) {
            vklIterateInterval4(valid.data(), &iterator, &interval, result);

            int resultSum = 0;

            for (int i = 0; i < width; i++) {
              resultSum += result[i];
            }

            if (!resultSum) {
              break;
            }

            for (int i = 0; i < width; i++) {
              if (!result[i]) {
                break;
              }

              INFO("interval iteration "
                   << intervalCount << " lane[" << i
                   << "] tRange = " << interval.tRange.lower[i] << ", "
                   << interval.tRange.upper[i]
                   << " valueRange = " << interval.valueRange.lower[i] << ", "
                   << interval.valueRange.upper[i]);

              vkl_range1f sampledValueRange = computeIntervalValueRange(
                  vklVolume,
                  reinterpret_cast<vkl_vec3f &>(origins[i]),
                  reinterpret_cast<vkl_vec3f &>(directions[i]),
                  vkl_range1f{interval.tRange.lower[i],
                              interval.tRange.upper[i]});

              INFO("sampled value range = " << sampledValueRange.lower << ", "
                                            << sampledValueRange.upper);

              // the sampled value range should be completely within the
              // returned interval value range
              REQUIRE(
                  (sampledValueRange.lower >= interval.valueRange.lower[i] &&
                   sampledValueRange.upper <= interval.valueRange.upper[i]));

              // the interval value range should overlap the value selector
              // value range(s)
              bool rangeIntersectsValueSelector = false;

              for (const auto &r : valueRanges) {
                if (rangesIntersect(
                        r,
                        vkl_range1f{interval.valueRange.lower[i],
                                    interval.valueRange.upper[i]})) {
                  rangeIntersectsValueSelector = true;
                  break;
                }
              }

              REQUIRE(rangeIntersectsValueSelector);
            }

            intervalCount++;
          }

          // make sure we had at least one interval...
          REQUIRE(intervalCount > 0);
        }

        else if (callingWidth == 8) {
          VKLIntervalIterator8 iterator;
          vklInitIntervalIterator8(valid.data(),
                                   &iterator,
                                   vklVolume,
                                   (const vkl_vvec3f8 *)originsSOA.data(),
                                   (const vkl_vvec3f8 *)directionsSOA.data(),
                                   (const vkl_vrange1f8 *)tRangesSOA.data(),
                                   valueSelector);

          VKLInterval8 interval;
          int result[8];

          int intervalCount = 0;

          while (true) {
            vklIterateInterval8(valid.data(), &iterator, &interval, result);

            int resultSum = 0;

            for (int i = 0; i < width; i++) {
              resultSum += result[i];
            }

            if (!resultSum) {
              break;
            }

            for (int i = 0; i < width; i++) {
              if (!result[i]) {
                break;
              }

              INFO("interval iteration "
                   << intervalCount << " lane[" << i
                   << "] tRange = " << interval.tRange.lower[i] << ", "
                   << interval.tRange.upper[i]
                   << " valueRange = " << interval.valueRange.lower[i] << ", "
                   << interval.valueRange.upper[i]);

              vkl_range1f sampledValueRange = computeIntervalValueRange(
                  vklVolume,
                  reinterpret_cast<vkl_vec3f &>(origins[i]),
                  reinterpret_cast<vkl_vec3f &>(directions[i]),
                  vkl_range1f{interval.tRange.lower[i],
                              interval.tRange.upper[i]});

              INFO("sampled value range = " << sampledValueRange.lower << ", "
                                            << sampledValueRange.upper);

              // the sampled value range should be completely within the
              // returned interval value range
              REQUIRE(
                  (sampledValueRange.lower >= interval.valueRange.lower[i] &&
                   sampledValueRange.upper <= interval.valueRange.upper[i]));

              // the interval value range should overlap the value selector
              // value range(s)
              bool rangeIntersectsValueSelector = false;

              for (const auto &r : valueRanges) {
                if (rangesIntersect(
                        r,
                        vkl_range1f{interval.valueRange.lower[i],
                                    interval.valueRange.upper[i]})) {
                  rangeIntersectsValueSelector = true;
                  break;
                }
              }

              REQUIRE(rangeIntersectsValueSelector);
            }

            intervalCount++;
          }

          // make sure we had at least one interval...
          REQUIRE(intervalCount > 0);
        }

        else if (callingWidth == 16) {
          VKLIntervalIterator16 iterator;
          vklInitIntervalIterator16(valid.data(),
                                    &iterator,
                                    vklVolume,
                                    (const vkl_vvec3f16 *)originsSOA.data(),
                                    (const vkl_vvec3f16 *)directionsSOA.data(),
                                    (const vkl_vrange1f16 *)tRangesSOA.data(),
                                    valueSelector);

          VKLInterval16 interval;
          int result[16];

          int intervalCount = 0;

          while (true) {
            vklIterateInterval16(valid.data(), &iterator, &interval, result);

            int resultSum = 0;

            for (int i = 0; i < width; i++) {
              resultSum += result[i];
            }

            if (!resultSum) {
              break;
            }

            for (int i = 0; i < width; i++) {
              if (!result[i]) {
                break;
              }

              INFO("interval iteration "
                   << intervalCount << " lane[" << i
                   << "] tRange = " << interval.tRange.lower[i] << ", "
                   << interval.tRange.upper[i]
                   << " valueRange = " << interval.valueRange.lower[i] << ", "
                   << interval.valueRange.upper[i]);

              vkl_range1f sampledValueRange = computeIntervalValueRange(
                  vklVolume,
                  reinterpret_cast<vkl_vec3f &>(origins[i]),
                  reinterpret_cast<vkl_vec3f &>(directions[i]),
                  vkl_range1f{interval.tRange.lower[i],
                              interval.tRange.upper[i]});

              INFO("sampled value range = " << sampledValueRange.lower << ", "
                                            << sampledValueRange.upper);

              // the sampled value range should be completely within the
              // returned interval value range
              REQUIRE(
                  (sampledValueRange.lower >= interval.valueRange.lower[i] &&
                   sampledValueRange.upper <= interval.valueRange.upper[i]));

              // the interval value range should overlap the value selector
              // value range(s)
              bool rangeIntersectsValueSelector = false;

              for (const auto &r : valueRanges) {
                if (rangesIntersect(
                        r,
                        vkl_range1f{interval.valueRange.lower[i],
                                    interval.valueRange.upper[i]})) {
                  rangeIntersectsValueSelector = true;
                  break;
                }
              }

              REQUIRE(rangeIntersectsValueSelector);
            }

            intervalCount++;
          }

          // make sure we had at least one interval...
          REQUIRE(intervalCount > 0);
        }

        else {
          throw std::runtime_error("unsupported calling width");
        }
      }
    }
  }
}

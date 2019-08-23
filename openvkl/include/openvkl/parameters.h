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

#pragma once

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

void vklSetBool(VKLObject object, const char *name, int b);
void vklSetFloat(VKLObject object, const char *name, float x);
void vklSetVec3f(VKLObject object, const char *name, float x, float y, float z);
void vklSetInt(VKLObject object, const char *name, int x);
void vklSetVec3i(VKLObject object, const char *name, int x, int y, int z);
void vklSetData(VKLObject object, const char *name, VKLData data);
void vklSetString(VKLObject object, const char *name, const char *s);
void vklSetVoidPtr(VKLObject object, const char *name, void *v);

#ifdef __cplusplus
}  // extern "C"
#endif

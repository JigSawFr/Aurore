/*
    Copyright 2019-2021 natinusala
    Copyright 2019 p-sam

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#pragma once

// legacy-names: begin
// borealis' name, still read (deprecated)
#if !defined(AURORE_RESOURCES) && defined(BRLS_RESOURCES)
#define AURORE_RESOURCES BRLS_RESOURCES
#endif
// legacy-names: end
#ifndef AURORE_RESOURCES
#error AURORE_RESOURCES define missing
#endif
#ifdef USE_LIBROMFS
#define AURORE_ASSET(_str) "@res/" _str
#else
#define AURORE_ASSET(_str) AURORE_RESOURCES _str
#endif

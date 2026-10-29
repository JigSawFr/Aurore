// SPDX-FileCopyrightText: 2019-2021 natinusala
// SPDX-FileCopyrightText: 2019 p-sam
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifndef AURORE_RESOURCES
#error AURORE_RESOURCES define missing
#endif
#ifdef USE_LIBROMFS
#define AURORE_ASSET(_str) "@res/" _str
#else
#define AURORE_ASSET(_str) AURORE_RESOURCES _str
#endif

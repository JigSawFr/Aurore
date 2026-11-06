// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

#include <aurore/core/cpu_boost.hpp>

namespace aurore
{

CpuBoostCounter::CpuBoostCounter(std::function<void(bool)> sink)
    : sink(std::move(sink))
{
}

void CpuBoostCounter::acquire()
{
    std::lock_guard<std::mutex> lock(this->mutex);
    if (this->count++ == 0 && this->sink)
        this->sink(true);
}

void CpuBoostCounter::release()
{
    std::lock_guard<std::mutex> lock(this->mutex);
    if (this->count == 0)
        return;
    if (--this->count == 0 && this->sink)
        this->sink(false);
}

void CpuBoostCounter::releaseAll()
{
    std::lock_guard<std::mutex> lock(this->mutex);
    if (this->count == 0)
        return;
    this->count = 0;
    if (this->sink)
        this->sink(false);
}

int CpuBoostCounter::holders() const
{
    std::lock_guard<std::mutex> lock(this->mutex);
    return this->count;
}

} // namespace aurore

// SPDX-FileCopyrightText: 2022 xfangfang
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace aurore
{

template <typename T>
class Singleton
{
  public:
    static T& instance()
    {
        static T instance;
        return instance;
    }

    Singleton(const Singleton&)           = delete;
    Singleton& operator=(const Singleton) = delete;

  protected:
    Singleton() { }
};

}
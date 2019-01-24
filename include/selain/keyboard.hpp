/*
 * Copyright (c) 2019, Rauli Laine
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef SELAIN_KEYBOARD_HPP_GUARD
#define SELAIN_KEYBOARD_HPP_GUARD

#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>

namespace selain
{
  class MainWindow;
  class Tab;

  namespace keyboard
  {
    using BindingCallback = std::function<void(MainWindow&, Tab&)>;

    struct Mapping
    {
      using mapping_type = std::unordered_map<
        std::uint32_t,
        std::shared_ptr<Mapping>
      >;

      std::uint32_t value;
      BindingCallback callback;
      mapping_type mapping;
      mapping_type control_mapping;
    };

    /**
     * Initializes all builtin keyboard bindings. Must be called during the
     * application startup before the UI is being shown.
     */
    void initialize();
  }
}

#endif /* !SELAIN_KEYBOARD_HPP_GUARD */

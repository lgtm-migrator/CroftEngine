#pragma once

#include "glassert.h"

#include <gsl-lite.hpp>

namespace gl
{
#define _SOGLB_LINE_NAME(prefix) /*NOLINT(bugprone-reserved-identifier)*/ prefix##__LINE__
#define SOGLB_DEBUGGROUP(message)                                         \
  [[maybe_unused]] ::gl::DebugGroup _SOGLB_LINE_NAME(_soglb_debug_group_) \
  {                                                                       \
    message                                                               \
  }

class DebugGroup final
{
public:
  explicit DebugGroup(const std::string& message, const uint32_t id = 0)
  {
    GL_ASSERT(api::pushDebugGroup(api::DebugSource::DebugSourceApplication,
                                  id,
                                  gsl::narrow<api::core::SizeType>(message.length()),
                                  message.c_str()));
  }

  DebugGroup(const DebugGroup&) = delete;

  DebugGroup(DebugGroup&&) noexcept = delete;

  DebugGroup& operator=(const DebugGroup&) = delete;

  DebugGroup& operator=(DebugGroup&&) = delete;

  ~DebugGroup()
  {
    GL_ASSERT(api::popDebugGroup());
  }
};
} // namespace gl

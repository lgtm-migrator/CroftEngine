#pragma once

#include "serialization/serialization_fwd.h"

#include <optional>
#include <string>

namespace engine
{
struct EngineConfig;
}

namespace render
{
struct RenderSettings
{
  bool crt = true;
  bool dof = true;
  bool lensDistortion = true;
  bool filmGrain = true;
  bool fullscreen = false;
  bool bilinearFiltering = false;
  bool anisotropicFiltering = true;
  bool waterDenoise = true;
  bool hbao = true;
  bool velvia = true;
  bool moreLights = true;
  bool highQualityShadows = true;
  std::optional<std::string> glidosPack = std::nullopt;

  [[nodiscard]] size_t getLightCollectionDepth() const
  {
    return moreLights ? 2 : 1;
  }

  [[nodiscard]] int32_t getCSMResolution() const
  {
    return highQualityShadows ? 2048 : 1024;
  }

  void serialize(const serialization::Serializer<engine::EngineConfig>& ser);
};
} // namespace render

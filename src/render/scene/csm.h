#pragma once

#include "blur.h"
#include "core/vec.h"

#include <cstdint>
#include <gl/buffer.h>
#include <gl/pixel.h>
#include <gl/soglb_fwd.h>
#include <glm/glm.hpp>
#include <gsl/gsl-lite.hpp>

namespace render::scene
{
class Camera;
class Material;
class Mesh;
class MaterialManager;

struct CSMBuffer
{
  static constexpr size_t NSplits = 5;

  std::array<glm::mat4, NSplits> lightMVP{};
  glm::vec4 lightDir{};
  std::array<glm::vec4, NSplits> csmSplits{};
};
static_assert(sizeof(CSMBuffer) % 16 == 0);

class CSM final
{
public:
  struct Split final
  {
    glm::mat4 vpMatrix{1.0f};
    std::shared_ptr<gl::TextureDepth<float>> depthTexture;
    std::shared_ptr<gl::Framebuffer> depthFramebuffer{};
    std::shared_ptr<gl::Texture2D<gl::RG16F>> squaredTexture;
    std::shared_ptr<gl::Framebuffer> squareFramebuffer{};
    std::shared_ptr<Material> squareMaterial{};
    std::shared_ptr<Mesh> squareMesh{};
    std::shared_ptr<SeparableBlur<gl::RG16F>> squareBlur;
    float end = 0;

    void init(int32_t resolution, size_t idx, MaterialManager& materialManager);
    void renderSquare();
    void renderBlur();
  };

  explicit CSM(int32_t resolution, MaterialManager& materialManager);

  [[nodiscard]] std::array<std::shared_ptr<gl::Texture2D<gl::RG16F>>, CSMBuffer::NSplits> getTextures() const;
  [[nodiscard]] std::array<glm::mat4, CSMBuffer::NSplits> getMatrices(const glm::mat4& modelMatrix) const;
  [[nodiscard]] std::array<float, CSMBuffer::NSplits> getSplitEnds() const;

  [[nodiscard]] auto getActiveMatrix(const glm::mat4& modelMatrix) const
  {
    return m_splits.at(m_activeSplit).vpMatrix * modelMatrix;
  }

  [[nodiscard]] const auto& getActiveFramebuffer() const
  {
    return m_splits.at(m_activeSplit).depthFramebuffer;
  }

  void setActiveSplit(size_t idx)
  {
    Expects(idx < m_splits.size());
    m_activeSplit = idx;
  }

  void updateCamera(const Camera& camera);

  auto& getBuffer(const glm::mat4& modelMatrix)
  {
    const auto splitEnds = getSplitEnds();
    std::transform(splitEnds.begin(),
                   splitEnds.end(),
                   m_bufferData.csmSplits.begin(),
                   [](float v) {
                     return glm::vec4{v, 0, 0, 0};
                   });
    m_bufferData.lightMVP = getMatrices(modelMatrix);
    m_bufferData.lightDir = glm::vec4{m_lightDir, 0.0f};
    m_buffer.setData(m_bufferData, gl::api::BufferUsageARB::DynamicDraw);
    return m_buffer;
  }

  // NOLINTNEXTLINE(readability-make-member-function-const)
  void applyViewport()
  {
    gl::RenderState::getWantedState().setViewport(glm::ivec2{m_resolution, m_resolution});
  }

  void renderSquare()
  {
    m_splits.at(m_activeSplit).renderSquare();
  }

  void renderBlur()
  {
    m_splits.at(m_activeSplit).renderBlur();
  }

private:
  const int32_t m_resolution;
  const glm::vec3 m_lightDir{core::TRVec{0_len, 1_len, 0_len}.toRenderSystem()};
  const glm::vec3 m_lightDirOrtho{core::TRVec{1_len, 0_len, 0_len}.toRenderSystem()};
  std::array<Split, CSMBuffer::NSplits> m_splits;
  size_t m_activeSplit = 0;
  CSMBuffer m_bufferData;
  gl::UniformBuffer<CSMBuffer> m_buffer{"csm-data-ubo"};
};
} // namespace render::scene

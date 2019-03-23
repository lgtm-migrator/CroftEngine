#pragma once

#include "Drawable.h"
#include "Mesh.h"

#include "gsl-lite.hpp"

namespace render
{
namespace scene
{
class Model : public Drawable
{
public:
    explicit Model() = default;

    ~Model() override = default;

    Model(const Model&) = delete;

    Model(Model&&) = delete;

    Model& operator=(Model&&) = delete;

    Model& operator=(const Model&) = delete;

    const std::vector<gsl::not_null<std::shared_ptr<Mesh>>>& getMeshes() const
    {
        return m_meshes;
    }

    void addMesh(const gsl::not_null<std::shared_ptr<Mesh>>& mesh)
    {
        m_meshes.emplace_back( mesh );
    }

    void draw(RenderContext& context) override;

private:
    std::vector<gsl::not_null<std::shared_ptr<Mesh>>> m_meshes{};
};
}
}
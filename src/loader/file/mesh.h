#pragma once

#include "color.h"
#include "core/vec.h"
#include "primitives.h"
#include "render/scene/mesh.h"
#include "render/scene/names.h"
#include "texture.h"

#include <vector>

namespace render
{
class TextureAnimator;

namespace scene
{
class Model;
}
} // namespace render

namespace loader::file
{
namespace io
{
class SDLReader;
}

struct Mesh
{
  core::TRVec
    center; // This is usually close to the mesh's centroid, and appears to be the center of a sphere used for collision testing.
  core::Length collision_size = 0_len;   // This appears to be the radius of that aforementioned collisional sphere.
  std::vector<core::TRVec> vertices;     //[NumVertices]; // list of vertices (relative coordinates)
  std::vector<core::TRVec> normals;      //[NumNormals]; // list of normals (if NumNormals is positive)
  std::vector<int16_t> vertexDarknesses; //[-NumNormals]; // list of light values (if NumNormals is negative), 0..8191
  std::vector<QuadFace> textured_rectangles; //[NumTexturedRectangles]; // list of textured rectangles
  std::vector<Triangle> textured_triangles;  //[NumTexturedTriangles]; // list of textured triangles
  // the rest is not present in TR4
  std::vector<QuadFace> colored_rectangles; //[NumColoredRectangles]; // list of colored rectangles
  std::vector<Triangle> colored_triangles;  //[NumColoredTriangles]; // list of colored triangles

  /** \brief reads mesh definition.
    *
    * The read num_normals value is positive when normals are available and negative when light
    * values are available. The values get set appropriately.
    */
  static std::unique_ptr<Mesh> readTr1(io::SDLReader& reader);

  static std::unique_ptr<Mesh> readTr4(io::SDLReader& reader);

  [[nodiscard]] std::shared_ptr<render::scene::Model>
    createModel(const std::vector<TextureTile>& textureTiles,
                const std::map<TextureKey, gsl::not_null<std::shared_ptr<render::scene::Material>>>& materials,
                const gsl::not_null<std::shared_ptr<render::scene::Material>>& colorMaterial,
                const Palette& palette,
                const std::string& label = {}) const;
};
} // namespace loader::file

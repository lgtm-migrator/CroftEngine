#pragma once

#include "core/angle.h"
#include "core/units.h"
#include "core/vec.h"
#include "menuobject.h"

#include <memory>

namespace engine
{
class Engine;
}

namespace menu
{
struct MenuRing
{
  enum class Type
  {
    Inventory,
    Options,
    Items
  };

  std::string title;
  Type type;
  std::vector<MenuObject> list;
  size_t currentObject = 0;

  explicit MenuRing(Type type, std::string title, std::vector<MenuObject> list);

  [[nodiscard]] auto getAnglePerItem() const
  {
    Expects(!list.empty());
    const auto anglePerItemDeg = 360.0f / list.size();
    return core::angleFromDegrees(anglePerItemDeg);
  }

  MenuObject& getSelectedObject()
  {
    return list.at(currentObject);
  }

  [[nodiscard]] core::Angle getCurrentObjectAngle() const
  {
    return getAnglePerItem() * gsl::narrow_cast<int>(currentObject);
  }
};
} // namespace menu
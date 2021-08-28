#include "pathfinder.h"

#include "engine/world/world.h"
#include "serialization/box_ptr.h"
#include "serialization/deque.h"
#include "serialization/not_null.h"
#include "serialization/quantity.h"
#include "serialization/serialization.h"
#include "serialization/unordered_map.h"
#include "serialization/unordered_set.h"
#include "serialization/vector.h"
#include "serialization/vector_element.h"

namespace engine::ai
{
namespace
{
template<typename T>
[[nodiscard]] constexpr const auto& uncheckedClamp(const T& x, const core::Interval<T>& interval)
{
  if(x < interval.min)
    return interval.min;
  else if(x > interval.max)
    return interval.max;
  else
    return x;
}
} // namespace

bool PathFinder::calculateTarget(const world::World& world,
                                 core::TRVec& moveTarget,
                                 const core::TRVec& startPos,
                                 const gsl::not_null<const world::Box*>& startBox)
{
  Expects(m_targetBox != nullptr);
  Expects(m_targetBox->xInterval.contains(target.X));
  Expects(m_targetBox->zInterval.contains(target.Z));
  Expects(startBox->xInterval.contains(startPos.X));
  Expects(startBox->zInterval.contains(startPos.Z));
  searchPath(world);

  moveTarget = startPos;

  auto here = startBox;
  core::Interval<core::Length> xRange{0_len, 0_len};
  core::Interval<core::Length> zRange{0_len, 0_len};

  static constexpr uint8_t CanMoveXPos = 0x01u;
  static constexpr uint8_t CanMoveXNeg = 0x02u;
  static constexpr uint8_t CanMoveZPos = 0x04u;
  static constexpr uint8_t CanMoveZNeg = 0x08u;
  // NOLINTNEXTLINE(hicpp-signed-bitwise)
  static constexpr uint8_t CanMoveAllDirs = CanMoveXPos | CanMoveXNeg | CanMoveZPos | CanMoveZNeg;

  bool detour = false;

  uint8_t moveDirs = CanMoveAllDirs;
  while(true)
  {
    if(isFlying())
    {
      moveTarget.Y = std::min(moveTarget.Y, here->floor - core::SectorSize);
    }
    else
    {
      moveTarget.Y = std::min(moveTarget.Y, here->floor);
    }

    if(here->xInterval.contains(startPos.X) && here->zInterval.contains(startPos.Z))
    {
      xRange = here->xInterval;
      zRange = here->zInterval;
    }
    else
    {
      if(startPos.Z < here->zInterval.min)
      {
        // try to move to -Z
        if((moveDirs & CanMoveZNeg) && here->xInterval.contains(startPos.X))
        {
          // can move straight to -Z while not leaving the X limits of the current box
          moveTarget.Z = std::max(moveTarget.Z, here->zInterval.min + Margin);

          if(detour)
            return true;

          xRange = xRange.intersect(here->xInterval);
          moveDirs = CanMoveZNeg;
        }
        else if(detour || moveDirs != CanMoveZNeg)
        {
          moveTarget.Z = zRange.max - Margin;
          if(detour || moveDirs != CanMoveAllDirs)
            return true;

          detour = true;
        }
      }
      else if(startPos.Z > here->zInterval.max)
      {
        if((moveDirs & CanMoveZPos) && here->xInterval.contains(startPos.X))
        {
          moveTarget.Z = std::min(moveTarget.Z, here->zInterval.max - Margin);

          if(detour)
            return true;

          xRange = xRange.intersect(here->xInterval);
          moveDirs = CanMoveZPos;
        }
        else if(detour || moveDirs != CanMoveZPos)
        {
          moveTarget.Z = zRange.min + Margin;
          if(detour || moveDirs != CanMoveAllDirs)
            return true;

          detour = true;
        }
      }

      if(startPos.X < here->xInterval.min)
      {
        if((moveDirs & CanMoveXNeg) && here->zInterval.contains(startPos.Z))
        {
          moveTarget.X = std::max(moveTarget.X, here->xInterval.min + Margin);

          if(detour)
            return true;

          zRange = zRange.intersect(here->zInterval);
          moveDirs = CanMoveXNeg;
        }
        else if(detour || moveDirs != CanMoveXNeg)
        {
          moveTarget.X = xRange.max - Margin;
          if(detour || moveDirs != CanMoveAllDirs)
            return true;

          detour = true;
        }
      }
      else if(startPos.X > here->xInterval.max)
      {
        if((moveDirs & CanMoveXPos) && here->zInterval.contains(startPos.Z))
        {
          moveTarget.X = std::min(moveTarget.X, here->xInterval.max - Margin);

          if(detour)
            return true;

          zRange = zRange.intersect(here->zInterval);
          moveDirs = CanMoveXPos;
        }
        else if(detour || moveDirs != CanMoveXPos)
        {
          moveTarget.X = xRange.min + Margin;
          if(detour || moveDirs != CanMoveAllDirs)
            return true;

          detour = true;
        }
      }
    }

    if(here == m_targetBox)
    {
      // NOLINTNEXTLINE(hicpp-signed-bitwise)
      if(moveDirs & (CanMoveZPos | CanMoveZNeg))
      {
        moveTarget.Z = target.Z;
      }
      else if(!detour)
      {
        moveTarget.Z = uncheckedClamp(moveTarget.Z, here->zInterval.narrowed(Margin));
      }
      Ensures(here->zInterval.contains(moveTarget.Z));

      // NOLINTNEXTLINE(hicpp-signed-bitwise)
      if(moveDirs & (CanMoveXPos | CanMoveXNeg))
      {
        moveTarget.X = target.X;
      }
      else if(!detour)
      {
        moveTarget.X = uncheckedClamp(moveTarget.X, here->xInterval.narrowed(Margin));
      }
      Ensures(here->xInterval.contains(moveTarget.X));

      moveTarget.Y = target.Y;

      return true;
    }

    const auto nextBox = getNextPathBox(here);
    if(nextBox == nullptr || !canVisit(*nextBox))
      break;

    here = nextBox;
  }

  // NOLINTNEXTLINE(hicpp-signed-bitwise)
  if(moveDirs & (CanMoveZPos | CanMoveZNeg))
  {
    const auto range = here->zInterval.size() - 2 * Margin;
    moveTarget.Z = util::rand15(range) + here->zInterval.min + Margin;
  }
  else if(!detour)
  {
    moveTarget.Z = uncheckedClamp(moveTarget.Z, here->zInterval.narrowed(Margin));
  }
  Ensures(here->zInterval.contains(moveTarget.Z));

  // NOLINTNEXTLINE(hicpp-signed-bitwise)
  if(moveDirs & (CanMoveXPos | CanMoveXNeg))
  {
    const auto range = here->xInterval.size() - 2 * Margin;
    moveTarget.X = util::rand15(range) + here->xInterval.min + Margin;
  }
  else if(!detour)
  {
    moveTarget.X = uncheckedClamp(moveTarget.X, here->xInterval.narrowed(Margin));
  }
  Ensures(here->xInterval.contains(moveTarget.X));

  if(isFlying())
    moveTarget.Y = here->floor - 384_len;
  else
    moveTarget.Y = here->floor;

  return false;
}

void PathFinder::searchPath(const world::World& world)
{
  const auto zoneRef = world::Box::getZoneRef(world.roomsAreSwapped(), isFlying(), step);

  static constexpr uint8_t MaxExpansions = 15;

  auto setReachable = [this](const world::Box* box, bool reachable)
  {
    m_reachable[box] = reachable;
    if(std::find(m_expansions.begin(), m_expansions.end(), box) == m_expansions.end())
      m_expansions.emplace_back(box);
  };

  for(uint8_t i = 0; i < MaxExpansions && !m_expansions.empty(); ++i)
  {
    const auto currentBox = m_expansions.front();
    m_expansions.pop_front();
    const auto searchZone = currentBox.get()->*zoneRef;

    for(const auto& successorBox : currentBox->overlaps)
    {
      if(successorBox == currentBox)
        continue;

      if(searchZone != successorBox.get()->*zoneRef)
        continue;

      if(const auto boxHeightDiff = successorBox->floor - currentBox->floor;
         boxHeightDiff > step || boxHeightDiff < drop)
        continue;

      const auto it = m_reachable.find(successorBox);
      const bool initialized = it != m_reachable.end();

      if(!m_reachable.at(currentBox))
      {
        // propagate "unreachable" to all connected boxes if their reachability hasn't been determined yet
        if(!initialized)
        {
          setReachable(successorBox, false);
        }
      }
      else
      {
        // propagate "reachable" to all connected boxes if their reachability hasn't been determined yet
        // OR they were previously determined to be unreachable
        if(initialized && it->second)
          continue; // already visited and marked reachable

        const auto reachable = canVisit(*successorBox);
        if(reachable)
        {
          BOOST_ASSERT_MSG(m_edges.count(successorBox) == 0, "cycle in pathfinder graph detected");
          m_edges.emplace(successorBox, currentBox); // success! connect both boxes
        }

        setReachable(successorBox, reachable);
      }
    }
  }
}

void PathFinder::serialize(const serialization::Serializer<world::World>& ser)
{
  ser(S_NV("edges", m_edges),
      S_NV("boxes", m_boxes),
      S_NV("expansions", m_expansions),
      S_NV("reachable", m_reachable),
      S_NV("cannotVisitBlockable", cannotVisitBlockable),
      S_NV("cannotVisitBlocked", cannotVisitBlocked),
      S_NV("step", step),
      S_NV("drop", drop),
      S_NV("fly", fly),
      S_NV_VECTOR_ELEMENT("targetBox", ser.context.getBoxes(), m_targetBox),
      S_NV("target", target));
}

void PathFinder::collectBoxes(const world::World& world, const gsl::not_null<const world::Box*>& box)
{
  const auto zoneRef1 = world::Box::getZoneRef(false, isFlying(), step);
  const auto zoneRef2 = world::Box::getZoneRef(true, isFlying(), step);
  const auto zoneData1 = box.get()->*zoneRef1;
  const auto zoneData2 = box.get()->*zoneRef2;
  m_boxes.clear();
  for(const auto& levelBox : world.getBoxes())
  {
    if(levelBox.*zoneRef1 == zoneData1 || levelBox.*zoneRef2 == zoneData2)
    {
      m_boxes.emplace_back(&levelBox);
    }
  }
}
} // namespace engine::ai

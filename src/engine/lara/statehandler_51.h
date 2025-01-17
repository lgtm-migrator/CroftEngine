#pragma once

#include "abstractstatehandler.h"
#include "engine/collisioninfo.h"
#include "engine/world/skeletalmodeltype.h"

namespace engine::lara
{
class StateHandler_51 final : public AbstractStateHandler
{
public:
  explicit StateHandler_51(objects::LaraObject& lara)
      : AbstractStateHandler{lara, LaraStateId::MidasDeath}
  {
  }

  void handleInput(CollisionInfo& collisionInfo) override
  {
    getLara().m_state.falling = false;
    collisionInfo.policies &= ~CollisionInfo::SpazPushPolicy;
    const auto& alternateLara = getWorld().findAnimatedModelForType(TR1ItemId::AlternativeLara);
    if(alternateLara == nullptr)
      return;

    static constexpr gl::SRGBA8 GoldColor{255, 192, 64, 255};
    const auto skeleton = getLara().getSkeleton();
    switch(skeleton->getLocalFrame().get())
    {
    case 5:
      skeleton->setMeshPart(3, alternateLara->bones[3].mesh);
      skeleton->setMeshReflective(3, GoldColor);
      skeleton->setMeshPart(6, alternateLara->bones[6].mesh);
      skeleton->setMeshReflective(6, GoldColor);
      break;
    case 70:
      skeleton->setMeshPart(2, alternateLara->bones[2].mesh);
      skeleton->setMeshReflective(2, GoldColor);
      break;
    case 90:
      skeleton->setMeshPart(1, alternateLara->bones[1].mesh);
      skeleton->setMeshReflective(1, GoldColor);
      break;
    case 100:
      skeleton->setMeshPart(5, alternateLara->bones[5].mesh);
      skeleton->setMeshReflective(5, GoldColor);
      break;
    case 120:
      skeleton->setMeshPart(0, alternateLara->bones[0].mesh);
      skeleton->setMeshReflective(0, GoldColor);
      skeleton->setMeshPart(4, alternateLara->bones[4].mesh);
      skeleton->setMeshReflective(4, GoldColor);
      break;
    case 135:
      skeleton->setMeshPart(7, alternateLara->bones[7].mesh);
      skeleton->setMeshReflective(7, GoldColor);
      break;
    case 150:
      skeleton->setMeshPart(11, alternateLara->bones[11].mesh);
      skeleton->setMeshReflective(11, GoldColor);
      break;
    case 163:
      skeleton->setMeshPart(12, alternateLara->bones[12].mesh);
      skeleton->setMeshReflective(12, GoldColor);
      break;
    case 174:
      skeleton->setMeshPart(13, alternateLara->bones[13].mesh);
      skeleton->setMeshReflective(13, GoldColor);
      break;
    case 186:
      skeleton->setMeshPart(8, alternateLara->bones[8].mesh);
      skeleton->setMeshReflective(8, GoldColor);
      break;
    case 195:
      skeleton->setMeshPart(9, alternateLara->bones[9].mesh);
      skeleton->setMeshReflective(9, GoldColor);
      break;
    case 218:
      skeleton->setMeshPart(10, alternateLara->bones[10].mesh);
      skeleton->setMeshReflective(10, GoldColor);
      break;
    case 225:
      skeleton->setMeshPart(14, alternateLara->bones[14].mesh);
      skeleton->setMeshReflective(14, GoldColor);
      getLara().m_state.health = core::DeadHealth;
      break;
    default:
      // silence compiler
      break;
    }
    skeleton->rebuildMesh();
    StateHandler_50::emitSparkles(getWorld());
  }

  void postprocessFrame(CollisionInfo& collisionInfo) override
  {
    collisionInfo.validFloorHeight = {-core::ClimbLimit2ClickMin, core::ClimbLimit2ClickMin};
    collisionInfo.validCeilingHeightMin = 0_len;
    collisionInfo.policies |= CollisionInfo::SlopeBlockingPolicy;
    collisionInfo.facingAngle = getLara().m_state.rotation.Y;
    collisionInfo.initHeightInfo(getLara().m_state.location.position, getWorld(), core::LaraWalkHeight);

    setMovementAngle(collisionInfo.facingAngle);
  }
};
} // namespace engine::lara

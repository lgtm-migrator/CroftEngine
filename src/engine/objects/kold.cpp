#include "kold.h"

#include "engine/cameracontroller.h"
#include "engine/raycast.h"
#include "engine/world/animation.h"
#include "engine/world/world.h"
#include "laraobject.h"
#include "pickupobject.h"
#include "serialization/serialization.h"

namespace engine::objects
{
void Kold::update()
{
  Expects(m_state.creatureInfo != nullptr);

  core::Angle tiltRot = 0_deg;
  core::Angle creatureTurn = 0_deg;
  core::Angle headRot = 0_deg;
  if(alive())
  {
    ai::EnemyLocation enemyLocation{getWorld(), m_state};
    if(enemyLocation.enemyAhead)
    {
      headRot = enemyLocation.angleToEnemy;
    }

    updateMood(getWorld(), m_state, enemyLocation, true);

    creatureTurn = rotateTowardsTarget(m_state.creatureInfo->maxTurnSpeed);
    switch(m_state.current_anim_state.get())
    {
    case 1:
      if(m_state.required_anim_state != 0_as)
      {
        goal(m_state.required_anim_state);
      }
      else if(canShootAtLara(enemyLocation))
      {
        goal(4_as);
      }
      else if(m_state.creatureInfo->mood != ai::Mood::Bored)
      {
        goal(3_as);
      }
      else
      {
        goal(2_as);
      }
      break;
    case 2:
      m_state.creatureInfo->maxTurnSpeed = 3_deg / 1_frame;
      if(m_state.creatureInfo->mood == ai::Mood::Escape || !enemyLocation.enemyAhead)
      {
        require(3_as);
        goal(1_as);
      }
      else if(canShootAtLara(enemyLocation))
      {
        require(4_as);
        goal(1_as);
      }
      else if(enemyLocation.enemyDistance > util::square(4096_len))
      {
        require(3_as);
        goal(1_as);
      }
      break;
    case 3:
      m_state.creatureInfo->maxTurnSpeed = 6_deg / 1_frame;
      tiltRot = creatureTurn / 2;
      if(m_state.creatureInfo->mood != ai::Mood::Escape || enemyLocation.enemyAhead)
      {
        if(canShootAtLara(enemyLocation))
        {
          require(4_as);
          goal(1_as);
        }
        else if(enemyLocation.enemyAhead)
        {
          if(enemyLocation.enemyDistance < util::square(4096_len))
          {
            require(2_as);
            goal(1_as);
          }
        }
      }
      break;
    case 4:
      m_shotAtLara = false;
      if(m_state.required_anim_state != 0_as || !canShootAtLara(enemyLocation))
      {
        goal(1_as);
      }
      else
      {
        goal(6_as);
      }
      break;
    case 6:
      if(!m_shotAtLara)
      {
        if(tryShootAtLara(*this, enemyLocation.enemyDistance / 2, {-20_len, 440_len, 20_len}, 9, headRot))
        {
          hitLara(150_hp);
        }
        m_shotAtLara = true;
      }
      if(m_state.creatureInfo->mood == ai::Mood::Escape)
      {
        require(3_as);
      }
      break;
    default: break;
    }
  }
  else if(m_state.current_anim_state != 5_as) // injured/dying
  {
    getSkeleton()->setAnim(&getWorld().findAnimatedModelForType(TR1ItemId::Kold)->animations[14]);
    m_state.current_anim_state = 5_as;
    getWorld().createPickup(TR1ItemId::ShotgunSprite, m_state.location.room, m_state.location.position);
  }
  rotateCreatureTilt(tiltRot);
  rotateCreatureHead(headRot);
  animateCreature(creatureTurn, 0_deg);
  getSkeleton()->patchBone(0, core::TRRotation{0_deg, m_state.creatureInfo->headRotation, 0_deg}.toMatrix());
}

void Kold::serialize(const serialization::Serializer<world::World>& ser)
{
  AIAgent::serialize(ser);
  ser(S_NV("shotAtLara", m_shotAtLara));
}
} // namespace engine::objects

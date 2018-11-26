#include "PlayerAnimationComponent.h"

PlayerAnimationComponent::PlayerAnimationComponent() :
   mNewAnimationTickTime(4),
   mAnimationTransitionTime(0.25f),
   mUseAnimationTransitions(true),
   mMinLookAngle(-90),
   mMaxLookAngle(90)
{

   //Set up our thread channels

}

PlayerAnimationComponent::~PlayerAnimationComponent()
{
}

void PlayerAnimationComponent::initPersistFields()
{
   addGroup("AnimationController");
   addField("newAnimationTickTime", TypeF32, Offset(mNewAnimationTickTime, PlayerAnimationComponent), "");
   addField("animationTransitionTime", TypeF32, Offset(mAnimationTransitionTime, PlayerAnimationComponent), "");
   addField("useAnimationTransitions", TypeF32, Offset(mUseAnimationTransitions, PlayerAnimationComponent), "");
   addField("minLookAngle", TypeF32, Offset(mMinLookAngle, PlayerAnimationComponent), "");
   addField("maxLookAngle", TypeF32, Offset(mMaxLookAngle, PlayerAnimationComponent), "");
   endGroup("AnimationController");

   Parent::initPersistFields();
}

bool PlayerAnimationComponent::onAdd()
{
   if (!Parent::onAdd())
      return false;

   //Configure our references here for ease of use
   mActionAnimation.thread = mAnimationThreads[0].thread;
   mArmThread = mAnimationThreads[1].thread;
   mHeadVThread = mAnimationThreads[2].thread;
   mHeadHThread = mAnimationThreads[3].thread;
   mRecoilThread = mAnimationThreads[4].thread;

   return true;
}

void PlayerAnimationComponent::onRemove()
{
   Parent::onRemove();
}

void PlayerAnimationComponent::onComponentAdd()
{
   Parent::onComponentAdd();
}

void PlayerAnimationComponent::componentAddedToOwner(Component *comp)
{
   Parent::componentAddedToOwner(comp);
}

void PlayerAnimationComponent::componentRemovedFromOwner(Component *comp)
{
   Parent::componentRemovedFromOwner(comp);
}

void PlayerAnimationComponent::processTick()
{
   Parent::processTick();

   //if (!isActive())
      return;

   Entity::StateDelta delta = mOwner->getNetworkDelta();

   if (delta.warpTicks > 0)
   {
      //updateDeathOffsets();
      updateLookAnimation();
   }
   else
   {
      if (isServerObject())
      {
         updateAnimation(TickSec);

         updateLookAnimation();
         //updateDeathOffsets();

         // Animations are advanced based on frame rate on the
         // client and must be ticked on the server.
         updateActionThread();
         updateAnimationTree(true);
      }
   }
}

void PlayerAnimationComponent::interpolateTick(F32 dt)
{
   Parent::interpolateTick(dt);
   //updateLookAnimation(dt);
}

void PlayerAnimationComponent::advanceTime(F32 dt)
{
   Parent::advanceTime(dt);

   //updateActionThread();
   //updateAnimation(dt);
}

//
//
//
void PlayerAnimationComponent::addAction(String animName, VectorF direction, String tags)
{
   ActionAnimationDef newActionDef;

   newActionDef.name = animName;
   newActionDef.dir = direction;
   newActionDef.tags.push_back(StringTable->insert(tags));

   mActionAnimationList.push_back(newActionDef);
}

void PlayerAnimationComponent::getGroundInfo(TSShapeInstance* si, TSThread* thread, ActionAnimationDef *dp)
{
  /* dp->death = !dStrnicmp(dp->name, "death", 5);
   if (dp->death)
   {
      // Death animations use roll frame-to-frame changes in ground transform into position
      dp->speed = 0.0f;
      dp->dir.set(0.0f, 0.0f, 0.0f);

      // Death animations MUST define ground transforms, so add dummy ones if required
      if (si->getShape()->sequences[dp->sequence].numGroundFrames == 0)
         si->getShape()->setSequenceGroundSpeed(dp->name, Point3F(0, 0, 0), Point3F(0, 0, 0));
   }
   else
   {*/
      VectorF save = dp->dir;
      si->setSequence(thread, dp->sequence, 0);
      si->animate();
      si->advanceTime(1);
      si->animateGround();
      si->getGroundTransform().getColumn(3, &dp->dir);
      if ((dp->speed = dp->dir.len()) < 0.01f)
      {
         // No ground displacement... In this case we'll use the
         // default table entry, if there is one.
         if (save.len() > 0.01f)
         {
            dp->dir = save;
            dp->speed = 1.0f;
            dp->velocityScale = false;
         }
         else
            dp->speed = 0.0f;
      }
      else
         dp->dir *= 1.0f / dp->speed;
   //}
}

void PlayerAnimationComponent::updateLookAnimation(F32 dt)
{
   // If the preference setting overrideLookAnimation is true, the player's
   // arm and head no longer animate according to the view direction. They
   // are instead given fixed positions.
   if (overrideLookAnimation)
   {
      if (mArmAnimation.thread)
         mOwnerShapeInstance->setPos(mArmAnimation.thread, armLookOverridePos);
      if (mHeadVThread)
         mOwnerShapeInstance->setPos(mHeadVThread, headVLookOverridePos);
      if (mHeadHThread)
         mOwnerShapeInstance->setPos(mHeadHThread, headHLookOverridePos);
      return;
   }
   // Calculate our interpolated head position.
   Point3F renderHead = head + headVec * dt;

   // Adjust look pos.  This assumes that the animations match
   // the min and max look angles provided in the datablock.
   if (mArmAnimation.thread)
   {
      /*if (mControlObject)
      {
         mOwnerShapeInstance->setPos(mArmAnimation.thread, 0.5f);
      }
      else
      {*/
         F32 d = mMaxLookAngle - mMinLookAngle;
         F32 tp = (renderHead.x - mMinLookAngle) / d;
         mOwnerShapeInstance->setPos(mArmAnimation.thread, mClampF(tp, 0, 1));
      //}
   }

   if (mHeadVThread)
   {
      F32 d = mMaxLookAngle - mMinLookAngle;
      F32 tp = (renderHead.x - mMinLookAngle) / d;
      mOwnerShapeInstance->setPos(mHeadVThread, mClampF(tp, 0, 1));
   }

   if (mHeadHThread)
   {
      F32 d = 2 * mMaxFreelookAngle;
      F32 tp = (renderHead.z + mMaxFreelookAngle) / d;
      mOwnerShapeInstance->setPos(mHeadHThread, mClampF(tp, 0, 1));
   }
}


//----------------------------------------------------------------------------
// Methods to get delta (as amount to affect velocity by)

/*bool PlayerAnimationComponent::inDeathAnim()
{
   if ((anim_clip_flags & ANIM_OVERRIDDEN) != 0 && (anim_clip_flags & IS_DEATH_ANIM) == 0)
      return false;
   if (mActionAnimation.thread && mActionAnimation.action >= 0)
      if (mActionAnimation.action < mActionList.size())
         return mActionAnimationList[mActionAnimation.action].death;

   return false;
}

// Get change from mLastDeathPos - return current pos.  Assumes we're in death anim.
F32 PlayerAnimationComponent::deathDelta(Point3F & delta)
{
   // Get ground delta from the last time we offset this.
   MatrixF  mat;
   F32 pos = mOwnerShapeInstance->getPos(mActionAnimation.thread);
   mOwnerShapeInstance->deltaGround1(mActionAnimation.thread, mDeath.lastPos, pos, mat);
   mat.getColumn(3, &delta);
   return pos;
}

// Called before updatePos() to prepare it's needed change to velocity, which
// must roll over.  Should be updated on tick, this is where we remember last
// position of animation that was used to roll into velocity.
void PlayerAnimationComponent::updateDeathOffsets()
{
   if (inDeathAnim())
      // Get ground delta from the last time we offset this.
      mDeath.lastPos = deathDelta(mDeath.posAdd);
   else
      mDeath.clear();
}*/

void PlayerAnimationComponent::updateAnimation(F32 dt)
{
   // update any active blend clips
   //if (isClientObject())
   //   for (S32 i = 0; i < blend_clips.size(); i++)
   //      mOwnerShapeInstance->advanceTime(dt, blend_clips[i].thread);

   // If we are the client's player on this machine, then we need
   // to make sure the transforms are up to date as they are used
   // to setup the camera.
   if (isClientObject())
   {
      updateAnimationTree(false);
   }
}

void PlayerAnimationComponent::updateAnimationTree(bool firstPerson)
{
   S32 mode = 0;
   if (firstPerson)
   {
      if (mActionAnimation.firstPerson)
         mode = 0;
      //            TSShapeInstance::MaskNodeRotation;
      //            TSShapeInstance::MaskNodePosX |
      //            TSShapeInstance::MaskNodePosY;
      else
         mode = TSShapeInstance::MaskNodeAllButBlend;
   }

   /*for (U32 i = 0; i < PlayerData::NumSpineNodes; i++)
      if (mDataBlock->spineNode[i] != -1)
         mOwnerShapeInstance->setNodeAnimationState(mDataBlock->spineNode[i], mode);*/
}

const String& PlayerAnimationComponent::getArmThread() const
{
   if (mArmAnimation.thread && mArmAnimation.thread->hasSequence())
   {
      return mArmAnimation.thread->getSequenceName();
   }

   return String::EmptyString;
}

bool PlayerAnimationComponent::setArmThread(const char* sequence)
{
   // The arm sequence must be in the action list.
   for (U32 i = 1; i < mActionAnimationList.size(); i++)
      if (!dStricmp(mActionAnimationList[i].name, sequence))
         return setArmThread(i);
   return false;
}

bool PlayerAnimationComponent::setArmThread(U32 action)
{
   ActionAnimationDef &anim = mActionAnimationList[action];
   if (anim.sequence != -1 &&
      anim.sequence != mOwnerShapeInstance->getSequence(mArmAnimation.thread))
   {
      mOwnerShapeInstance->setSequence(mArmAnimation.thread, anim.sequence, 0);
      mArmAnimation.action = action;
      setMaskBits(ThreadMaskN << 1);
      return true;
   }
   return false;
}


//----------------------------------------------------------------------------

bool PlayerAnimationComponent::setActionThread(const char* sequence, bool hold, bool wait, bool fsp)
{
   //if (anim_clip_flags & ANIM_OVERRIDDEN)
    //  return false;

   for (U32 i = 1; i < mActionAnimationList.size(); i++)
   {
      ActionAnimationDef &anim = mActionAnimationList[i];
      if (!dStricmp(anim.name, sequence))
      {
         setActionThread(i, true, hold, wait, fsp);
         setMaskBits(ThreadMaskN << 0);
         return true;
      }
   }
   return false;
}

void PlayerAnimationComponent::setActionThread(U32 action, bool forward, bool hold, bool wait, bool fsp, bool forceSet)
{
   if (!mActionAnimationList.size() || (mActionAnimation.action == action && mActionAnimation.forward == forward && !forceSet))
      return;

   if (action >= mActionAnimationList.size())
   {
      Con::errorf("PlayerAnimationComponent::setActionThread(%d): Player action out of range", action);
      return;
   }

   /*if (isClientObject())
   {
      mark_idle = (action == PlayerData::RootAnim);
      idle_timer = (mark_idle) ? 0.0f : -1.0f;
   }*/

   ActionAnimationDef &anim = mActionAnimationList[action];
   if (anim.sequence != -1)
   {
      U32 lastAction = mActionAnimation.action;

      mActionAnimation.action = action;
      mActionAnimation.forward = forward;
      mActionAnimation.firstPerson = fsp;
      mActionAnimation.holdAtEnd = hold;
      mActionAnimation.waitForEnd = hold ? true : wait;
      mActionAnimation.animateOnServer = fsp;
      mActionAnimation.atEnd = false;
      mActionAnimation.delayTicks = mNewAnimationTickTime;
      mActionAnimation.atEnd = false;

      if (mUseAnimationTransitions && /*(action != PlayerData::LandAnim || !(mDataBlock->landSequenceTime > 0.0f && !mDataBlock->transitionToLand)) &&*/ (isClientObject()/* || mActionAnimation.animateOnServer*/))
      {
         // The transition code needs the timeScale to be set in the
         // right direction to know which way to go.
         F32   transTime = mAnimationTransitionTime;
         //if (mDataBlock && mDataBlock->isJumpAction(action))
         //   transTime = 0.15f;

         F32 timeScale = mActionAnimation.forward ? 1.0f : -1.0f;
        // if (mDataBlock && mDataBlock->isJumpAction(action))
         //   timeScale *= 1.5f;

         mOwnerShapeInstance->setTimeScale(mActionAnimation.thread, timeScale);

         // If we're transitioning into the same sequence (an action may use the
         // same sequence as a previous action) then we want to start at the same
         // position.
         F32 pos = mActionAnimation.forward ? 0.0f : 1.0f;
         ActionAnimationDef &lastAnim = mActionAnimationList[lastAction];
         if (lastAnim.sequence == anim.sequence)
         {
            pos = mOwnerShapeInstance->getPos(mActionAnimation.thread);
         }

         mOwnerShapeInstance->transitionToSequence(mActionAnimation.thread, anim.sequence,
            pos, transTime, true);
      }
      else
      {
         mOwnerShapeInstance->setSequence(mActionAnimation.thread, anim.sequence,
            mActionAnimation.forward ? 0.0f : 1.0f);
      }
   }
}

void PlayerAnimationComponent::updateActionThread()
{
   PROFILE_START(PlayerAnimationComponent_UpdateActionThread);

   // Select an action animation sequence, this assumes that
   // this function is called once per tick.
   if (mActionAnimation.action != -1)
   {
      if (mActionAnimation.forward)
         mActionAnimation.atEnd = mOwnerShapeInstance->getPos(mActionAnimation.thread) == 1;
      else
         mActionAnimation.atEnd = mOwnerShapeInstance->getPos(mActionAnimation.thread) == 0;
   }

   // Only need to deal with triggers on the client
   /*if (isClientObject())
   {
      bool triggeredLeft = false;
      bool triggeredRight = false;

      F32 offset = 0.0f;
      if (mOwnerShapeInstance->getTriggerState(1))
      {
         triggeredLeft = true;
         offset = -mDataBlock->decalOffset * getScale().x;
      }
      else if (mOwnerShapeInstance->getTriggerState(2))
      {
         triggeredRight = true;
         offset = mDataBlock->decalOffset * getScale().x;
      }

      process_client_triggers(triggeredLeft, triggeredRight);
      if ((triggeredLeft || triggeredRight) && !noFootfallFX)
      {
         Point3F rot, pos;
         RayInfo rInfo;
         MatrixF mat = getRenderTransform();
         mat.getColumn(1, &rot);
         mat.mulP(Point3F(offset, 0.0f, 0.0f), &pos);

         if (gClientContainer.castRay(Point3F(pos.x, pos.y, pos.z + 0.01f),
            Point3F(pos.x, pos.y, pos.z - 2.0f),
            STATIC_COLLISION_TYPEMASK | VehicleObjectType, &rInfo))
         {
            Material* material = (rInfo.material ? dynamic_cast< Material* >(rInfo.material->getMaterial()) : 0);

            // Put footprints on surface, if appropriate for material.

            if (material && material->mShowFootprints
               && mDataBlock->decalData && !footfallDecalOverride)
            {
               Point3F normal;
               Point3F tangent;
               mObjToWorld.getColumn(0, &tangent);
               mObjToWorld.getColumn(2, &normal);
               gDecalManager->addDecal(rInfo.point, normal, tangent, mDataBlock->decalData, getScale().y);
            }

            // Emit footpuffs.

            if (!footfallDustOverride && rInfo.t <= 0.5f && mWaterCoverage == 0.0f
               && material && material->mShowDust)
            {
               // New emitter every time for visibility reasons
               ParticleEmitter * emitter = new ParticleEmitter;
               emitter->onNewDataBlock(mDataBlock->footPuffEmitter, false);

               LinearColorF colorList[ParticleData::PDC_NUM_KEYS];

               for (U32 x = 0; x < getMin(Material::NUM_EFFECT_COLOR_STAGES, ParticleData::PDC_NUM_KEYS); ++x)
                  colorList[x].set(material->mEffectColor[x].red,
                     material->mEffectColor[x].green,
                     material->mEffectColor[x].blue,
                     material->mEffectColor[x].alpha);
               for (U32 x = Material::NUM_EFFECT_COLOR_STAGES; x < ParticleData::PDC_NUM_KEYS; ++x)
                  colorList[x].set(1.0, 1.0, 1.0, 0.0);

               emitter->setColors(colorList);
               if (!emitter->registerObject())
               {
                  Con::warnf(ConsoleLogEntry::General, "Could not register emitter for particle of class: %s", mDataBlock->getName());
                  delete emitter;
                  emitter = NULL;
               }
               else
               {
                  emitter->emitParticles(pos, Point3F(0.0, 0.0, 1.0), mDataBlock->footPuffRadius,
                     Point3F(0, 0, 0), mDataBlock->footPuffNumParts);
                  emitter->deleteWhenEmpty();
               }
            }

            // Play footstep sound.

            if (footfallSoundOverride <= 0)
               playFootstepSound(triggeredLeft, material, rInfo.object);
         }
      }
   }*/

   // Mount pending variable puts a hold on the delayTicks below so players don't
   // inadvertently stand up because their mount has not come over yet.
   //if (mMountPending)
   //   mMountPending = (mOwner->isMounted() ? 0 : (mMountPending - 1));

   if ((mActionAnimation.action == -1) ||
      ((!mActionAnimation.waitForEnd || mActionAnimation.atEnd) /*&&
      (!mActionAnimation.holdAtEnd && (mActionAnimation.delayTicks -= !mMountPending) <= 0)*/))
   {
      //The scripting language will get a call back when a script animation has finished...
      //  example: When the chat menu animations are done playing...
      if (isServerObject()/* && mActionAnimation.action >= PlayerData::NumTableActionAnims*/)
      {
         Con::executef(this, "onAnimationEnd", mActionAnimation.thread->getSequenceName());
      }
      pickActionAnimation();
   }

   PROFILE_END();
}

void PlayerAnimationComponent::pickBestMoveAction(U32 startAnim, U32 endAnim, U32 * action, bool * forward) const
{
   *action = startAnim;
   *forward = false;

   VectorF vel;
   mOwner->getWorldToObj().mulV(mVelocity, &vel);

   if (vel.lenSquared() > 0.01f)
   {
      // Bias the velocity towards picking the forward/backward anims over
      // the sideways ones to prevent oscillation between anims.
      vel *= VectorF(0.5f, 1.0f, 0.5f);

      // Pick animation that is the best fit for our current (local) velocity.
      // Assumes that the root (stationary) animation is at startAnim.
      F32 curMax = -0.1f;
      for (U32 i = startAnim + 1; i <= endAnim; i++)
      {
         const ActionAnimationDef &anim = mActionAnimationList[i];
         if (anim.sequence != -1 && anim.speed)
         {
            F32 d = mDot(vel, anim.dir);
            if (d > curMax)
            {
               curMax = d;
               *action = i;
               *forward = true;
            }
            else
            {
               // Check if reversing this animation would fit (bias against this
               // so that when moving right, the real right anim is still chosen,
               // but if not present, the reversed left anim will be used instead)
               d *= -0.75f;
               if (d > curMax)
               {
                  curMax = d;
                  *action = i;
                  *forward = false;
               }
            }
         }
      }
   }
}

void PlayerAnimationComponent::pickActionAnimation()
{
   /*if (isMounted() || mMountPending)
   {
      // Go into root position unless something was set explicitly
      // from a script.
      if (mActionAnimation.action != PlayerData::RootAnim &&
         mActionAnimation.action < PlayerData::NumTableActionAnims)
         setActionThread(PlayerData::RootAnim, true, false, false);
      return;
   }

   bool forward = true;
   U32 action = PlayerData::RootAnim;
   bool fsp = false;

   // Jetting overrides the fall animation condition
   if (mJetting)
   {
      // Play the jetting animation
      action = PlayerData::JetAnim;
   }
   else if (mFalling)
   {
      // Not in contact with any surface and falling
      action = PlayerData::FallAnim;
   }
   else if (mSwimming)
   {
      pickBestMoveAction(PlayerData::SwimRootAnim, PlayerData::SwimRightAnim, &action, &forward);
   }
   else if (mPose == StandPose)
   {
      if (mContactTimer >= sContactTickTime)
      {
         // Nothing under our feet
         action = PlayerData::RootAnim;
      }
      else
      {
         // Our feet are on something
         pickBestMoveAction(PlayerData::RootAnim, PlayerData::SideRightAnim, &action, &forward);
      }
   }
   else if (mPose == CrouchPose)
   {
      pickBestMoveAction(PlayerData::CrouchRootAnim, PlayerData::CrouchRightAnim, &action, &forward);
   }
   else if (mPose == PronePose)
   {
      pickBestMoveAction(PlayerData::ProneRootAnim, PlayerData::ProneBackwardAnim, &action, &forward);
   }
   else if (mPose == SprintPose)
   {
      pickBestMoveAction(PlayerData::SprintRootAnim, PlayerData::SprintRightAnim, &action, &forward);
   }*/

   bool forward = true;
   U32 action = 0;
   bool fsp = false;

   //pickBestMoveAction(PlayerData::SprintRootAnim, PlayerData::SprintRightAnim, &action, &forward);

   setActionThread(action, forward, false, false, fsp);
}

DefineEngineMethod(PlayerAnimationComponent, addAction, void, (String animName, VectorF direction, String tags), ("", VectorF::Zero, ""),
   "")
{
   return object->addAction(animName, direction, tags);
}
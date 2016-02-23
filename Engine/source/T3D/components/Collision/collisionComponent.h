//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef COLLISION_COMPONENT_H
#define COLLISION_COMPONENT_H

#ifndef __RESOURCE_H__
#include "core/resource.h"
#endif
#ifndef _TSSHAPE_H_
#include "ts/tsShape.h"
#endif
#ifndef _SCENERENDERSTATE_H_
#include "scene/sceneRenderState.h"
#endif
#ifndef _MBOX_H_
#include "math/mBox.h"
#endif
#ifndef _Entity_H_
#include "T3D/Entity.h"
#endif

#ifndef _CORE_INTERFACES_H_
#include "T3D/Components/coreInterfaces.h"
#endif

#ifndef _COLLISION_INTERFACES_H_
#include "T3D/Components/collision/collisionInterfaces.h"
#endif
#ifndef _RENDER_COMPONENT_INTERFACE_H_
#include "T3D/Components/render/renderComponentInterface.h"
#endif
#ifndef _PHYSICS_COMPONENT_INTERFACE_H_
#include "T3D/Components/physics/physicsComponentInterface.h"
#endif

#ifndef _T3D_PHYSICSCOMMON_H_
#include "T3D/physics/physicsCommon.h"
#endif
#ifndef _T3D_PHYSICS_PHYSICSWORLD_H_
#include "T3D/physics/physicsWorld.h"
#endif

class TSShapeInstance;
class SceneRenderState;
class CollisionComponent;
class PhysicsBody;
class PhysicsWorld;

class CollisionComponent : public Component,
   public CollisionInterface,
   public CastRayInterface
{
   typedef Component Parent;
public:
   enum MeshType
   {
      None = 0,            ///< No mesh
      Bounds = 1,          ///< Bounding box of the shape
      CollisionMesh = 2,   ///< Specifically designated collision meshes
      VisibleMesh = 3      ///< Rendered mesh polygons
   };

   PhysicsWorld* mPhysicsWorld;
   PhysicsBody* mPhysicsRep;

protected:
   MeshType mCollisionType;
   MeshType mDecalType;
   MeshType mLOSType;

   Vector<S32> mCollisionDetails;
   Vector<S32> mLOSDetails;

   StringTableEntry colisionMeshPrefix;

   RenderComponentInterface* mOwnerRenderInterface;

   PhysicsComponentInterface* mOwnerPhysicsInterface;

   //only really relevent for the collision mesh type
   //if we note an animation component is added, we flag as being animated.
   //This way, if we're using collision meshes, we can set it up to update their transforms
   //as needed
   bool mAnimated;

   enum
   {
      ColliderMask = Parent::NextFreeMask,
   };

public:
   CollisionComponent();
   virtual ~CollisionComponent();
   DECLARE_CONOBJECT(CollisionComponent);

   virtual U32 packUpdate(NetConnection *con, U32 mask, BitStream *stream);
   virtual void unpackUpdate(NetConnection *con, BitStream *stream);

   virtual void componentAddedToOwner(Component *comp);
   virtual void componentRemovedFromOwner(Component *comp);
   virtual void ownerTransformSet(MatrixF *mat);
   void targetShapeChanged(RenderComponentInterface* instanceInterface);

   virtual void onComponentRemove();
   virtual void onComponentAdd();

   virtual void checkDependencies();

   static void initPersistFields();

   void inspectPostApply();

   virtual void processTick();

   void prepCollision();

   PhysicsCollision* buildColShapes();

   void updatePhysics();

   virtual bool castRay(const Point3F &start, const Point3F &end, RayInfo* info);

   virtual bool buildPolyList(PolyListContext context, AbstractPolyList* polyList, const Box3F &box, const SphereF &sphere){ return false; }

   virtual PhysicsCollision* getCollisionData();

   //Utility functions, mostly for script
   Point3F getContactNormal() { return mContactInfo.contactNormal; }
   bool hasContact()
   {
      if (mContactInfo.contactObject)
         return true;
      else
         return false;
   }
   S32 getCollisionCount()
   {
      return mCollisionList.getCount();
   }

   Point3F getCollisionNormal(S32 collisionIndex)
   {
      if (collisionIndex < 0 || mCollisionList.getCount() < collisionIndex)
         return Point3F::Zero;

      return mCollisionList[collisionIndex].normal;
   }

   F32 getCollisionAngle(S32 collisionIndex, Point3F upVector)
   {
      if (collisionIndex < 0 || mCollisionList.getCount() < collisionIndex)
         return 0.0f;

      return mRadToDeg(mAcos(mDot(mCollisionList[collisionIndex].normal, upVector)));
   }

   S32 getBestCollision(Point3F upVector)
   {
      S32 bestCollision = -1;

      F32 bestAngle = 360.f;
      S32 count = mCollisionList.getCount();
      for (U32 i = 0; i < count; ++i)
      {
         F32 angle = mRadToDeg(mAcos(mDot(mCollisionList[i].normal, upVector)));

         if (angle < bestAngle)
         {
            bestCollision = i;
            bestAngle = angle;
         }
      }

      return bestCollision;
   }

   F32 getBestCollisionAngle(VectorF upVector)
   {
      S32 bestCol = getBestCollision(upVector);

      if (bestCol == -1)
         return 0;

      return getCollisionAngle(bestCol, upVector);
   }
};

typedef CollisionComponent::MeshType CollisionMeshMeshType;
DefineEnumType(CollisionMeshMeshType);

#endif // _COMPONENT_H_

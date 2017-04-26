//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------
#ifndef RENDER_PROBE_MGR_H
#define RENDER_PROBE_MGR_H

#ifndef _RENDERBINMANAGER_H_
#include "renderInstance/renderBinManager.h"
#endif
#ifndef _MATINSTANCE_H_
#include "materials/matInstance.h"
#endif
#ifndef _MATTEXTURETARGET_H_
#include "materials/matTextureTarget.h"
#endif
#ifndef _GFXPRIMITIVEBUFFER_H_
#include "gfx/gfxPrimitiveBuffer.h"
#endif
#ifndef _GFXVERTEXBUFFER_H_
#include "gfx/gfxVertexBuffer.h"
#endif

class ReflectProbeMatInstance : public MatInstance
{
   typedef MatInstance Parent;
protected:
   MaterialParameterHandle *mProbeParamsSC;
   bool mInternalPass;

   GFXStateBlockRef mStateBlock;

public:
   ReflectProbeMatInstance(Material &mat) : Parent(mat), mProbeParamsSC(NULL), mInternalPass(false) {}

   virtual bool init(const FeatureSet &features, const GFXVertexFormat *vertexFormat);
   virtual bool setupPass(SceneRenderState *state, const SceneData &sgData);
};

//**************************************************************************
// RenderObjectMgr
//**************************************************************************
class RenderProbeMgr : public RenderBinManager
{
   typedef RenderBinManager Parent;
public:
   typedef GFXVertexPNTT FarFrustumQuadVert;

protected:
   struct ReflectProbeMaterialInfo
   {
      ReflectProbeMatInstance *matInstance;

      // { zNear, zFar, 1/zNear, 1/zFar }
      MaterialParameterHandle *zNearFarInvNearFar;

      // Far frustum plane (World Space)
      MaterialParameterHandle *farPlane;

      // Far frustum plane (View Space)
      MaterialParameterHandle *vsFarPlane;

      // -dot( farPlane, eyePos )
      MaterialParameterHandle *negFarPlaneDotEye;

      // Inverse View matrix
      MaterialParameterHandle *invViewMat;

      // Light Parameters
      MaterialParameterHandle *lightPosition;
      MaterialParameterHandle *lightDirection;
      MaterialParameterHandle *lightColor;
      MaterialParameterHandle *lightBrightness;
      MaterialParameterHandle *lightAttenuation;
      MaterialParameterHandle *lightRange;
      MaterialParameterHandle *lightAmbient;
      MaterialParameterHandle *lightTrilight;
      MaterialParameterHandle *lightSpotParams;

      MaterialParameterHandle *ambientColor;

      MaterialParameterHandle *skyColor;
      MaterialParameterHandle *groundColor;
      MaterialParameterHandle *intensity;

      MaterialParameterHandle *useCubemap;
      MaterialParameterHandle *cubemap;

      MaterialParameterHandle *eyePosWorld;
      MaterialParameterHandle *volumeStart;
      MaterialParameterHandle *volumeSize;
      MaterialParameterHandle *volumePosition;

      ReflectProbeMaterialInfo(const String &matName, const GFXVertexFormat *vertexFormat);

      virtual ~ReflectProbeMaterialInfo();


      void setViewParameters(const F32 zNear,
         const F32 zFar,
         const Point3F &eyePos,
         const PlaneF &farPlane,
         const PlaneF &_vsFarPlane,
         const MatrixF &_inverseViewMatrix);

      void setProbeParameters(const ProbeRenderInst *probe, const SceneRenderState* renderState, const MatrixF &worldViewOnly);
   };

   GFXVertexBufferHandle<FarFrustumQuadVert> mFarFrustumQuadVerts;

   GFXVertexBufferHandle<GFXVertexPC> getSphereMesh(U32 &outNumPrimitives, GFXPrimitiveBufferHandle &outPrimitives);

   // Convex geometry for lights
   GFXVertexBufferHandle<GFXVertexPC> mSphereGeometry;

   GFXPrimitiveBufferHandle mSphereIndices;

   U32 mSpherePrimitiveCount;

public:
   RenderProbeMgr();
   RenderProbeMgr(RenderInstType riType, F32 renderOrder, F32 processAddOrder);

   // RenderBinMgr
   virtual void addElement(RenderInst *inst);
   virtual void render(SceneRenderState * state);

   // ConsoleObject
   static void initPersistFields();
   DECLARE_CONOBJECT(RenderProbeMgr);

   ReflectProbeMaterialInfo* mReflectProbeMaterial;
   ReflectProbeMaterialInfo* mReflectProbeDiffuseMaterial;
   ReflectProbeMaterialInfo* _getReflectProbeMaterial();
   ReflectProbeMaterialInfo* _getReflectProbeDiffuseMaterial();

   // Add a reflection probe to the bin
   void addSphereReflectionProbe(ProbeRenderInst *probeInfo);
   void addConvexReflectionProbe(ProbeRenderInst *probeInfo);
};

#endif // RENDER_PROBE_MGR_H
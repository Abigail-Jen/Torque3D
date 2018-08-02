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

#ifndef SKYLIGHT_H
#define SKYLIGHT_H

#ifndef _SCENEOBJECT_H_
#include "scene/sceneObject.h"
#endif
#ifndef _GFXVERTEXBUFFER_H_
#include "gfx/gfxVertexBuffer.h"
#endif
#ifndef _GFXPRIMITIVEBUFFER_H_
#include "gfx/gfxPrimitiveBuffer.h"
#endif
#ifndef _TSSHAPEINSTANCE_H_
#include "ts/tsShapeInstance.h"
#endif
#include "lighting/lightInfo.h"

#ifndef _RENDERPASSMANAGER_H_
#include "renderInstance/renderPassManager.h"
#endif

#ifndef PROBEMANAGER_H
#include "lighting/probeManager.h"
#endif

class BaseMatInstance;


//-----------------------------------------------------------------------------
// This class implements a basic SceneObject that can exist in the world at a
// 3D position and render itself. There are several valid ways to render an
// object in Torque. This class implements the preferred rendering method which
// is to submit a MeshRenderInst along with a Material, vertex buffer,
// primitive buffer, and transform and allow the RenderMeshMgr handle the
// actual setup and rendering for you.
//-----------------------------------------------------------------------------

class Skylight : public SceneObject
{
   typedef SceneObject Parent;

public:

   enum IndrectLightingModeType
   {
      NoIndirect = 0,            
      AmbientColor = 1, 
      SphericalHarmonics = 2
   };

   enum ReflectionModeType
   {
      StaticCubemap = 1,
      BakedCubemap = 2
   };

private:

   // Networking masks
   // We need to implement a mask specifically to handle
   // updating our transform from the server object to its
   // client-side "ghost". We also need to implement a
   // maks for handling editor updates to our properties
   // (like material).
   enum MaskBits
   {
      TransformMask = Parent::NextFreeMask << 0,
      UpdateMask = Parent::NextFreeMask << 1,
      EnabledMask = Parent::NextFreeMask << 2,
      CubemapMask = Parent::NextFreeMask << 3,
      ModeMask = Parent::NextFreeMask << 4,
      RadiusMask = Parent::NextFreeMask << 5,
      ShapeTypeMask = Parent::NextFreeMask << 6,
      BakeInfoMask = Parent::NextFreeMask << 7,
      NextFreeMask = Parent::NextFreeMask << 8
   };

   bool mBake;
   bool mEnabled;
   bool mDirty;

   Resource<TSShape> mEditorShape;
   TSShapeInstance* mEditorShapeInst;

   //--------------------------------------------------------------------------
   // Rendering variables
   //--------------------------------------------------------------------------
   ProbeInfo* mProbeInfo;

   //Reflection Contribution stuff
   ReflectionModeType mReflectionModeType;

   String mCubemapName;
   CubemapData *mCubemap;

   String mReflectionPath;
   String mProbeUniqueID;

   //Debug rendering
   static bool smRenderSkylights;
   static bool smRenderPreviewProbes;

   //irridiance resources
   GFXCubemapHandle mIrridianceMap;

   //prefilter resources
   GFXCubemapHandle mPrefilterMap;
   U32 mPrefilterMipLevels;
   U32 mPrefilterSize;

   //brdflookup resources - shares the texture target with the prefilter
   GFXTexHandle mBrdfTexture;

   bool mResourcesCreated;

public:
   Skylight();
   virtual ~Skylight();

   // Declare this object as a ConsoleObject so that we can
   // instantiate it into the world and network it
   DECLARE_CONOBJECT(Skylight);

   //--------------------------------------------------------------------------
   // Object Editing
   // Since there is always a server and a client object in Torque and we
   // actually edit the server object we need to implement some basic
   // networking functions
   //--------------------------------------------------------------------------
   // Set up any fields that we want to be editable (like position)
   static void initPersistFields();

   // Allows the object to update its editable settings
   // from the server object to the client
   virtual void inspectPostApply();

   static bool _setEnabled(void *object, const char *index, const char *data);
   static bool _doBake(void *object, const char *index, const char *data);

   // Handle when we are added to the scene and removed from the scene
   bool onAdd();
   void onRemove();

   // Override this so that we can dirty the network flag when it is called
   void setTransform(const MatrixF &mat);

   // This function handles sending the relevant data from the server
   // object to the client object
   U32 packUpdate(NetConnection *conn, U32 mask, BitStream *stream);
   // This function handles receiving relevant data from the server
   // object and applying it to the client object
   void unpackUpdate(NetConnection *conn, BitStream *stream);

   //--------------------------------------------------------------------------
   // Object Rendering
   // Torque utilizes a "batch" rendering system. This means that it builds a
   // list of objects that need to render (via RenderInst's) and then renders
   // them all in one batch. This allows it to optimized on things like
   // minimizing texture, state, and shader switching by grouping objects that
   // use the same Materials.
   //--------------------------------------------------------------------------

   // Create the geometry for rendering
   void createGeometry();

   bool createClientResources();

   // Get the Material instance
   void updateMaterial();

   void generateTextures();

   void updateProbeParams();

   // This is the function that allows this object to submit itself for rendering
   void prepRenderImage(SceneRenderState *state);

   void setPreviewMatParameters(SceneRenderState* renderState, BaseMatInstance* mat);

    //Baking
   void bake(String outputPath, S32 resolution);
};

typedef Skylight::IndrectLightingModeType SkylightIndrectLightingModeEnum;
DefineEnumType(SkylightIndrectLightingModeEnum);

typedef Skylight::ReflectionModeType SkylightReflectionModeEnum;
DefineEnumType(SkylightReflectionModeEnum);

#endif // _Skylight_H_
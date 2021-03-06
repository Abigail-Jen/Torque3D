//-----------------------------------------------------------------------------
// Copyright (c) 2013 GarageGames, LLC
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

#ifndef IMAGE_ASSET_H
#include "ImageAsset.h"
#endif

#ifndef _ASSET_MANAGER_H_
#include "assets/assetManager.h"
#endif

#ifndef _CONSOLETYPES_H_
#include "console/consoleTypes.h"
#endif

#ifndef _TAML_
#include "persistence/taml/taml.h"
#endif

#ifndef _ASSET_PTR_H_
#include "assets/assetPtr.h"
#endif

// Debug Profiling.
#include "platform/profiler.h"

//-----------------------------------------------------------------------------

IMPLEMENT_CONOBJECT(ImageAsset);

ConsoleType(ImageAssetPtr, TypeImageAssetPtr, String, ASSET_ID_FIELD_PREFIX)

//-----------------------------------------------------------------------------

ConsoleGetType(TypeImageAssetPtr)
{
   // Fetch asset Id.
   return *((StringTableEntry*)dptr);
}

//-----------------------------------------------------------------------------

ConsoleSetType(TypeImageAssetPtr)
{
   // Was a single argument specified?
   if (argc == 1)
   {
      // Yes, so fetch field value.
      const char* pFieldValue = argv[0];

      // Fetch asset Id.
      StringTableEntry* assetId = (StringTableEntry*)(dptr);

      // Update asset value.
      *assetId = StringTable->insert(pFieldValue);

      return;
   }

   // Warn.
   Con::warnf("(TypeImageAssetPtr) - Cannot set multiple args to a single asset.");
}

//-----------------------------------------------------------------------------

ImplementEnumType(ImageAssetType,
   "Type of mesh data available in a shape.\n"
   "@ingroup gameObjects")
   { ImageAsset::Albedo,      "Albedo",      "" },
   { ImageAsset::Normal,      "Normal",      "" },
   { ImageAsset::Composite,   "Composite",   "" },
   { ImageAsset::GUI,         "GUI",         "" },
   { ImageAsset::Roughness,   "Roughness",   "" },
   { ImageAsset::AO,          "AO",          "" },
   { ImageAsset::Metalness,   "Metalness",   "" },
   { ImageAsset::Glow,        "Glow",        "" },
   { ImageAsset::Particle,    "Particle",    "" },
   { ImageAsset::Decal,       "Decal",       "" },

EndImplementEnumType;


//-----------------------------------------------------------------------------
ImageAsset::ImageAsset() : AssetBase(), mImage(nullptr), mUseMips(true), mIsHDRImage(false), mIsValidImage(false)
{
   mImageFileName = StringTable->EmptyString();
}

//-----------------------------------------------------------------------------

ImageAsset::~ImageAsset()
{
}

//-----------------------------------------------------------------------------

void ImageAsset::initPersistFields()
{
   // Call parent.
   Parent::initPersistFields();

   addProtectedField("imageFile", TypeAssetLooseFilePath, Offset(mImageFileName, ImageAsset),
      &setImageFileName, &getImageFileName, "Path to the image file.");

   addField("useMips", TypeBool, Offset(mUseMips, ImageAsset), "Should the image use mips? (Currently unused).");
   addField("isHDRImage", TypeBool, Offset(mIsHDRImage, ImageAsset), "Is the image in an HDR format? (Currently unused)");

   addField("imageType", TypeImageAssetType, Offset(mImageType, ImageAsset), "What the main use-case for the image is for.");
}

//------------------------------------------------------------------------------

void ImageAsset::copyTo(SimObject* object)
{
   // Call to parent.
   Parent::copyTo(object);
}

void ImageAsset::loadImage()
{
   SAFE_DELETE(mImage);

   if (mImageFileName)
   {
      if (!Platform::isFile(mImageFileName))
      {
         Con::errorf("ImageAsset::initializeAsset: Attempted to load file %s but it was not valid!", mImageFileName);
         return;
      }

      mImage.set(mImageFileName, &GFXStaticTextureSRGBProfile, avar("%s() - mImage (line %d)", __FUNCTION__, __LINE__));

      if (mImage)
      {
         mIsValidImage = true;
         return;
      }
   }

   mIsValidImage = false;
}

void ImageAsset::initializeAsset()
{
   mImageFileName = expandAssetFilePath(mImageFileName);

   loadImage();
}

void ImageAsset::onAssetRefresh()
{
   setImageFileName(mImageFileName);
}

void ImageAsset::setImageFileName(const char* pScriptFile)
{
   // Sanity!
   AssertFatal(pScriptFile != NULL, "Cannot use a NULL image file.");

   // Update.
   mImageFileName = StringTable->insert(pScriptFile);
}

DefineEngineMethod(ImageAsset, getImageFilename, const char*, (), ,
   "Creates an instance of the given GameObject given the asset definition.\n"
   "@return The GameObject entity created from the asset.")
{
   return object->getImageFileName();
}

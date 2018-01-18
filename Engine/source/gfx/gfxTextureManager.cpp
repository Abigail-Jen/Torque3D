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

#include "platform/platform.h"
#include "gfx/gfxTextureManager.h"

#include "gfx/gfxDevice.h"
#include "gfx/gfxCardProfile.h"
#include "gfx/gfxStringEnumTranslate.h"
#include "gfx/bitmap/imageUtils.h"
#include "core/strings/stringFunctions.h"
#include "core/util/safeDelete.h"
#include "core/resourceManager.h"
#include "core/volume.h"
#include "core/util/dxt5nmSwizzle.h"
#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "materials/matTextureTarget.h"
#include "platform/threads/threadPool.h"

using namespace Torque;

//#define DEBUG_SPEW


S32 GFXTextureManager::smTextureReductionLevel = 0;

String GFXTextureManager::smMissingTexturePath(Con::getVariable("$Core::MissingTexturePath"));
String GFXTextureManager::smUnavailableTexturePath(Con::getVariable("$Core::UnAvailableTexturePath"));
String GFXTextureManager::smWarningTexturePath(Con::getVariable("$Core::WarningTexturePath"));

GFXTextureManager::EventSignal GFXTextureManager::smEventSignal;

static const String  sDDSExt( "dds" );

void GFXTextureManager::init()
{
   Con::addVariable( "$pref::Video::textureReductionLevel", TypeS32, &smTextureReductionLevel,
      "The number of mipmap levels to drop on loaded textures to reduce "
      "video memory usage.  It will skip any textures that have been defined "
      "as not allowing down scaling.\n"
      "@ingroup GFX\n" );

   Con::addVariable( "$pref::Video::missingTexturePath", TypeRealString, &smMissingTexturePath,
      "The file path of the texture to display when the requested texture is missing.\n"
      "@ingroup GFX\n" );

   Con::addVariable( "$pref::Video::unavailableTexturePath", TypeRealString, &smUnavailableTexturePath,
      "@brief The file path of the texture to display when the requested texture is unavailable.\n\n"
      "Often this texture is used by GUI controls to indicate that the request image is unavailable.\n"
      "@ingroup GFX\n" );

   Con::addVariable( "$pref::Video::warningTexturePath", TypeRealString, &smWarningTexturePath,
      "The file path of the texture used to warn the developer.\n"
      "@ingroup GFX\n" );
}

GFXTextureManager::GFXTextureManager()
	: mMutex("GFXTextureManager::mMutex")
{
   mListHead = mListTail = NULL;
   mTextureManagerState = GFXTextureManager::Living;

   // Set up the hash table
   mHashCount = 1023;
   mHashTable = new GFXTextureObject *[mHashCount];
   for(U32 i = 0; i < mHashCount; i++)
      mHashTable[i] = NULL;
}

void GFXTextureManager::preDestroy()
{
	NamedTexTarget::sCleanUp();
	kill();
	cleanupPool();
	MutexHandle mutexHandle = TORQUE_LOCK(mMutex);
	if (mHashTable)
	{
		SAFE_DELETE_ARRAY(mHashTable);
	}
	mTexturePool.clear();
}

GFXTextureManager::~GFXTextureManager()
{
	const auto ts = mTexturePool.size();
	const auto cs = mCubemapTable.size();
	AssertFatal(ts == 0 && cs == 0, "Some textures are not cleaned up!");
}


U32 GFXTextureManager::getTextureDownscalePower( GFXTextureProfile *profile )
{
   if ( !profile || profile->canDownscale() )
      return smTextureReductionLevel;

   return 0;
}

bool GFXTextureManager::validateTextureQuality( GFXTextureProfile *profile, U32 &width, U32 &height )
{
   U32 scaleFactor = getTextureDownscalePower( profile );
   if ( scaleFactor == 0 )
      return true;

   // Otherwise apply the appropriate scale...
   width  >>= scaleFactor;
   height >>= scaleFactor;

   return true;
}

void GFXTextureManager::kill()
{
   AssertFatal( mTextureManagerState != GFXTextureManager::Dead, "Texture Manager already killed!" );

   // Release everything in the cache we can
   // so we don't leak any textures.
   cleanupCache();

   MutexHandle mutexHandle = TORQUE_LOCK(mMutex);

   GFXTextureObject *curr = mListHead;
   GFXTextureObject *temp;

   // Actually delete all the textures we know about.
   while( curr != NULL ) 
   {
      temp = curr->mNext;
      curr->kill();
      curr = temp;
   }

   mCubemapTable.clear();

   mTextureManagerState = GFXTextureManager::Dead;
}

void GFXTextureManager::zombify()
{
   AssertFatal( mTextureManagerState != GFXTextureManager::Zombie, "Texture Manager already a zombie!" );

   // Notify everyone that cares about the zombification!
   smEventSignal.trigger( GFXZombify );

   // Release unused pool textures.
   cleanupPool();

   // Release everything in the cache we can.
   cleanupCache();

   // Free all the device copies of the textures.
   GFXTextureObject *temp = mListHead;
   while( temp != NULL ) 
   {
      freeTexture( temp, true );
      temp = temp->mNext;
   }

   // Finally, note our state.
   mTextureManagerState = GFXTextureManager::Zombie;
}

void GFXTextureManager::resurrect()
{
	MutexHandle mutexHandle = TORQUE_LOCK(mMutex);
	MutexHandle gfxHandle = TORQUE_LOCK(GFX->mMutex);

   // Reupload all the device copies of the textures.
   GFXTextureObject *temp = mListHead;

   while( temp != NULL ) 
   {
      refreshTexture( temp );
      temp = temp->mNext;
   }
   mutexHandle.unlock();

   // Notify callback registries.
   smEventSignal.trigger( GFXResurrect );
   
   // Update our state.
   mTextureManagerState = GFXTextureManager::Living;
}

void GFXTextureManager::cleanupPool()
{
   PROFILE_SCOPE( GFXTextureManager_CleanupPool );
	mTexturePool.clear();
}

void GFXTextureManager::requestDeleteTexture( GFXTextureObject *texture )
{
   // If this is a non-cached texture then just really delete it.
   if ( texture->mTextureLookupName.isEmpty() )
   {
      delete texture;
      return;
   }

   // Set the time and store it.
   texture->mDeleteTime = Platform::getTime();
   mToDelete.push_back_unique( texture );
}

void GFXTextureManager::cleanupCache( U32 secondsToLive )
{
}

GFXTextureObject *GFXTextureManager::_lookupTexture( const char *hashName, const GFXTextureProfile *profile  )
{
   GFXTextureObject *ret = hashFind( hashName );

   //compare just the profile flags and not the entire profile, names could be different but otherwise identical flags
   if (ret && (ret->mProfile->compareFlags(*profile)))
      return ret;
   else if (ret)
      Con::warnf("GFXTextureManager::_lookupTexture: Cached texture %s has different profile flags: (%s,%s) ", hashName, ret->mProfile->getName().c_str(), profile->getName().c_str());

   return NULL;
}

GFXTextureObject *GFXTextureManager::_lookupTexture( const DDSFile *ddsFile, const GFXTextureProfile *profile )
{
   if( ddsFile->getTextureCacheString().isNotEmpty() )
   {
      // Call _lookupTexture()
      return _lookupTexture( ddsFile->getTextureCacheString(), profile );
   }

   return NULL;
}

GFXTextureObject *GFXTextureManager::createTexture( GBitmap *bmp, const String &resourceName, GFXTextureProfile *profile, bool deleteBmp )
{
   AssertFatal(bmp, "GFXTextureManager::createTexture() - Got NULL bitmap!");

   GFXTextureObject *cacheHit = _lookupTexture( resourceName, profile );
   if( cacheHit != NULL)
   {
      // Con::errorf("Cached texture '%s'", (resourceName.isNotEmpty() ? resourceName.c_str() : "unknown"));
      if (deleteBmp)
         delete bmp;
      return cacheHit;
   }

   return _createTexture( bmp, resourceName, profile, deleteBmp, NULL );
}

GFXTextureObject *GFXTextureManager::_createTexture(  GBitmap *bmp, 
                                                      const String &resourceName, 
                                                      GFXTextureProfile *profile, 
                                                      bool deleteBmp,
                                                      GFXTextureObject *inObj )
{
   PROFILE_SCOPE( GFXTextureManager_CreateTexture_Bitmap );
   
   #ifdef DEBUG_SPEW
   Platform::outputDebugString( "[GFXTextureManager] _createTexture (GBitmap) '%s'",
      resourceName.c_str()
   );
   #endif

   // Massage the bitmap based on any resize rules.
   U32 scalePower = getTextureDownscalePower( profile );

   GBitmap *realBmp = bmp;
   U32 realWidth = bmp->getWidth();
   U32 realHeight = bmp->getHeight();

   if (  scalePower && 
         isPow2(bmp->getWidth()) && 
         isPow2(bmp->getHeight()) && 
         profile->canDownscale() )
   {
      // We only work with power of 2 textures for now, so we 
      // don't have to worry about padding.

      // We downscale the bitmap on the CPU... this is the reason
      // you should be using DDS which already has good looking mips.
      GBitmap *padBmp = bmp;
      padBmp->extrudeMipLevels();
      scalePower = getMin( scalePower, padBmp->getNumMipLevels() - 1 );

      realWidth  = getMax( (U32)1, padBmp->getWidth() >> scalePower );
      realHeight = getMax( (U32)1, padBmp->getHeight() >> scalePower );
      realBmp = new GBitmap( realWidth, realHeight, false, bmp->getFormat() );

      // Copy to the new bitmap...
      dMemcpy( realBmp->getWritableBits(), 
               padBmp->getBits(scalePower),
               padBmp->getBytesPerPixel() * realWidth * realHeight );

      // This line is commented out because createPaddedBitmap is commented out.
      // If that line is added back in, this line should be added back in.
      // delete padBmp;
   }

   // Call the internal create... (use the real* variables now, as they
   // reflect the reality of the texture we are creating.)
   U32 numMips = 0;
   GFXFormat realFmt = realBmp->getFormat();
   _validateTexParams( realWidth, realHeight, profile, numMips, realFmt );

   GFXTextureObject *ret;
   if ( inObj )
   {
      // If the texture has changed in dimensions 
      // then we need to recreate it.
      if (  inObj->getWidth() != realWidth ||
            inObj->getHeight() != realHeight ||
            inObj->getFormat() != realFmt )
         ret = _createTextureObject( realHeight, realWidth, 0, realFmt, profile, numMips, false, 0, inObj );
      else
         ret = inObj;
   }
   else
      ret = _createTextureObject(realHeight, realWidth, 0, realFmt, profile, numMips );

   if(!ret)
   {
      Con::errorf("GFXTextureManager - failed to create texture (1) for '%s'", (resourceName.isNotEmpty() ? resourceName.c_str() : "unknown"));
      return NULL;
   }

   // Extrude mip levels
   // Don't do this for fonts!
   if( ret->mMipLevels > 1 && ( realBmp->getNumMipLevels() == 1 ) && ( realBmp->getFormat() != GFXFormatA8 ) &&
      isPow2( realBmp->getHeight() ) && isPow2( realBmp->getWidth() ) && !profile->noMip() )
   {
      // NOTE: This should really be done by extruding mips INTO a DDS file instead
      // of modifying the gbitmap
      realBmp->extrudeMipLevels(false); 
   }

   // If _validateTexParams kicked back a different format, than there needs to be
   // a conversion unless it's a sRGB format
   DDSFile *bmpDDS = NULL;
   if( realBmp->getFormat() != realFmt && !profile->isSRGB() )
   {
      const GFXFormat oldFmt = realBmp->getFormat();

      // TODO: Set it up so that ALL format conversions use DDSFile. Rip format
      // switching out of GBitmap entirely.
      if( !realBmp->setFormat( realFmt ) )
      {
         // This is not the ideal implementation...
         bmpDDS = DDSFile::createDDSFileFromGBitmap( realBmp );

         bool convSuccess = false;

         if( bmpDDS != NULL )
         {       
            // This shouldn't live here, I don't think
            switch( realFmt )
            {
               case GFXFormatBC1:
               case GFXFormatBC2:
               case GFXFormatBC3:
                  // If this is a Normal Map profile, than the data needs to be conditioned
                  // to use the swizzle trick
                  if( ret->mProfile->getType() == GFXTextureProfile::NormalMap )
                  {
                     PROFILE_START(DXT_DXTNMSwizzle);
                     static DXT5nmSwizzle sDXT5nmSwizzle;
                     ImageUtil::swizzleDDS( bmpDDS, sDXT5nmSwizzle );
                     PROFILE_END();
                  }

                  convSuccess = ImageUtil::ddsCompress( bmpDDS, realFmt );
                  break;
               default:
                  AssertFatal(false, "Attempting to convert to a non-DXT format");
                  break;
            }
         }

         if( !convSuccess )
         {
            Con::errorf( "[GFXTextureManager]: Failed to change source format from %s to %s. Cannot create texture.", 
               GFXStringTextureFormat[oldFmt], GFXStringTextureFormat[realFmt] );
            delete bmpDDS;

            return NULL;
         }
      }
#ifdef TORQUE_DEBUG
      else
      {
         //Con::warnf( "[GFXTextureManager]: Changed bitmap format from %s to %s.", 
         //   GFXStringTextureFormat[oldFmt], GFXStringTextureFormat[realFmt] );
      }
#endif
   }

   // Call the internal load...
   if( ( bmpDDS == NULL && !_loadTexture( ret, realBmp ) ) || // If we aren't doing a DDS format change, use bitmap load
       ( bmpDDS != NULL && !_loadTexture( ret, bmpDDS ) ) )   // If there is a DDS, than load that instead. A format change took place.
   {
      Con::errorf("GFXTextureManager - failed to load GBitmap for '%s'", (resourceName.isNotEmpty() ? resourceName.c_str() : "unknown"));
      return NULL;
   }

   // Do statistics and book-keeping...
   
   //    - info for the texture...
   ret->mTextureLookupName = resourceName;
   ret->mBitmapSize.set(realWidth, realHeight,0);

#ifdef TORQUE_DEBUG
   if (resourceName.isNotEmpty())
      ret->mDebugDescription = resourceName;
   else
      ret->mDebugDescription = "Anonymous Texture Object";

#endif

   if(profile->doStoreBitmap())
   {
      // NOTE: may store a downscaled copy!
      SAFE_DELETE( ret->mBitmap );
      SAFE_DELETE( ret->mDDS );

      if( bmpDDS == NULL )
         ret->mBitmap = new GBitmap( *realBmp );
      else
         ret->mDDS = bmpDDS;
   }
   else
   {
      // Delete the DDS if we made one
      SAFE_DELETE( bmpDDS );
   }

   if ( !inObj )
      _linkTexture( ret );

   //    - output debug info?
   // Save texture for debug purpose
   //   static int texId = 0;
   //   char buff[256];
   //   dSprintf(buff, sizeof(buff), "tex_%d", texId++);
   //   bmp->writePNGDebug(buff);
   //   texId++;

   // Before we delete the bitmap save our transparency flag
   ret->mHasTransparency = realBmp->getHasTransparency();

   // Some final cleanup...
   if(realBmp != bmp)
      SAFE_DELETE(realBmp);
   if (deleteBmp)
      SAFE_DELETE(bmp);

   // Return the new texture!
   return ret;
}

GFXTextureObject *GFXTextureManager::createTexture( DDSFile *dds, GFXTextureProfile *profile, bool deleteDDS )
{
   AssertFatal(dds, "GFXTextureManager::createTexture() - Got NULL dds!");

   // Check the cache first...
   GFXTextureObject *cacheHit = _lookupTexture( dds, profile );
   if ( cacheHit )
   {
      //      Con::errorf("Cached texture '%s'", (fileName.isNotEmpty() ? fileName.c_str() : "unknown"));
      if( deleteDDS )
         delete dds;

      return cacheHit;
   }

   return _createTexture( dds, profile, deleteDDS, NULL );
}

GFXTextureObject *GFXTextureManager::_createTexture(  DDSFile *dds,
                                                      GFXTextureProfile *profile,
                                                      bool deleteDDS,
                                                      GFXTextureObject *inObj )
{
   PROFILE_SCOPE( GFXTextureManager_CreateTexture_DDS );

   const char *fileName = dds->getTextureCacheString();
   if( !fileName )
      fileName = "unknown";

   #ifdef DEBUG_SPEW
   Platform::outputDebugString( "[GFXTextureManager] _createTexture (DDS) '%s'",
      fileName
   );
   #endif

   // Ignore padding from the profile.
   U32 numMips = dds->mMipMapCount;
   GFXFormat fmt = dds->mFormat;
   _validateTexParams( dds->getHeight(), dds->getWidth(), profile, numMips, fmt );

   if( fmt != dds->mFormat && !profile->isSRGB())
   {
      Con::errorf( "GFXTextureManager - failed to validate texture parameters for DDS file '%s'", fileName );
      return NULL;
   }

   // Call the internal create... (use the real* variables now, as they
   // reflect the reality of the texture we are creating.)

   GFXTextureObject *ret;
   if ( inObj )
   {
      // If the texture has changed in dimensions 
      // then we need to recreate it.   
      if (  inObj->getWidth() != dds->getWidth() ||
            inObj->getHeight() != dds->getHeight() ||
            inObj->getFormat() != fmt ||
            inObj->getMipLevels() != numMips )
         ret = _createTextureObject(   dds->getHeight(), dds->getWidth(), 0, 
                                       fmt, profile, numMips, 
                                       true, 0, inObj );
      else
         ret = inObj;
   }
   else
      ret =  _createTextureObject(  dds->getHeight(), dds->getWidth(), 0, 
                                    fmt, profile, numMips, true );


   if(!ret)
   {
      Con::errorf("GFXTextureManager - failed to create texture (1) for '%s' DDSFile.", fileName);
      return NULL;
   }

   // Call the internal load...
   if(!_loadTexture(ret, dds))
   {
      Con::errorf("GFXTextureManager - failed to load DDS for '%s'", fileName);
      return NULL;
   }

   // Do statistics and book-keeping...

   //    - info for the texture...
   ret->mTextureLookupName = dds->getTextureCacheString();
   ret->mBitmapSize.set( dds->mWidth, dds->mHeight, 0 );

#ifdef TORQUE_DEBUG
   ret->mDebugDescription = fileName;
#endif

   if(profile->doStoreBitmap())
   {
      // NOTE: may store a downscaled copy!
      SAFE_DELETE( ret->mBitmap );
      SAFE_DELETE( ret->mDDS );

      ret->mDDS = new DDSFile( *dds );
   }

   if ( !inObj )
      _linkTexture( ret );

   //    - output debug info?
   // Save texture for debug purpose
   //   static int texId = 0;
   //   char buff[256];
   //   dSprintf(buff, sizeof(buff), "tex_%d", texId++);
   //   bmp->writePNGDebug(buff);
   //   texId++;

   // Save our transparency flag
   ret->mHasTransparency = dds->getHasTransparency();

   if( deleteDDS )
      delete dds;

   // Return the new texture!
   return ret;
}

GFXTextureObject *GFXTextureManager::createTexture( const Torque::Path &path, GFXTextureProfile *profile )
{
   PROFILE_SCOPE( GFXTextureManager_createTexture );
   
   // Resource handles used for loading.  Hold on to them
   // throughout this function so that change notifications
   // don't get added, then removed, and then re-added.
   
   Resource< DDSFile > dds;
   Resource< GBitmap > bitmap;
   
   // We need to handle path's that have had "incorrect"
   // extensions parsed out of the file name
   Torque::Path correctPath = validatePath(path);

   // Check the cache first...
   String pathNoExt = Torque::Path::Join( correctPath.getRoot(), ':', correctPath.getPath() );
   pathNoExt = Torque::Path::Join( pathNoExt, '/', correctPath.getFileName() );

   GFXTextureObject *retTexObj = _lookupTexture( pathNoExt, profile );
   if( retTexObj )
      return retTexObj;

   const U32 scalePower = getTextureDownscalePower( profile );

   // If this is a valid file (has an extension) than load it
   Path realPath;
   if( Torque::FS::IsFile( correctPath ) )
   {
      // Check for DDS
      if( sDDSExt.equal(correctPath.getExtension(), String::NoCase ) )
      {
         dds = DDSFile::load( correctPath, scalePower );
         if( dds != NULL )
         {
            realPath = dds.getPath();
            retTexObj = createTexture( dds, profile, false );
         }
      }
      else // Let GBitmap take care of it
      {
         bitmap = GBitmap::load( correctPath );
         if( bitmap != NULL )
         {
            realPath = bitmap.getPath();
            retTexObj = createTexture( bitmap, pathNoExt, profile, false );
         }
      }      
   }
   else
   {
      // NOTE -- We should probably remove the code from GBitmap that tries different
      // extensions for things GBitmap loads, and move it here. I think it should
      // be a bit more involved than just a list of extensions. Some kind of 
      // extension registration thing, maybe.

      // Check to see if there is a .DDS file with this name (if no extension is provided)
      Torque::Path tryDDSPath = pathNoExt;
      if( tryDDSPath.getExtension().isNotEmpty() )
         tryDDSPath.setFileName( tryDDSPath.getFullFileName() );
      tryDDSPath.setExtension( sDDSExt );

      if( Torque::FS::IsFile( tryDDSPath ) )
      {
         dds = DDSFile::load( tryDDSPath, scalePower );
         if( dds != NULL )
         {
            realPath = dds.getPath();
            retTexObj = createTexture( dds, profile, false );
         }
      }
      
      // Otherwise, retTexObj stays NULL, and fall through to the generic GBitmap
      // load.
   }

   // If we still don't have a texture object yet, feed the correctPath to GBitmap and
   // it will try a bunch of extensions
   if( retTexObj == NULL )
   {
      // Find and load the texture.
      bitmap = GBitmap::load( correctPath );

      if ( bitmap != NULL )
      {
         realPath = bitmap.getPath();
         retTexObj = createTexture( bitmap, pathNoExt, profile, false );
      }
   }

   if ( retTexObj )
   {
      // Store the path for later use.
      retTexObj->mPath = realPath;

      // Register the texture file for change notifications.
      FS::AddChangeNotification( retTexObj->getPath(), this, &GFXTextureManager::_onFileChanged );
   }

   // Could put in a final check for 'retTexObj == NULL' here as an error message.

   return retTexObj;
}

GFXTextureObject *GFXTextureManager::createTexture(  U32 width, U32 height, void *pixels, GFXFormat format, GFXTextureProfile *profile )
{
   // For now, stuff everything into a GBitmap and pass it off... This may need to be revisited -- BJG
   GBitmap *bmp = new GBitmap(width, height, 0, format);
   dMemcpy(bmp->getWritableBits(), pixels, width * height * bmp->getBytesPerPixel());

   return createTexture( bmp, String::EmptyString, profile, true );
}

GFXTextureObject *GFXTextureManager::createTexture( U32 width, U32 height, GFXFormat format, GFXTextureProfile *profile, U32 numMipLevels, S32 antialiasLevel )
{
   // Deal with sizing issues...
   U32 localWidth = width;
   U32 localHeight = height;

   // TODO: Format check HERE! -patw

   validateTextureQuality(profile, localWidth, localHeight);

   U32 numMips = numMipLevels;
   GFXFormat checkFmt = format;
   _validateTexParams( localWidth, localHeight, profile, numMips, checkFmt );

//   AssertFatal( checkFmt == format, "Anonymous texture didn't get the format it wanted." );

   GFXTextureObject *outTex = NULL;

   // If this is a pooled profile then look there first.
   if ( profile->isPooled() )
   {
      outTex = _findPooledTexure(   localWidth, localHeight, checkFmt, 
                                    profile, numMips, antialiasLevel );

      // If we got a pooled texture then its
      // already setup... just return it.
      if ( outTex )
         return outTex;
   }
   
   // Create the texture if we didn't get one from the pool.
   if ( !outTex )
   {
      outTex = _createTextureObject( localHeight, localWidth, 0, format, profile, numMips, false, antialiasLevel );

      // Make sure we add it to the pool.
      if ( outTex && profile->isPooled() )
	  {
		  mTexturePool[profile][TexturePoolDescriptor(localWidth, localHeight, format, numMips, antialiasLevel)].push_back(outTex);
	  }
   }

   if ( !outTex )
   {
      Con::errorf("GFXTextureManager - failed to create anonymous texture.");
      return NULL;
   }

   // And do book-keeping...
   //    - texture info
   outTex->mBitmapSize.set(localWidth, localHeight, 0);
   outTex->mAntialiasLevel = antialiasLevel;

   // PWTODO: Need to assign this a lookup name before _linkTexture() is called
   // otherwise it won't get a hash insert call

   _linkTexture( outTex );

   return outTex;
}

GFXTextureObject *GFXTextureManager::createTexture(   U32 width,
                                                      U32 height,
                                                      U32 depth,
                                                      void *pixels,
                                                      GFXFormat format,
                                                      GFXTextureProfile *profile,
                                                      U32 numMipLevels)
{
   PROFILE_SCOPE( GFXTextureManager_CreateTexture_3D );

   // Create texture...
   GFXTextureObject *ret = _createTextureObject( height, width, depth, format, profile, numMipLevels );

   if(!ret)
   {
      Con::errorf("GFXTextureManager - failed to create anonymous texture.");
      return NULL;
   }

   // Call the internal load...
   if( !_loadTexture( ret, pixels ) )
   {
      Con::errorf("GFXTextureManager - failed to load volume texture" );
      return NULL;
   }


   // And do book-keeping...
   //    - texture info
   ret->mBitmapSize.set( width, height, depth );

   _linkTexture( ret );


   // Return the new texture!
   return ret;
}

Torque::Path GFXTextureManager::validatePath(const Torque::Path &path)
{
   // We need to handle path's that have had "incorrect"
   // extensions parsed out of the file name
   Torque::Path correctPath = path;

   bool textureExt = false;

   // Easiest case to handle is when there isn't an extension
   if (path.getExtension().isEmpty())
      textureExt = true;

   // Since "dds" isn't registered with GBitmap currently we
   // have to test it separately
   if (sDDSExt.equal(path.getExtension(), String::NoCase))
      textureExt = true;

   // Now loop through the rest of the GBitmap extensions
   // to see if we have any matches
   for (U32 i = 0; i < GBitmap::sRegistrations.size(); i++)
   {
      // If we have gotten a match (either in this loop or before)
      // then we can exit
      if (textureExt)
         break;

      const GBitmap::Registration   &reg = GBitmap::sRegistrations[i];
      const Vector<String>          &extensions = reg.extensions;

      for (U32 j = 0; j < extensions.size(); ++j)
      {
         if (extensions[j].equal(path.getExtension(), String::NoCase))
         {
            // Found a valid texture extension
            textureExt = true;
            break;
         }
      }
   }

   // If we didn't find a valid texture extension then assume that
   // the parsed out "extension" was actually intended to be part of
   // the texture name so add it back
   if (!textureExt)
   {
      correctPath.setFileName(Torque::Path::Join(path.getFileName(), '.', path.getExtension()));
      correctPath.setExtension(String::EmptyString);
   }
   return correctPath;
}

GBitmap *GFXTextureManager::loadUncompressedTexture(const Torque::Path &path, GFXTextureProfile *profile)
{
   PROFILE_SCOPE(GFXTextureManager_loadUncompressedTexture);

   GBitmap *retBitmap = NULL;

   // Resource handles used for loading.  Hold on to them
   // throughout this function so that change notifications
   // don't get added, then removed, and then re-added.

   Resource< DDSFile > dds;
   Resource< GBitmap > bitmap;

   // We need to handle path's that have had "incorrect"
   // extensions parsed out of the file name
   Torque::Path correctPath = validatePath(path);

   U32 scalePower = profile ? getTextureDownscalePower(profile) : 0;

   // Check the cache first...
   String pathNoExt = Torque::Path::Join(correctPath.getRoot(), ':', correctPath.getPath());
   pathNoExt = Torque::Path::Join(pathNoExt, '/', correctPath.getFileName());

   // If this is a valid file (has an extension) than load it
   Path realPath;
   if (Torque::FS::IsFile(correctPath))
   {
      PROFILE_SCOPE(GFXTextureManager_loadUncompressedTexture_INNNER1)
         // Check for DDS
         if (sDDSExt.equal(correctPath.getExtension(), String::NoCase))
         {
            dds = DDSFile::load(correctPath, scalePower);
            if (dds != NULL)
            {
               realPath = dds.getPath();
               retBitmap = new GBitmap();
               if (!dds->decompressToGBitmap(retBitmap))
               {
                  delete retBitmap;
                  retBitmap = NULL;
               }
            }
         }
         else // Let GBitmap take care of it
         {
            bitmap = GBitmap::load(correctPath);
            if (bitmap != NULL)
            {
               realPath = bitmap.getPath();
               retBitmap = new GBitmap(*bitmap);

               if (scalePower &&
                  isPow2(retBitmap->getWidth()) &&
                  isPow2(retBitmap->getHeight()) &&
                  profile->canDownscale())
               {
                  retBitmap->extrudeMipLevels();
                  retBitmap->chopTopMips(scalePower);
               }
            }
         }
   }
   else
   {
      PROFILE_SCOPE(GFXTextureManager_loadUncompressedTexture_INNNER2)
         // NOTE -- We should probably remove the code from GBitmap that tries different
         // extensions for things GBitmap loads, and move it here. I think it should
         // be a bit more involved than just a list of extensions. Some kind of
         // extension registration thing, maybe.

         // Check to see if there is a .DDS file with this name (if no extension is provided)
         Torque::Path tryDDSPath = pathNoExt;
      if (tryDDSPath.getExtension().isNotEmpty())
         tryDDSPath.setFileName(tryDDSPath.getFullFileName());
      tryDDSPath.setExtension(sDDSExt);

      if (Torque::FS::IsFile(tryDDSPath))
      {
         dds = DDSFile::load(tryDDSPath, scalePower);
         if (dds != NULL)
         {
            realPath = dds.getPath();
            // Decompress dds into the GBitmap
            retBitmap = new GBitmap();
            if (!dds->decompressToGBitmap(retBitmap))
            {
               delete retBitmap;
               retBitmap = NULL;
            }
         }
      }

      // Otherwise, retTexObj stays NULL, and fall through to the generic GBitmap
      // load.
   }

   // If we still don't have a texture object yet, feed the correctPath to GBitmap and
   // it will try a bunch of extensions
   if (retBitmap == NULL)
   {
      PROFILE_SCOPE(GFXTextureManager_loadUncompressedTexture_INNNER3)
         // Find and load the texture.
         bitmap = GBitmap::load(correctPath);

      if (bitmap != NULL)
      {
         retBitmap = new GBitmap(*bitmap);

         if (scalePower &&
            isPow2(retBitmap->getWidth()) &&
            isPow2(retBitmap->getHeight()) &&
            profile->canDownscale())
         {
            retBitmap->extrudeMipLevels();
            retBitmap->chopTopMips(scalePower);
         }
      }
   }

   return retBitmap;
}

GFXTextureObject *GFXTextureManager::createCompositeTexture(const Torque::Path &pathR, const Torque::Path &pathG, const Torque::Path &pathB, const Torque::Path &pathA, U32 inputKey[4],
   GFXTextureProfile *profile)
{
   PROFILE_SCOPE(GFXTextureManager_createCompositeTexture);
   
   String inputKeyStr = String::ToString("%d%d%d%d", inputKey[0], inputKey[1], inputKey[2], inputKey[3]);

   String resourceTag = pathR.getFileName() + pathG.getFileName() + pathB.getFileName() + pathA.getFileName() + inputKeyStr; //associate texture object with a key combo

   GFXTextureObject *cacheHit = _lookupTexture(resourceTag, profile);
   if (cacheHit != NULL) return cacheHit;

   GBitmap*bitmap[4];
   bitmap[0] = loadUncompressedTexture(pathR, profile);
   if (!pathG.isEmpty())
      bitmap[1] = loadUncompressedTexture(pathG, profile);
   else
      bitmap[1] = NULL;

   if (!pathB.isEmpty())
      bitmap[2] = loadUncompressedTexture(pathB, profile);
   else
      bitmap[2] = NULL;
   if (!pathA.isEmpty())
      bitmap[3] = loadUncompressedTexture(pathA, profile);
   else
      bitmap[3] = NULL;
   

   Path realPath;
   GFXTextureObject *retTexObj = NULL;
   realPath = validatePath(pathR); //associate path with r channel texture in.

   retTexObj = createCompositeTexture(bitmap, inputKey, resourceTag, profile, false);

   if (retTexObj)
   {
      // Store the path for later use.
      retTexObj->mPath = resourceTag;

      // Register the texture file for change notifications.
      FS::AddChangeNotification(retTexObj->getPath(), this, &GFXTextureManager::_onFileChanged);
   }

   // Could put in a final check for 'retTexObj == NULL' here as an error message.
   for (U32 i = 0; i < 4; i++)
   {
      if (bitmap[i])
      {
         bitmap[i]->deleteImage();
         delete bitmap[i];
      }
   }
   return retTexObj;
}

void GFXTextureManager::saveCompositeTexture(const Torque::Path &pathR, const Torque::Path &pathG, const Torque::Path &pathB, const Torque::Path &pathA, U32 inputKey[4],
   const Torque::Path &saveAs,GFXTextureProfile *profile)
{
   PROFILE_SCOPE(GFXTextureManager_saveCompositeTexture);

   String inputKeyStr = String::ToString("%d%d%d%d", inputKey[0], inputKey[1], inputKey[2], inputKey[3]);

   String resourceTag = pathR.getFileName() + pathG.getFileName() + pathB.getFileName() + pathA.getFileName() + inputKeyStr; //associate texture object with a key combo

   GFXTextureObject *cacheHit = _lookupTexture(resourceTag, profile);
   if (cacheHit != NULL)
   {
      cacheHit->dumpToDisk("png", saveAs.getFullPath());
      return;
   }
   GBitmap*bitmap[4];
   bitmap[0] = loadUncompressedTexture(pathR, profile);
   if (!pathG.isEmpty())
      bitmap[1] = loadUncompressedTexture(pathG, profile);
   else
      bitmap[1] = NULL;

   if (!pathB.isEmpty())
      bitmap[2] = loadUncompressedTexture(pathB, profile);
   else
      bitmap[2] = NULL;
   if (!pathA.isEmpty())
      bitmap[3] = loadUncompressedTexture(pathA, profile);
   else
      bitmap[3] = NULL;


   Path realPath;
   GFXTextureObject *retTexObj = NULL;
   realPath = validatePath(pathR); //associate path with r channel texture in.

   retTexObj = createCompositeTexture(bitmap, inputKey, resourceTag, profile, false);
   if (retTexObj != NULL)
      retTexObj->dumpToDisk("png", saveAs.getFullPath());
   return;
}

DefineEngineFunction(saveCompositeTexture, void, (const char* pathR, const char* pathG, const char* pathB, const char* pathA,
                                                  const char * inputKeyString, const char* saveAs),
                                                  ("", "", "", "", "", ""), "File1,file2,file3,file4,[chanels for r g b and a locations],saveAs")
{
   U32 inputKey[4] = {0,0,0,0};

   if (dStrcmp(inputKeyString, "") != 0)
   {
      dSscanf(inputKeyString, "%i %i %i %i", &inputKey[0], &inputKey[1], &inputKey[2], &inputKey[3]);
   }
   GFX->getTextureManager()->saveCompositeTexture(pathR, pathG, pathB, pathA, inputKey, saveAs, &GFXTexturePersistentProfile);
}

GFXTextureObject *GFXTextureManager::createCompositeTexture(GBitmap*bmp[4], U32 inputKey[4],
   const String &resourceName, GFXTextureProfile *profile, bool deleteBmp)
{
   if (!bmp[0])
   {
      Con::errorf(ConsoleLogEntry::General, "GFXTextureManager::createCompositeTexture() - Got NULL bitmap(R)!");
      return NULL;
   }

   U8 rChan, gChan, bChan, aChan;
   GBitmap *outBitmap = new GBitmap();
   outBitmap->allocateBitmap(bmp[0]->getWidth(), bmp[0]->getHeight(),false, GFXFormatR8G8B8A8);
   //pack additional bitmaps into the origional
   for (U32 x = 0; x < bmp[0]->getWidth(); x++)
   {
      for (U32 y = 0; y < bmp[0]->getHeight(); y++)
      {
         rChan = bmp[0]->getChanelValueAt(x, y, inputKey[0]);

         if (bmp[1])
            gChan = bmp[1]->getChanelValueAt(x, y, inputKey[1]);
         else
            gChan = 255;

         if (bmp[2])
            bChan = bmp[2]->getChanelValueAt(x, y, inputKey[2]);
         else
            bChan = 255;

         if (bmp[3])
            aChan = bmp[3]->getChanelValueAt(x, y, inputKey[3]);
         else
            aChan = 255;

		 outBitmap->setColor(x, y, ColorI(rChan, gChan, bChan, aChan));
      }
   }

   GFXTextureObject *cacheHit = _lookupTexture(resourceName, profile);
   if (cacheHit != NULL)
   {
      // Con::errorf("Cached texture '%s'", (resourceName.isNotEmpty() ? resourceName.c_str() : "unknown"));
	   if (deleteBmp)
	   {
		   delete outBitmap;
		   delete[] bmp;
	   }
      return cacheHit;
   }

   return _createTexture(outBitmap, resourceName, profile, deleteBmp, NULL);
}

GFXTextureObject* GFXTextureManager::_findPooledTexure(U32 width,
	U32 height,
	GFXFormat format,
	GFXTextureProfile* profile,
	U32 numMipLevels,
	S32 antialiasLevel)
{
   PROFILE_SCOPE( GFXTextureManager_FindPooledTexure );

	// find by profile
	const auto& profilePoolItem = mTexturePool.find(profile);
	if (profilePoolItem != mTexturePool.end())
	{
		// find by descriptor
		const auto& descriptorPool = (*profilePoolItem).second;

		TexturePoolDescriptor descriptor(width, height, format, numMipLevels, antialiasLevel);

		const auto& descriptorPoolItem = descriptorPool.find(descriptor);
		if (descriptorPoolItem != descriptorPool.end())
		{
			// loop through available textures to find unused one
			const auto& poolItems = (*descriptorPoolItem).second;

			for (const auto& textureItem : poolItems)
			{
				// If the reference count is 1 then we're the only
				// ones holding on to this texture and we can hand
				// it out if the size matches... else its in use.
				if (textureItem->getRefCount() == 1)
				{
					return textureItem;
				}

				// EXCEPT for render targets - render target reuse is allowed
				// EXCEPT for postFX targets - they need a huge rework to be shareable
				static bool enableRenderTargetSharing = true;
				if (enableRenderTargetSharing && profile->isRenderTarget() && (profile != &GFXRenderTargetProfile || profile != &GFXRenderTargetSRGBProfile || profile != &GFXDynamicTextureProfile || profile != &GFXDynamicTextureSRGBProfile))
				{
					return textureItem;
				}
			}
		}
	}

	Con::warnf("GFXTextureManager::_findPooledTexure::failed to find pooled texture, creating a new one (%i %i %i %i)",
		width, height, static_cast<U32>(format), numMipLevels);
	return nullptr;
}

void GFXTextureManager::hashInsert( GFXTextureObject *object )
{
   if ( object->mTextureLookupName.isEmpty() )
      return;

   MutexHandle mutexHandle = TORQUE_LOCK(mMutex);
   U32 key = object->mTextureLookupName.getHashCaseInsensitive() % mHashCount;
   object->mHashNext = mHashTable[key];
   mHashTable[key] = object;
}

void GFXTextureManager::hashRemove( GFXTextureObject *object )
{
   if ( object->mTextureLookupName.isEmpty() )
      return;

   MutexHandle mutexHandle = TORQUE_LOCK(mMutex);

   U32 key = object->mTextureLookupName.getHashCaseInsensitive() % mHashCount;
   GFXTextureObject **walk = &mHashTable[key];
   while(*walk)
   {
      if(*walk == object)
      {
         *walk = object->mHashNext;
         break;
      }
      walk = &((*walk)->mHashNext);
   }
}

GFXTextureObject* GFXTextureManager::hashFind( const String &name )
{
   if ( name.isEmpty() )
      return NULL;

   MutexHandle mutexHandle = TORQUE_LOCK(mMutex);

   U32 key = name.getHashCaseInsensitive() % mHashCount;
   GFXTextureObject *walk = mHashTable[key];
   for(; walk; walk = walk->mHashNext)
   {
      if( walk->mTextureLookupName.equal( name, String::NoCase ) )
         break;
   }

   return walk;
}

void GFXTextureManager::freeTexture(GFXTextureObject *texture, bool zombify)
{
   // Ok, let the backend deal with it.
   _freeTexture(texture, zombify);
}

void GFXTextureManager::refreshTexture(GFXTextureObject *texture)
{
   _refreshTexture(texture);
}

void GFXTextureManager::_linkTexture( GFXTextureObject *obj )
{
	MutexHandle mutexHandle = TORQUE_LOCK(mMutex);

   // info for the profile
   GFXTextureProfile::updateStatsForCreation(obj);

   // info for the cache
   hashInsert(obj);

   // info for the master list
   if( mListHead == NULL )
      mListHead = obj;

   if( mListTail != NULL ) 
      mListTail->mNext = obj;

   obj->mPrev = mListTail;
   mListTail = obj;
}

void GFXTextureManager::deleteTexture( GFXTextureObject *texture )
{
   if ( mTextureManagerState == GFXTextureManager::Dead )
      return;

   #ifdef DEBUG_SPEW
   Platform::outputDebugString( "[GFXTextureManager] deleteTexture '%s'",
      texture->mTextureLookupName.c_str()
   );
   #endif

   MutexHandle mutexHandle = TORQUE_LOCK(mMutex);

   if( mListHead == texture )
      mListHead = texture->mNext;
   if( mListTail == texture )
      mListTail = texture->mPrev;

   hashRemove( texture );

   // If we have a path for the texture then
   // remove change notifications for it.
   Path texPath = texture->getPath();
   if ( !texPath.isEmpty() )
      FS::RemoveChangeNotification( texPath, this, &GFXTextureManager::_onFileChanged );

   GFXTextureProfile::updateStatsForDeletion(texture);

   freeTexture( texture );
}

void GFXTextureManager::_validateTexParams( const U32 width, const U32 height, 
                                          const GFXTextureProfile *profile, 
                                          U32 &inOutNumMips, GFXFormat &inOutFormat  )
{
   // Validate mipmap parameter. If this profile requests no mips, set mips to 1.
   if( profile->noMip() )
   {
      inOutNumMips = 1;
   }
   else if( !isPow2( width ) || !isPow2( height ) )
   {
      // If a texture is not power-of-2 in size for both dimensions, it must
      // have only 1 mip level.
      inOutNumMips = 1;
   }
   
   // Check format, and compatibility with texture profile requirements
   bool autoGenSupp = ( inOutNumMips == 0 );

   // If the format is non-compressed, and the profile requests a compressed format
   // than change the format.
   GFXFormat testingFormat = inOutFormat;
   if( profile->getCompression() != GFXTextureProfile::NONE )
   {
      const S32 offset = profile->getCompression() - GFXTextureProfile::BC1;
      testingFormat = GFXFormat( GFXFormatBC1 + offset );

      // No auto-gen mips on compressed textures
      autoGenSupp = false;
   }

   if (profile->isSRGB())
      testingFormat = ImageUtil::toSRGBFormat(testingFormat);

   // inOutFormat is not modified by this method
   GFXCardProfiler* cardProfiler = GFX->getCardProfiler();
   bool chekFmt = cardProfiler->checkFormat(testingFormat, profile, autoGenSupp);
   
   if( !chekFmt )
   {
      // It tested for a compressed format, and didn't like it
      if( testingFormat != inOutFormat && profile->getCompression() )
         testingFormat = inOutFormat; // Reset to requested format, and try again

      // Trying again here, so reset autogen mip
      autoGenSupp = ( inOutNumMips == 0 );

      // Wow more weak sauce. There should be a better way to do this.
      switch( inOutFormat )
      {
         case GFXFormatR8G8B8:
            testingFormat = GFXFormatR8G8B8X8;
            chekFmt = cardProfiler->checkFormat(testingFormat, profile, autoGenSupp);
            break;

         case GFXFormatA8:
            testingFormat = GFXFormatR8G8B8A8;
            chekFmt = cardProfiler->checkFormat(testingFormat, profile, autoGenSupp);
            break;
         
         default:
            chekFmt = cardProfiler->checkFormat(testingFormat, profile, autoGenSupp);
            break;
      }
   }

   // Write back num mips that need to be generated by GBitmap
   if( !chekFmt )
      Con::errorf( "Format %s not supported with specified profile.", GFXStringTextureFormat[inOutFormat] );
   else
   {
      inOutFormat = testingFormat;

      // If auto gen mipmaps were requested, and they aren't supported for whatever
      // reason, than write out the number of mips that need to be generated.
      //
      // NOTE: Does this belong here?
      if( inOutNumMips == 0 && !autoGenSupp )
      {
         inOutNumMips = mFloor(mLog2(mMax(width, height))) + 1;
      }
   }
}

GFXCubemap* GFXTextureManager::createCubemap( const Torque::Path &path )
{
   // Very first thing... check the cache.
	MutexHandle mutexHandle = TORQUE_LOCK(mMutex);
   CubemapTable::Iterator iter = mCubemapTable.find( path.getFullPath() );
   if ( iter != mCubemapTable.end() )
      return iter->value;

   mutexHandle.unlock();
   // Not in the cache... we have to load it ourselves.

   // First check for a DDS file.
   if ( !sDDSExt.equal( path.getExtension(), String::NoCase ) )
   {
      // At the moment we only support DDS cubemaps.
      return NULL;
   }

   const U32 scalePower = getTextureDownscalePower( NULL );

   // Ok... load the DDS file then.
   Resource<DDSFile> dds = DDSFile::load( path, scalePower );
   if ( !dds || !dds->isCubemap() )
   {
      // This wasn't a cubemap... give up too.
      return NULL;
   }

   // We loaded the cubemap dds, so now we create the GFXCubemap from it.
   GFXCubemap *cubemap = GFX->createCubemap();
   cubemap->initStatic( dds );
   cubemap->_setPath( path.getFullPath() );

   // Store the cubemap into the cache.
   MutexHandle mutexHandle2 = TORQUE_LOCK(mMutex);
   mCubemapTable.insertUnique( path.getFullPath(), cubemap );

   return cubemap;
}

void GFXTextureManager::releaseCubemap( GFXCubemap *cubemap )
{
   if ( mTextureManagerState == GFXTextureManager::Dead )
      return;

   const String &path = cubemap->getPath();
   MutexHandle mutexHandle = TORQUE_LOCK(mMutex);

   CubemapTable::Iterator iter = mCubemapTable.find( path );
   if ( iter != mCubemapTable.end() && iter->value == cubemap )
      mCubemapTable.erase( iter );

   // If we have a path for the texture then
   // remove change notifications for it.
   //Path texPath = texture->getPath();
   //if ( !texPath.isEmpty() )
      //FS::RemoveChangeNotification( texPath, this, &GFXTextureManager::_onFileChanged );
}

void GFXTextureManager::_onFileChanged( const Torque::Path &path )
{
	MutexHandle mutexHandle = TORQUE_LOCK(mMutex);
   String pathNoExt = Torque::Path::Join( path.getRoot(), ':', path.getPath() );
   pathNoExt = Torque::Path::Join( pathNoExt, '/', path.getFileName() );

   // See if we've got it loaded.
   GFXTextureObject *obj = hashFind( pathNoExt );
   if ( !obj || path != obj->getPath() )
      return;

   Con::errorf( "[GFXTextureManager::_onFileChanged] : File changed [%s]", path.getFullPath().c_str() );

   const U32 scalePower = getTextureDownscalePower( obj->mProfile );

   if ( sDDSExt.equal( path.getExtension(), String::NoCase) )
   {
      Resource<DDSFile> dds = DDSFile::load( path, scalePower );
      if ( dds )
         _createTexture( dds, obj->mProfile, false, obj );
   }
   else
   {
      Resource<GBitmap> bmp = GBitmap::load( path );
      if( bmp )
         _createTexture( bmp, obj->mTextureLookupName, obj->mProfile, false, obj );
   }
}

void GFXTextureManager::reloadTextures()
{
	MutexHandle mutexHandleGFX = TORQUE_LOCK(GFX->mMutex);
	MutexHandle mutexHandleThis = TORQUE_LOCK(mMutex);

   GFXTextureObject *tex = mListHead;

   while ( tex != NULL ) 
   {
      const Torque::Path path( tex->mPath );
      if ( !path.isEmpty() )
      {
         const U32 scalePower = getTextureDownscalePower( tex->mProfile );

         if ( sDDSExt.equal( path.getExtension(), String::NoCase ) )
         {
            Resource<DDSFile> dds = DDSFile::load( path, scalePower );
            if ( dds )
               _createTexture( dds, tex->mProfile, false, tex );
         }
         else
         {
            Resource<GBitmap> bmp = GBitmap::load( path );
            if( bmp )
               _createTexture( bmp, tex->mTextureLookupName, tex->mProfile, false, tex );
         }
      }

      tex = tex->mNext;
   }
}

void GFXTextureManager::reloadTextures(std::unordered_map<GFXTextureObject*, Resource<DDSFile>>& ddsFiles, std::unordered_map<GFXTextureObject*, Resource<GBitmap>>& bmpFiles)
{
	MutexHandle mutexHandleGFX = TORQUE_LOCK(GFX->mMutex);
	MutexHandle mutexHandleThis = TORQUE_LOCK(mMutex);

	for (GFXTextureObject* tex = mListHead; tex != nullptr; tex = tex->mNext)
	{
		std::unordered_map<GFXTextureObject*, Resource<DDSFile>>::iterator dds = ddsFiles.find(tex);
		if (dds != ddsFiles.end())
		{
			_createTexture(dds->second, dds->first->mProfile, false, dds->first);
		}
		else
		{
			
			std::unordered_map<GFXTextureObject*, Resource<GBitmap>>::iterator bmp = bmpFiles.find(tex);
			if (bmp != bmpFiles.end())
			{
				_createTexture(bmp->second, bmp->first->mTextureLookupName, bmp->first->mProfile, false, bmp->first);
			}
		}
	}
}

class LoadTexturesWorker : public ThreadPool::WorkItem
{
public:

	LoadTexturesWorker(std::unordered_map<GFXTextureObject*, Resource<DDSFile>>&& ddsFiles,
		std::unordered_map<GFXTextureObject*, Resource<GBitmap>>&& bmpFiles)
		: mDDSFiles(std::move(ddsFiles)), mBMPFiles(std::move(bmpFiles))
	{
		mFlags |= FlagMainThreadOnly;
	}

	void execute() override
	{
		TEXMGR->reloadTextures(mDDSFiles, mBMPFiles);
	}

private:
	std::unordered_map<GFXTextureObject*, Resource<DDSFile>> mDDSFiles;
	std::unordered_map<GFXTextureObject*, Resource<GBitmap>> mBMPFiles;
};

class LoadTexturesSyncWorker : public ThreadPool::WorkItem
{
public:

	LoadTexturesSyncWorker()
	{
		mFlags |= FlagDoesSynchronousIO;
	}

	void execute() override
	{
		std::unordered_map<GFXTextureObject*, Resource<DDSFile>> ddsFiles;
		std::unordered_map<GFXTextureObject*, Resource<GBitmap>> bmpFiles;

		for (const auto& data : mData)
		{
			if (data.path.getExtension().equal(sDDSExt, String::NoCase))
			{
				Resource<DDSFile> dds = DDSFile::load(data.path, data.scalePower);

				if (dds)
				{
					ddsFiles[data.tex] = dds;
				}
			}
			else
			{
				Resource<GBitmap> bmp = GBitmap::load(data.path);

				if (bmp)
				{
					bmpFiles[data.tex] = bmp;
				}
			}
		}

		ThreadPool::instance()->queueWorkItem(new LoadTexturesWorker(std::move(ddsFiles), std::move(bmpFiles)));
	}

	void addTexture(GFXTextureObject* tex, const Torque::Path path, U32 scalePower)
	{
		Data data;
		data.tex = tex;
		data.path = path;
		data.scalePower = scalePower;
		mData.push_back(data);
	}

private:
	struct Data
	{
		GFXTextureObject* tex;
		Torque::Path path;
		U32 scalePower;
	};

	std::vector<Data> mData;
};

void GFXTextureManager::sheduleReloadTextures()
{
	MutexHandle mutexHandleGFX = TORQUE_LOCK(GFX->mMutex);
	MutexHandle mutexHandleThis = TORQUE_LOCK(mMutex);

	auto worker = std::make_unique<LoadTexturesSyncWorker>();

	for (auto tex = mListHead; tex != nullptr; tex = tex->mNext)
	{
		Torque::Path path(tex->mPath);
		if (!path.isEmpty())
		{
			//hack: rename texture
			String fileName = path.getFileName();
			path.setFileName(fileName);


			if (tex->mPath != path)
			{
				tex->mPath = path;
				const U32 scalePower = TEXMGR->getTextureDownscalePower(tex->mProfile);
				worker->addTexture(tex, path, scalePower);
			}
		}
	}

	ThreadPool::instance()->queueWorkItem(worker.release());
}

DefineEngineFunction( flushTextureCache, void, (),,
   "Releases all textures and resurrects the texture manager.\n"
   "@ingroup GFX\n" )
{
   if ( !GFX || !TEXMGR )
      return;

   TEXMGR->zombify();
   TEXMGR->resurrect();
}

DefineEngineFunction( cleanupTexturePool, void, (),,
   "Release the unused pooled textures in texture manager freeing up video memory.\n"
   "@ingroup GFX\n" )
{
   if ( !GFX || !TEXMGR )
      return;

   TEXMGR->cleanupPool();
}

DefineEngineFunction( reloadTextures, void, (),,
   "Reload all the textures from disk.\n"
   "@ingroup GFX\n" )
{
   if ( !GFX || !TEXMGR )
      return;

   TEXMGR->reloadTextures();
}

void GFXTextureManager::dumpTextures()
{
	MutexHandle mutexHandleGFX = TORQUE_LOCK(GFX->mMutex);
	MutexHandle mutexHandleThis = TORQUE_LOCK(mMutex);

	GFXTextureObject* tex = mListHead;

	U32 totalSize = 0, namedSize = 0;
	Con::printf("******************************* begin dumpTextures **********************************");
	while (tex != NULL)
	{
		totalSize += tex->getEstimatedSizeInBytes();

		Torque::Path path(tex->mPath);
		if (!path.isEmpty())
		{
			namedSize += tex->getEstimatedSizeInBytes();
			Con::printf("Texture: %s  with size %i", tex->mPath.c_str(), tex->getEstimatedSizeInBytes());
		}

		tex = tex->mNext;
	}

	Con::printf("Total texture size: %u  ", totalSize);
	Con::printf("Internal texture size: %u  ", totalSize - namedSize);
	Con::printf("******************************* end dumpTextures **********************************");
}

DefineEngineFunction(dumpTextures, void, (), ,
	"Dump the textures from disk.\n"
	"@ingroup GFX\n")
{
	if (!GFX || !TEXMGR)
	{
		return;
	}

	TEXMGR->dumpTextures();
}
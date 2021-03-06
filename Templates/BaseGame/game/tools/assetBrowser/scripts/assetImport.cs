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

//
//
function isImageFormat(%fileExt)
{
   if( (%fileExt $= ".png") || (%fileExt $= ".jpg") || (%fileExt $= ".bmp") || (%fileExt $= ".dds") || (%fileExt $= ".tif"))
      return true;
      
   return false;
}

function isShapeFormat(%fileExt)
{
   if( (%fileExt $= ".dae") || 
   (%fileExt $= ".dts") || 
   (%fileExt $= ".fbx") || 
   (%fileExt $= ".gltf") || 
   (%fileExt $= ".glb") || 
   (%fileExt $= ".obj") || 
   (%fileExt $= ".blend"))
      return true;
      
   return false;
}

function isSoundFormat(%fileExt)
{
   if( (%fileExt $= ".ogg") || (%fileExt $= ".wav") || (%fileExt $= ".mp3"))
      return true;
      
   return false;
}

function getImageInfo(%file)
{
   //we're going to populate a GuiTreeCtrl with info of the inbound image file
}

//This lets us go and look for a image at the importing directory as long as it matches the material name
function findImageFile(%path, %materialName, %type)
{
   
   if(isFile(%path @ "/" @ %materialName @ ".jpg"))
      return %path @ "/" @ %materialName @ ".jpg";
   else if(isFile(%path @ "/" @ %materialName @ ".png"))
      return %path @ "/" @ %materialName @ ".png";
   else if(isFile(%path @ "/" @ %materialName @ ".dds"))
      return %path @ "/" @ %materialName @ ".dds";
   else if(isFile(%path @ "/" @ %materialName @ ".tif"))
      return %path @ "/" @ %materialName @ ".tif";
}

function AssetBrowser::onBeginDropFiles( %this )
{   
   if(!AssetBrowser.isAwake())
      return;
      
   error("% DragDrop - Beginning files dropping.");
   if(!ImportAssetWindow.isAwake())
      ImportAssetWindow.showDialog();
}

function AssetBrowser::onDropFile( %this, %filePath )
{
   if(!%this.isVisible())
      return;
      
   %fileExt = fileExt( %filePath );
   //add it to our array!
   if(isImageFormat(%fileExt))
      %this.addImportingAsset("ImageAsset", %filePath);
   else if( isShapeFormat(%fileExt))
      %this.addImportingAsset("ShapeAsset", %filePath);
   else if( isSoundFormat(%fileExt))
      %this.addImportingAsset("SoundAsset", %filePath);
   else if( %fileExt $= ".cs" || %fileExt $= ".cs.dso" )
      %this.addImportingAsset("ScriptAsset", %filePath);
   else if( %fileExt $= ".gui" || %fileExt $= ".gui.dso" )
      %this.addImportingAsset("GUIAsset", %filePath);
   else if (%fileExt $= ".zip")
      %this.onDropZipFile(%filePath);
   else if( %fileExt $= "")
      %this.onDropFolder(%filePath);
      
   //Used to keep tabs on what files we were trying to import, used mainly in the event of
   //adjusting configs and needing to completely reprocess the import
   //ensure we're not doubling-up on files by accident
   if(ImportAssetWindow.importingFilesArray.getIndexFromKey(%filePath) == -1)
      ImportAssetWindow.importingFilesArray.add(%filePath);
}

function AssetBrowser::onDropZipFile(%this, %filePath)
{
   if(!%this.isVisible())
      return;
      
   %zip = new ZipObject();
   %zip.openArchive(%filePath);
   %count = %zip.getFileEntryCount();
   
   echo("Dropped in a zip file with" SPC %count SPC "files inside!");
   
   for (%i = 0; %i < %count; %i++)
   {
      %fileEntry = %zip.getFileEntry(%i);
      %fileFrom = getField(%fileEntry, 0);
      
      //First, we wanna scan to see if we have modules to contend with. If we do, we'll just plunk them in wholesale
      //and not process their contents.
      
      //If not modules, it's likely an art pack or other mixed files, so we'll import them as normal
      /*if( (%fileExt $= ".png") || (%fileExt $= ".jpg") || (%fileExt $= ".bmp") || (%fileExt $= ".dds") )
         %this.importAssetListArray.add("ImageAsset", %filePath);
      else if( (%fileExt $= ".dae") || (%fileExt $= ".dts"))
         %this.importAssetListArray.add("ShapeAsset", %filePath);
      else if( (%fileExt $= ".ogg") || (%fileExt $= ".wav") || (%fileExt $= ".mp3"))
         %this.importAssetListArray.add("SoundAsset", %filePath);
      else if( (%fileExt $= ".gui") || (%fileExt $= ".gui.dso"))
         %this.importAssetListArray.add("GUIAsset", %filePath);
      //else if( (%fileExt $= ".cs") || (%fileExt $= ".dso"))
      //   %this.importAssetListArray.add("Script", %filePath);
      else if( (%fileExt $= ".mis"))
         %this.importAssetListArray.add("LevelAsset", %filePath);*/
         
      // For now, if it's a .cs file, we'll assume it's a behavior.
      //if (fileExt(%fileFrom) !$= ".cs")
      //   continue;
      
      %fileTo = expandFilename("^tools/assetBrowser/importTemp/") @ %fileFrom;
      %zip.extractFile(%fileFrom, %fileTo);
      //exec(%fileTo);
   }
   
   %zip.delete();
   
   //Next, we loop over the files and import them
}

function AssetBrowser::onDropFolder(%this, %filePath)
{
   if(!%this.isVisible())
      return;
      
   %zip = new ZipObject();
   %zip.openArchive(%filePath);
   %count = %zip.getFileEntryCount();
   
   echo("Dropped in a zip file with" SPC %count SPC "files inside!");
   
   return;
   for (%i = 0; %i < %count; %i++)
   {
      %fileEntry = %zip.getFileEntry(%i);
      %fileFrom = getField(%fileEntry, 0);
      
      //First, we wanna scan to see if we have modules to contend with. If we do, we'll just plunk them in wholesale
      //and not process their contents.
      
      //If not modules, it's likely an art pack or other mixed files, so we'll import them as normal
      if( (%fileExt $= ".png") || (%fileExt $= ".jpg") || (%fileExt $= ".bmp") || (%fileExt $= ".dds") )
         %this.importAssetListArray.add("ImageAsset", %filePath);
      else if( (%fileExt $= ".dae") || (%fileExt $= ".dts"))
         %this.importAssetListArray.add("ShapeAsset", %filePath);
      else if( (%fileExt $= ".ogg") || (%fileExt $= ".wav") || (%fileExt $= ".mp3"))
         %this.importAssetListArray.add("SoundAsset", %filePath);
      else if( (%fileExt $= ".gui") || (%fileExt $= ".gui.dso"))
         %this.importAssetListArray.add("GUIAsset", %filePath);
      //else if( (%fileExt $= ".cs") || (%fileExt $= ".dso"))
      //   %this.importAssetListArray.add("Script", %filePath);
      else if( (%fileExt $= ".mis"))
         %this.importAssetListArray.add("LevelAsset", %filePath);
         
      // For now, if it's a .cs file, we'll assume it's a behavior.
      if (fileExt(%fileFrom) !$= ".cs")
         continue;
      
      %fileTo = expandFilename("^game/behaviors/") @ fileName(%fileFrom);
      %zip.extractFile(%fileFrom, %fileTo);
      exec(%fileTo);
   }
}

function AssetBrowser::onEndDropFiles( %this )
{
   if(!%this.isVisible())
      return;
      
   ImportAssetWindow.refresh();
}

//
//
//
function AssetBrowser::reloadImportingFiles(%this)
{
   //Effectively, we re-import the files we were trying to originally. We'd only usually do this in the event we change our import config
   %this.onBeginDropFiles();
   
   for(%i=0; %i < ImportAssetWindow.importingFilesArray.count(); %i++)
   {
      %this.onDropFile(ImportAssetWindow.importingFilesArray.getKey(%i));
   }
    
   %this.onEndDropFiles();  
}

function AssetBrowser::ImportTemplateModules(%this)
{
   //AssetBrowser_ImportModule
   Canvas.pushDialog(AssetBrowser_ImportModuleTemplate);
   AssetBrowser_ImportModuleTemplateWindow.visible = true;   
   
   AssetBrowser_ImportModuleTemplateList.clear();
   
   //ModuleDatabase.scanModules("../../../../../../Templates/Modules/");
   
   %pattern = "../../../../../../Templates/Modules//*//*.module";   
   %file = findFirstFile( %pattern );

   while( %file !$= "" )
   {      
      echo("FOUND A TEMPLATE MODULE! " @ %file);
      %file = findNextFile( %pattern );
   }
   
   /*%moduleCheckbox = new GuiCheckBoxCtrl()
   {
      text = "Testadoo";
      moduleId = "";
   };
   
   AssetBrowser_ImportModuleTemplateList.addRow("0", "Testaroooooo");
   AssetBrowser_ImportModuleTemplateList.addRow("1", "Testadoooooo");*/
}

function AssetBrowser_ImportModuleTemplateList::onSelect(%this, %selectedRowIdx, %text)
{
   echo("Selected row: " @ %selectedRowIdx @ " " @ %text);
}

function AssetBrowser::addImportingAsset( %this, %assetType, %filePath, %parentAssetItem, %assetNameOverride )
{
   //In some cases(usually generated assets on import, like materials) we'll want to specifically define the asset name instead of peeled from the filePath
   if(%assetNameOverride !$= "")
      %assetName = %assetNameOverride;
   else
      %assetName = fileBase(%filePath);
      
   //We don't get a file path at all if we're a generated entry, like materials
   //if we have a file path, though, then sanitize it
   if(%filePath !$= "")
      %filePath = filePath(%filePath) @ "/" @ fileBase(%filePath) @ fileExt(%filePath);
   
   %moduleName = AssetBrowser.SelectedModule;
   ImportAssetModuleList.text = %moduleName;
   
   //Add to our main list
   %assetItem = new ScriptObject()
   {
      assetType = %assetType;
      filePath = %filePath;
      assetName = %assetName;
      cleanAssetName = %assetName; 
      moduleName = %moduleName;
      dirty  = true;
      parentAssetItem = %parentAssetItem;
      status = "";
      statusType = "";
      statusInfo = "";
      skip = false;
      processed = false;
      generatedAsset = false;
   };
   
   if(%parentAssetItem !$= "")
   {
      ImportActivityLog.add("Added Child Importing Asset to " @ %parentAssetItem.assetName);
   }
   else
   {
      ImportActivityLog.add("Added Importing Asset");
   }
   
   ImportActivityLog.add("   Asset Info: Name: " @ %assetName @ " | Type: " @ %assetType);
   
   if(%assetType $= "MaterialAsset")
   {
      %assetItem.generatedAsset = true;  
   }
   else
   {
      ImportActivityLog.add("   File: " @ %filePath);
   }
   
   if(%parentAssetItem $= "")
   {
      ImportAssetTree.insertObject(1, %assetItem);
      
      //%assetItem.parentDepth = 0;
      //%this.importAssetNewListArray.add(%assetItem);
      //%this.importAssetUnprocessedListArray.add(%assetItem);
   }
   else
   {
      %parentid = ImportAssetTree.findItemByObjectId(%parentAssetItem);
      ImportAssetTree.insertObject(%parentid, %assetItem);
   }
   
   %this.unprocessedAssetsCount++;
   
   ImportAssetWindow.assetValidationList.add(%assetItem);
   
   ImportAssetWindow.refresh();
   
   return %assetItem;
}

function AssetBrowser::importLegacyGame(%this)
{
   
}

function AssetBrowser::importNewAssetFile(%this)
{
   %dlg = new OpenFileDialog()
   {
      Filters        = "Shape Files(*.dae, *.cached.dts)|*.dae;*.cached.dts|Images Files(*.jpg,*.png,*.tga,*.bmp,*.dds)|*.jpg;*.png;*.tga;*.bmp;*.dds|Any Files (*.*)|*.*|";
      DefaultPath    = $Pref::WorldEditor::LastPath;
      DefaultFile    = "";
      ChangePath     = false;
      OverwritePrompt = true;
      forceRelativePath = false;
      //MultipleFiles = true;
   };

   %ret = %dlg.Execute();
   
   if ( %ret )
   {
      $Pref::WorldEditor::LastPath = filePath( %dlg.FileName );
      %fullPath = %dlg.FileName;
      %file = fileBase( %fullPath );
   }   
   
   %dlg.delete();
   
   if ( !%ret )
      return;
      
   AssetBrowser.onBeginDropFiles();
   AssetBrowser.onDropFile(%fullPath);
   AssetBrowser.onEndDropFiles();
}

//
function ImportAssetButton::onClick(%this)
{
   //ImportAssetsPopup.showPopup(Canvas);
   
   Canvas.pushDialog(AssetImportCtrl);
}
//

function ImportAssetWindow::showDialog(%this)
{
   ImportAssetWindow.importAssetUnprocessedListArray.empty();
   ImportAssetWindow.importAssetFinalListArray.empty();
   
   ImportAssetWindow.assetHeirarchyChanged = false;
   
   //prep the import control
   Canvas.pushDialog(AssetImportCtrl);
   AssetImportCtrl.setHidden(true);

   ImportAssetTree.clear();
   ImportAssetTree.insertItem(0, "Importing Assets");
   AssetBrowser.unprocessedAssetsCount = 0;
   
   %this.dirty = false;
}

function ImportAssetWindow::Close(%this)
{
   //Some cleanup
   ImportAssetWindow.importingFilesArray.empty();
   
   %this.importTempDirHandler.deleteFolder("tools/assetBrowser/importTemp/*/");
   
   if(ImportAssetWindow.isAwake())
      ImportAssetWindow.refresh();
      
   Canvas.popDialog();  
}
//
function ImportAssetWindow::onWake(%this)
{
   //We've woken, meaning we're trying to import assets
   //Lets refresh our list
   if(!ImportAssetWindow.isVisible())
      return;
      
   if(!isObject(%this.importTempDirHandler))
      %this.importTempDirHandler = makedirectoryHandler(0, "", ""); 
      
   if(!isObject(AssetImportSettings))
   {
      new Settings(AssetImportSettings) 
      { 
         file = $AssetBrowser::importConfigsFile; 
      };
   }
   AssetImportSettings.read();
   
   %this.reloadImportOptionConfigs();
   
   if(!isObject(%this.assetValidationList))
   {
      %this.assetValidationList = new ArrayObject();
   }
   
   AssetImportCtrl-->NewAssetsTree.buildIconTable( ":tools/classIcons/TSStatic:tools/classIcons/TSStatic" @
                                             ":tools/classIcons/material:tools/classIcons/material"@
                                             ":tools/classIcons/GuiBitmapCtrl:tools/classIcons/GuiBitmapCtrl"@
                                             ":tools/classIcons/SFXEmitter:tools/classIcons/SFXEmitter"@
                                             ":tools/gui/images/iconWarn:tools/gui/images/iconWarn"@
                                             ":tools/gui/images/iconError:tools/gui/images/iconError");
   
   AssetImportTargetAddress.text = AssetBrowser.dirHandler.currentAddress;
   AssetImportTargetModule.text = AssetBrowser.dirHandler.getModuleFromAddress(AssetBrowser.dirHandler.currentAddress).ModuleId;
   ImportAssetConfigList.setSelected(0);
   
   ImportActivityLog.empty();
   
   %this.refresh();
}

function ImportAssetWindow::reloadImportOptionConfigs(%this)
{
   if(!isObject(ImportAssetWindow.importConfigsList))
      ImportAssetWindow.importConfigsList = new ArrayObject();
   else
      ImportAssetWindow.importConfigsList.empty();
      
   ImportAssetConfigList.clear();
   
   %xmlDoc = new SimXMLDocument();
   if(%xmlDoc.loadFile($AssetBrowser::importConfigsFile))
   {
      //StateMachine element
      if(!%xmlDoc.pushFirstChildElement("AssetImportSettings"))
      {
         error("Invalid Import Configs file");
         return;  
      }
      
      //Config Groups
      %configCount = 0;
      %hasGroup = %xmlDoc.pushFirstChildElement("Group");
      while(%hasGroup)
      {
         %configName = %xmlDoc.attribute("name");
         
         ImportAssetWindow.importConfigsList.add(%configName);
         ImportAssetConfigList.add(%configName);
         
         %hasGroup = %xmlDoc.nextSiblingElement("Group");
      }

      %xmlDoc.popElement();
   }
   
   %xmlDoc.delete();
   
   %importConfigIdx = ImportAssetWindow.activeImportConfigIndex;
   if(%importConfigIdx $= "")
      %importConfigIdx = 0;
      
   //ImportAssetConfigList.setSelected(%importConfigIdx);
}

//
function assetImportUpdatePath(%newPath)
{
   AssetBrowser.navigateTo(%newPath);
   AssetImportTargetAddress.text = %newPath;
   AssetImportTargetModule.text = AssetBrowser.dirHandler.getModuleFromAddress(AssetBrowser.dirHandler.currentAddress).ModuleId;
}

//
function ImportAssetWindow::processNewImportAssets(%this, %id)
{
   while(%id > 0)
   {
      %assetItem = ImportAssetTree.getItemObject(%id);
      
      if(isObject(%assetItem) && %assetItem.processed == false)
      {
         //%assetConfigObj = ImportAssetWindow.activeImportConfig.clone();
         //%assetConfigObj.assetIndex = %i;

         //sanetize before modifying our asset name(suffix additions, etc)      
         if(%assetItem.assetName !$= %assetItem.cleanAssetName)
            %assetItem.assetName = %assetItem.cleanAssetName;
            
         //%assetConfigObj.assetName = %assetItem.assetName;
         
         if(%assetItem.assetType $= "AnimationAsset")
         {
            //if we don't have our own file, that means we're gunna be using our parent shape's file so reference that
            if(!isFile(%assetItem.filePath))
            {
               %assetItem.filePath = %assetItem.parentAssetItem.filePath;
            }
         }
         
         if(AssetBrowser.isMethod("prepareImport" @ %assetItem.assetType))
         {
            %command = AssetBrowser @ ".prepareImport" @ %assetItem.assetType @ "(" @ %assetItem @ ");";
            eval(%command);
         }
         
         %assetItem.processed = true;
      }
      
      if(ImportAssetTree.isParentItem(%id))
      {
         %childItem = ImportAssetTree.getChild(%id);
         
         //recurse!
         %this.processNewImportAssets(%childItem); 
      }

      //It's possible we restructured our asset heirarchy(generated assets being parents, etc
      //If that's happened, we need to back out of the current processing and restart to ensure we catch everything
      if(ImportAssetWindow.assetHeirarchyChanged)
         %id = -1;  //breaks the loop
      else
         %id = ImportAssetTree.getNextSibling(%id);
   }
   
   //We have a forced break out of the loop, so lets check if it's because the heirarchy changed.
   //If so, reprocess
   /*if(%id == -1 && ImportAssetWindow.assetHeirarchyChanged)
   {
      ImportAssetWindow.refresh();
   }*/
}

function ImportAssetWindow::findImportingAssetByName(%this, %assetName)
{
   %id = ImportAssetTree.getChild(1);
   
   return %this._findImportingAssetByName(%id, %assetName);
}

function ImportAssetWindow::_findImportingAssetByName(%this, %id, %assetName)
{
   while(%id > 0)
   {
      %assetItem = ImportAssetTree.getItemObject(%id);
      
      if(isObject(%assetItem) && %assetItem.cleanAssetName $= %assetName)
      {
         return %assetItem;
      }
      
      if(ImportAssetTree.isParentItem(%id))
      {
         %childItem = ImportAssetTree.getChild(%id);
         
         //recurse!
         %ret = %this._findImportingAssetByName(%childItem, %assetName);
         if(%ret != 0)
            return %ret;
      }

      %id = ImportAssetTree.getNextSibling(%id);
   }
   
   return 0;
}

function ImportAssetWindow::parseImageSuffixes(%this, %assetItem)
{
   //diffuse
   %suffixCount = getTokenCount(getAssetImportConfigValue("Images/DiffuseTypeSuffixes", ""), ",;");
   for(%sfx = 0; %sfx < %suffixCount; %sfx++)
   {
      %suffixToken = getToken(getAssetImportConfigValue("Images/DiffuseTypeSuffixes", ""), ",;", %sfx);
      if(strIsMatchExpr("*"@%suffixToken, %assetItem.AssetName))
      {
         %assetItem.imageSuffixType = %suffixToken;
         return "diffuse";
      }
   }
   
   //normal
   %suffixCount = getTokenCount(getAssetImportConfigValue("Images/NormalTypeSuffixes", ""), ",;");
   for(%sfx = 0; %sfx < %suffixCount; %sfx++)
   {
      %suffixToken = getToken(getAssetImportConfigValue("Images/NormalTypeSuffixes", ""), ",;", %sfx);
      if(strIsMatchExpr("*"@%suffixToken, %assetItem.AssetName))
      {
         %assetItem.imageSuffixType = %suffixToken;
         return "normal";
      }
   }
   
   //roughness
   %suffixCount = getTokenCount(getAssetImportConfigValue("Images/RoughnessTypeSuffixes", ""), ",;");
   for(%sfx = 0; %sfx < %suffixCount; %sfx++)
   {
      %suffixToken = getToken(getAssetImportConfigValue("Images/RoughnessTypeSuffixes", ""), ",;", %sfx);
      if(strIsMatchExpr("*"@%suffixToken, %assetItem.AssetName))
      {
         %assetItem.imageSuffixType = %suffixToken;
         return "roughness";
      }
   }
   
   //Ambient Occlusion
   %suffixCount = getTokenCount(getAssetImportConfigValue("Images/AOTypeSuffixes", ""), ",;");
   for(%sfx = 0; %sfx < %suffixCount; %sfx++)
   {
      %suffixToken = getToken(getAssetImportConfigValue("Images/AOTypeSuffixes", ""), ",;", %sfx);
      if(strIsMatchExpr("*"@%suffixToken, %assetItem.AssetName))
      {
         %assetItem.imageSuffixType = %suffixToken;
         return "AO";
      }
   }
   
   //metalness
   %suffixCount = getTokenCount(getAssetImportConfigValue("Images/MetalnessTypeSuffixes", ""), ",;");
   for(%sfx = 0; %sfx < %suffixCount; %sfx++)
   {
      %suffixToken = getToken(getAssetImportConfigValue("Images/MetalnessTypeSuffixes", ""), ",;", %sfx);
      if(strIsMatchExpr("*"@%suffixToken, %assetItem.AssetName))
      {
         %assetItem.imageSuffixType = %suffixToken;
         return "metalness";
      }
   }
   
   //composite
   %suffixCount = getTokenCount(getAssetImportConfigValue("Images/CompositeTypeSuffixes", ""), ",;");
   for(%sfx = 0; %sfx < %suffixCount; %sfx++)
   {
      %suffixToken = getToken(getAssetImportConfigValue("Images/CompositeTypeSuffixes", ""), ",;", %sfx);
      if(strIsMatchExpr("*"@%suffixToken, %assetItem.AssetName))
      {
         %assetItem.imageSuffixType = %suffixToken;
         return "composite";
      }
   }
   
   //specular
   /*%suffixCount = getTokenCount(ImportAssetWindow.activeImportConfig.SpecularTypeSuffixes, ",;");
   for(%sfx = 0; %sfx < %suffixCount; %sfx++)
   {
      %suffixToken = getToken(ImportAssetWindow.activeImportConfig.SpecularTypeSuffixes, ",;", %sfx);
      if(strIsMatchExpr("*"@%suffixToken, %assetItem.AssetName))
      {
         %assetItem.imageSuffixType = %suffixToken;
         return "specular";
      }
   }*/
   
   return "";
}

function ImportAssetWindow::parseImagePathSuffixes(%this, %filePath)
{
   //diffuse
   %diffuseSuffixes = getAssetImportConfigValue("Images/DiffuseTypeSuffixes", "");
   %suffixCount = getTokenCount(%diffuseSuffixes, ",;");
   for(%sfx = 0; %sfx < %suffixCount; %sfx++)
   {
      %suffixToken = getToken(%diffuseSuffixes, ",;", %sfx);
      if(strIsMatchExpr("*"@%suffixToken, %filePath))
      {
         return "diffuse";
      }
   }
   
   //normal
   %suffixCount = getTokenCount(ImportAssetWindow.activeImportConfig.NormalTypeSuffixes, ",;");
   for(%sfx = 0; %sfx < %suffixCount; %sfx++)
   {
      %suffixToken = getToken(ImportAssetWindow.activeImportConfig.NormalTypeSuffixes, ",;", %sfx);
      if(strIsMatchExpr("*"@%suffixToken, %filePath))
      {
         return "normal";
      }
   }
   
   //roughness
   %suffixCount = getTokenCount(ImportAssetWindow.activeImportConfig.RoughnessTypeSuffixes, ",;");
   for(%sfx = 0; %sfx < %suffixCount; %sfx++)
   {
      %suffixToken = getToken(ImportAssetWindow.activeImportConfig.RoughnessTypeSuffixes, ",;", %sfx);
      if(strIsMatchExpr("*"@%suffixToken, %filePath))
      {
         return "roughness";
      }
   }
   
   //Ambient Occlusion
   %suffixCount = getTokenCount(ImportAssetWindow.activeImportConfig.AOTypeSuffixes, ",;");
   for(%sfx = 0; %sfx < %suffixCount; %sfx++)
   {
      %suffixToken = getToken(ImportAssetWindow.activeImportConfig.AOTypeSuffixes, ",;", %sfx);
      if(strIsMatchExpr("*"@%suffixToken, %filePath))
      {
         return "AO";
      }
   }
   
   //metalness
   %suffixCount = getTokenCount(ImportAssetWindow.activeImportConfig.MetalnessTypeSuffixes, ",;");
   for(%sfx = 0; %sfx < %suffixCount; %sfx++)
   {
      %suffixToken = getToken(ImportAssetWindow.activeImportConfig.MetalnessTypeSuffixes, ",;", %sfx);
      if(strIsMatchExpr("*"@%suffixToken, %filePath))
      {
         return "metalness";
      }
   }
   
   //composite
   %suffixCount = getTokenCount(ImportAssetWindow.activeImportConfig.CompositeTypeSuffixes, ",;");
   for(%sfx = 0; %sfx < %suffixCount; %sfx++)
   {
      %suffixToken = getToken(ImportAssetWindow.activeImportConfig.CompositeTypeSuffixes, ",;", %sfx);
      if(strIsMatchExpr("*"@%suffixToken, %filePath))
      {
         return "composite";
      }
   }
   
   //specular
   %suffixCount = getTokenCount(ImportAssetWindow.activeImportConfig.SpecularTypeSuffixes, ",;");
   for(%sfx = 0; %sfx < %suffixCount; %sfx++)
   {
      %suffixToken = getToken(ImportAssetWindow.activeImportConfig.SpecularTypeSuffixes, ",;", %sfx);
      if(strIsMatchExpr("*"@%suffixToken, %filePath))
      {
         return "specular";
      }
   }
   
   return "";
}

function refreshImportAssetWindow()
{
   ImportAssetWindow.refresh();  
}

function ImportAssetWindow::refresh(%this)
{
   if(!%this.dirty)
   {
      %this.dirty = true;
      
      %this.schedule(16, "doRefresh");
   }
}

function ImportAssetWindow::doRefresh(%this)
{
   //Go through and process any newly, unprocessed assets
   %id = ImportAssetTree.getChild(1);
   
   ImportAssetWindow.assetHeirarchyChanged = false;
   ImportAssetWindow.importAssetFinalListArray.empty();
   
   %this.processNewImportAssets(%id);
   
   %this.ImportingAssets = 0;
   %this.FetchedAssets = 0;
   %this.prunedDuplicateAssets = 0;
   %this.autoRenamedAssets = 0;
   
   %this.validateAssets();
   
   AssetImportCtrl-->NewAssetsTree.clear();
   AssetImportCtrl-->NewAssetsTree.insertItem(0, "Importing Assets");
   
   if(ImportAssetWindow.importAssetUnprocessedListArray.count() == 0)
   {
      //We've processed them all, prep the assets for actual importing
      //Initial set of assets
      %id = ImportAssetTree.getChild(1);
      
     //recurse!
      %this.refreshChildItem(%id);   
   }
   else
   {
      //Continue processing
      %this.refresh();  
   }
   
   AssetImportCtrl-->NewAssetsTree.buildVisibleTree(true);
   
   %ImportActionSummary = "";
   
   if(%this.ImportingAssets != 0)
   {
      %ImportActionSummary = %ImportActionSummary SPC %this.ImportingAssets @ " Imported|";
   }
   if(%this.FetchedAssets != 0)
   {
      %ImportActionSummary = %ImportActionSummary SPC %this.FetchedAssets @ " Fetched|";
   }
   if(%this.prunedDuplicateAssets != 0)
   {
      %ImportActionSummary = %ImportActionSummary SPC %this.prunedDuplicateAssets @ " Duplicates Pruned|";
   }
   if(%this.autoRenamedAssets != 0)
   {
      %ImportActionSummary = %ImportActionSummary SPC %this.autoRenamedAssets @ " Auto Renamed|";
   }
   
   warn(%ImportActionSummary);
   
   AssetImportSummarization.Text = %ImportActionSummary;
   
   %hasIssues = ImportAssetWindow.validateAssets();
   
   //If we have a valid config file set and we've set to auto-import, and we have no
   //issues for importing, then go ahead and run the import immediately, don't
   //bother showing the window.
   //If any of these conditions fail, we'll display the import window so it can be handled
   //by the user
   if(ImportAssetWindow.importConfigsList.count() != 0 && 
      EditorSettings.value("Assets/AssetImporDefaultConfig") !$= "" && 
      EditorSettings.value("Assets/AutoImport", false) == true
      && %hasIssues == false)
   {
      AssetImportCtrl.setHidden(true);
      ImportAssetWindow.visible = false;
      
      //Go ahead and check if we have any issues, and if not, run the import!
      ImportAssetWindow.ImportAssets();
   }
   else
   {
      //we have assets to import, so go ahead and display the window for that now
      AssetImportCtrl.setHidden(false);
      ImportAssetWindow.visible = true;
      ImportAssetWindow.selectWindow();
   }
   
   if(%hasIssues && getAssetImportConfigValue("General/PreventImportWithErrors", "0") == 1)
   {
      DoAssetImportButton.enabled = false;  
   }
   else
   {
      DoAssetImportButton.enabled = true;  
   }

   // Update object library
   GuiFormManager::SendContentMessage($LBCreateSiderBar, %this, "refreshAll 1");
   
   if(ImportAssetWindow.importConfigsList.count() == 0)
   {
      MessageBoxOK( "Warning", "No base import config. Please create an import configuration set to simplify asset importing.");
   }
   
   %this.dirty = false;
}

function ImportAssetWindow::refreshChildItem(%this, %id)
{
   while (%id > 0)
   {
      %assetItem = ImportAssetTree.getItemObject(%id);
      
      if(!isObject(%assetItem) || %assetItem.skip)
      {
         %id = ImportAssetTree.getNextSibling(%id);
         continue;  
      }
      
      %assetType = %assetItem.assetType;
      %filePath = %assetItem.filePath;
      %assetName = %assetItem.assetName;
      
      //Once validated, attempt any fixes for issues
      %this.resolveIssue(%assetItem);
      
      //create!
      %toolTip = "";
      %configCommand = "ImportAssetOptionsWindow.editImportSettings(" @ %assetItem @ ");";
      
      if(%assetType $= "ShapeAsset" || %assetType $= "AnimationAsset" || %assetType $= "ImageAsset" || %assetType $= "SoundAsset")
      {
         if(%assetItem.status $= "Error")
         {
            %iconIdx = 11;
         }
         else if(%assetItem.status $= "Warning")
         {
            %iconIdx = 9;
         }
         
         %configCommand = "ImportAssetOptionsWindow.fixIssues(" @ %assetItem @ ");";
            
         if(%assetItem.statusType $= "DuplicateAsset" || %assetItem.statusType $= "DuplicateImportAsset")
            %assetName = %assetItem.assetName @ " <Duplicate Asset>";
      }
      else
      {
         if(%assetItem.status $= "Error")
         {
            %iconIdx = 11;
         }
         else if(%assetItem.status $= "Warning")
         {
            %iconIdx = 9;
         }
         
         %configCommand = "";//"ImportAssetOptionsWindow.fixIssues(" @ %assetItem @ ");";
            
            if(%assetItem.statusType $= "DuplicateAsset" || %assetItem.statusType $= "DuplicateImportAsset")
               %assetName = %assetItem.assetName @ " <Duplicate Asset>";
      }
      
      %toolTip = %assetItem.statusInfo;
      %parentItem = ImportAssetTree.getParentItem(%id);
      
      if(%assetItem.status $= "")
      {
         if(%assetType $= "ShapeAsset")
            %iconIdx = 1;
         else if(%assetType $= "MaterialAsset")
            %iconIdx = 3;
         else if(%assetType $= "ImageAsset")
            %iconIdx = 5;
         else if(%assetType $= "SoundAsset")
            %iconIdx = 7;
      }
         
      AssetImportCtrl-->NewAssetsTree.insertItem(%parentItem, %assetName, %assetItem, "", %iconIdx, %iconIdx+1);
      
      ImportAssetWindow.importAssetFinalListArray.add(%assetItem);
      
      if(ImportAssetTree.isParentItem(%id))
      {
         %childItem = ImportAssetTree.getChild(%id);
         
         //recurse!
         %this.refreshChildItem(%childItem); 
      }

      %id = ImportAssetTree.getNextSibling(%id);
   }
}

//
function NewAssetsViewTree::onSelect(%this, %itemId)
{
	if(%itemId == 1)
		//can't select root
		return;
		
   %assetItem = %this.getItemValue(%itemId);
   
   AssetImportCtrl-->NewAssetsInspector.clearFields();
   
   AssetImportCtrl-->NewAssetsInspector.startGroup("General");
   AssetImportCtrl-->NewAssetsInspector.addField("assetName", "Asset Name", "string", "", %assetItem.assetName, "", %assetItem);
   AssetImportCtrl-->NewAssetsInspector.addField("assetType", "Asset Type", "string", "", %assetItem.assetType, "", %assetItem);
   
   if(!%assetItem.generatedAsset)
      AssetImportCtrl-->NewAssetsInspector.addField("filePath", "File Path", "fileName", "", %assetItem.filePath, "", %assetItem);
   
   //AssetImportCtrl-->NewAssetsInspector.addField("assetName", "Asset Name", "string", "", %assetItem.assetName, "", %assetItem);
   //AssetImportCtrl-->NewAssetsInspector.addField("assetName", "Asset Name", "string", "", %assetItem.assetName, "", %assetItem);
   
   AssetImportCtrl-->NewAssetsInspector.addField("status", "Status", "string", "", %assetItem.status, "", %assetItem);
   AssetImportCtrl-->NewAssetsInspector.endGroup();
   
   AssetImportCtrl-->NewAssetsInspector.setFieldEnabled("assetType", false);
   
   if(AssetBrowser.isMethod("inspectImporting" @ %assetItem.assetType))
   {
      %command = "AssetBrowser.inspectImporting" @ %assetItem.assetType @ "(" @ %assetItem @ ");"; 
      eval(%command); 
   }
   //AssetImportCtrl-->NewAssetsInspector.setFieldEnabled("status", false);
   
   /*moduleName = %moduleName;
   dirty  = true;
   parentAssetItem = %parentAssetItem;
   status = "";
   statusType = "";
   statusInfo = "";
   skip = false;
   processed = false;
   generatedAsset = false;*/
}

function NewAssetsViewTree::onRightMouseDown(%this, %itemId)
{
   ImportAssetActions.enableItem(1, true);
   
   if( %itemId != 1 && %itemId != -1)
   {
      %assetItem = %this.getItemValue(%itemId);
      
      if(%assetItem.assetType $= "MaterialAsset")
      {
         %contextPopup = ImportAssetMaterialMaps;
         
         for(%i=0; %i < 7; %i++)
         {
            %contextPopup.enableItem(%i, true);
         }
         
         if(isObject(%assetItem.diffuseImageAsset))
            %contextPopup.enableItem(0, false);
            
         if(isObject(%assetItem.normalImageAsset))
            %contextPopup.enableItem(1, false);
            
         if(isObject(%assetItem.compositeImageAsset))
            %contextPopup.enableItem(2, false);
      }
      else
      {
         %contextPopup = ImportAssetActions;  
      }
      %contextPopup.showPopup(Canvas);
      %contextPopup.assetItem = %assetItem;
      %contextPopup.itemId = %itemId;
   }
   else
   {
      ImportAssetActions.showPopup(Canvas);
   }
}

function NewAssetsPanelInputs::onRightMouseDown(%this)
{
   NewAssetsViewTree::onRightMouseDown(0, -1);
}

//
function ImportAssetWindow::removeImportingAsset(%this)
{
   ImportActivityLog.add("Removing Asset from Import");
   
   ImportAssetTree.removeAllChildren(ImportAssetActions.itemId);
   ImportAssetTree.removeItem(ImportAssetActions.itemId);
   
   ImportAssetWindow.refresh();
}

function ImportAssetWindow::addNewImportingAsset(%this, %filterType)
{
   %filter = "Any Files (*.*)|*.*|";
   
   if(%filterType $= "Sound" || %filterType $= "")
      %filter = "Sound Files(*.wav, *.ogg)|*.wav;*.ogg|" @ %filter;
   if(%filterType $= "Image" || %filterType $= "")
      %filter = "Images Files(*.jpg,*.png,*.tga,*.bmp,*.dds)|*.jpg;*.png;*.tga;*.bmp;*.dds|" @ %filter;
   if(%filterType $= "Shape" || %filterType $= "")
      %filter = "Shape Files(*.dae, *.cached.dts)|*.dae;*.cached.dts|" @ %filter;
      
   //get our item depending on which action we're trying for
   if(ImportAssetActions.visible)
      %parentAssetItem = ImportAssetActions.assetItem;
   else if(ImportAssetMaterialMaps.visible)
      %parentAssetItem = ImportAssetMaterialMaps.assetItem;
      
   %defaultPath = filePath(%parentAssetItem.filePath) @ "/";
      
   %dlg = new OpenFileDialog()
   {
      Filters = %filter;
      DefaultFile = %defaultPath;
      ChangePath = false;
      MustExist = true;
      MultipleFiles = false;
      forceRelativePath = false;
   };
      
   if ( %dlg.Execute() )
   {
      %filePath = %dlg.FileName;
   }
   
   %dlg.delete();
   
   if(%filePath $= "")
      return "";
   
   //AssetBrowser.onDropFile( %path );
   
   %fileExt = fileExt( %filePath );
   //add it to our array!
   if(isImageFormat(%fileExt))
      %type = "ImageAsset";
   else if( isShapeFormat(%fileExt))
      %type = "ShapeAsset";
   else if( isSoundFormat(%fileExt))
      %type = "SoundAsset";
   else if( %fileExt $= ".cs" || %fileExt $= ".cs.dso" )
      %type = "ScriptAsset";
   else if( %fileExt $= ".gui" || %fileExt $= ".gui.dso" )
      %type = "GUIAsset";
      
   %newAssetItem = AssetBrowser.addImportingAsset(%type, %filePath, %parentAssetItem);
      
   //Used to keep tabs on what files we were trying to import, used mainly in the event of
   //adjusting configs and needing to completely reprocess the import
   //ensure we're not doubling-up on files by accident
   if(%this.importingFilesArray.getIndexFromKey(%filePath) == -1)
      %this.importingFilesArray.add(%filePath);
         
   AssetBrowser.onEndDropFiles();
   
   return %newAssetItem;
}

function ImportAssetWindow::addMaterialMap(%this, %map)
{
   %newAssetItem = %this.addNewImportingAsset("Image");
   
   %newAssetItem.ImageType = %map;
}

//
function ImportAssetWindow::importResolution(%this, %assetItem)
{
   if(%assetItem.status !$= "Error" && %assetItem.status !$= "Warning")
   {
      //If nothing's wrong, we just edit it
      ImportAssetOptionsWindow.editImportSettings(%assetItem);
      return;
   }
   else
   {
      ImportAssetResolutionsPopup.assetItem = %assetItem;
      if(%assetItem.statusType $= "DuplicateAsset" || %assetItem.statusType $= "DuplicateImportAsset")
      {
         ImportAssetResolutionsPopup.enableItem(3, false); //Rename
         ImportAssetResolutionsPopup.enableItem(5, false); //Find Missing
      }
      else if(%assetItem.statusType $= "MissingFile")
      {
         ImportAssetResolutionsPopup.enableItem(0, false); //Use Orig
         ImportAssetResolutionsPopup.enableItem(1, false); //Use Dupe
         ImportAssetResolutionsPopup.enableItem(3, false); //Rename
      }
   }
   
   ImportAssetResolutionsPopup.showPopup(Canvas);  
}

//
function ImportAssetWindow::validateAssets(%this)
{
   //Clear any status
   %this.resetAssetsValidationStatus();
   
   ImportAssetWindow.importIssues = false;
   
   %id = ImportAssetTree.getChild(1);
   %hasIssues = %this.validateAsset(%id);
   
   if(ImportAssetWindow.importIssues == false)
      return false;
   else
      return true;
}

function ImportAssetWindow::validateAsset(%this, %id)
{
   %moduleName = AssetImportTargetModule.getText();
   
   while (%id > 0)
   {
      %assetItem = ImportAssetTree.getItemObject(%id);
      
      if(!isObject(%assetItem) || %assetItem.skip)
      {
         %id = ImportAssetTree.getNextSibling(%id);
         continue;  
      }
      
      //First, check the obvious: name collisions. We should have no asset that shares a similar name.
      //If we do, prompt for it be renamed first before continuing
      %hasCollision = %this.checkAssetsForCollision(%assetItem);
      
      //Ran into a problem, so end checks on this one and move on
      if(%hasCollision)
      {
         %id = ImportAssetTree.getNextSibling(%id);
         continue;  
      }
      
      //No collisions of for this name in the importing assets. Now, check against the existing assets in the target module
      if(!AssetBrowser.isAssetReImport)
      {
         %assetQuery = new AssetQuery();
         
         %numAssetsFound = AssetDatabase.findAllAssets(%assetQuery);

         %foundCollision = false;
         for( %f=0; %f < %numAssetsFound; %f++)
         {
            %assetId = %assetQuery.getAsset(%f);
             
            //first, get the asset's module, as our major categories
            %module = AssetDatabase.getAssetModule(%assetId);
            
            %testModuleName = %module.moduleId;
            
            //These are core, native-level components, so we're not going to be messing with this module at all, skip it
            if(%moduleName !$= %testModuleName)
               continue;

            %testAssetName = AssetDatabase.getAssetName(%assetId);
            
            if(%testAssetName $= %assetItem.assetName)
            {
               %foundCollision = true;
               
               %assetItem.status = "error";
               %assetItem.statusType = "DuplicateAsset";
               %assetItem.statusInfo = "Duplicate asset names found with the target module!\nAsset \"" @ 
               %assetItem.assetName @ "\" of type \"" @ %assetItem.assetType @ "\" has a matching name.\nPlease rename it and try again!";
                  
               ImportActivityLog.add("Error! Asset " @ %assetItem.assetName @ " has an identically named asset in the target module");

               break;
            }
         }
         
         if(%foundCollision == true)
         {
            //yup, a collision, prompt for the change and bail out
            /*MessageBoxOK( "Error!", "Duplicate asset names found with the target module!\nAsset \"" @ 
               %assetItemA.assetName @ "\" of type \"" @ %assetItemA.assetType @ "\" has a matching name.\nPlease rename it and try again!");*/
               
            //%assetQuery.delete();
            //return false;
         }
         
         //Clean up our queries
         %assetQuery.delete();
      }
         
      //Check if we were given a file path(so not generated) but somehow isn't a valid file
      if(%assetItem.filePath !$= ""  && !%assetItem.generatedAsset && !isFile(%assetItem.filePath))
      {
         %assetItem.status = "error";
         %assetItem.statusType = "MissingFile";
         %assetItem.statusInfo = "Unable to find file to be imported. Please select asset file.";
         
         ImportActivityLog.add("Error! Asset " @ %assetItem.filePath @ " was not found");
      }
      
      if(%assetItem.status $= "Warning")
      {
         if(getAssetImportConfigValue("General/WarningsAsErrors", "0") == 1)
         {
            %assetItem.status = "error";
            
            ImportActivityLog.add("Warnings treated as errors!");
         }
      }
      
      if(%assetItem.status $= "error")
         ImportAssetWindow.importIssues = true;
      
      if(ImportAssetTree.isParentItem(%id))
      {
         %childItem = ImportAssetTree.getChild(%id);
         
         //recurse!
         %this.validateAsset(%childItem); 
      }

      %id = ImportAssetTree.getNextSibling(%id);
   }
}


function ImportAssetWindow::resetAssetsValidationStatus(%this)
{
   %id = ImportAssetTree.getChild(1);
   
   %this.resetAssetValidationStatus(%id);
}

function ImportAssetWindow::resetAssetValidationStatus(%this, %id)
{
   %moduleName = AssetImportTargetModule.getText();
  
   %id = ImportAssetTree.getChild(%id);
   while (%id > 0)
   {
      %assetItem = ImportAssetTree.getItemObject(%id);
      
      if(!isObject(%assetItem) || %assetItem.skip)
      {
         %id = ImportAssetTree.getNextSibling(%id);
         continue;  
      }
      
      %assetItem.status = "";
      %assetItem.statusType = "";
      %assetItem.statusInfo = "";

      if(ImportAssetTree.isParentItem(%id))
      {
         %childItem = ImportAssetTree.getChild(%id);
         
         //recurse!
         %this.resetAssetValidationStatus(%childItem); 
      }

      %id = ImportAssetTree.getNextSibling(%id);
   }
}

function ImportAssetWindow::checkAssetsForCollision(%this, %assetItem)
{
   %id = ImportAssetTree.getChild(1);
   
   return %this.checkAssetForCollision(%assetItem, %id);
}

function ImportAssetWindow::checkAssetForCollision(%this, %assetItem, %id)
{
   %moduleName = AssetImportTargetModule.getText();
  
   %id = ImportAssetTree.getChild(%id);
   while (%id > 0)
   {
      %assetItemB = ImportAssetTree.getItemObject(%id);
      
      if(!isObject(%assetItemB) || %assetItemB.skip)
      {
         %id = ImportAssetTree.getNextSibling(%id);
         continue;  
      }
   
      if( (%assetItem.assetName $= %assetItemB.assetName) && (%assetItem.getId() != %assetItemB.getId()) )
      {
         //yup, a collision, prompt for the change and bail out
         %assetItem.status = "Warning";
         %assetItem.statusType = "DuplicateImportAsset";
         %assetItem.statusInfo = "Duplicate asset names found with importing assets!\nAsset \"" @ 
            %assetItemB.assetName @ "\" of type \"" @ %assetItemB.assetType @ "\" and \"" @
            %assetItem.assetName @ "\" of type \"" @ %assetItem.assetType @ "\" have matching names.\nPlease rename one of them and try again!";
            
         ImportActivityLog.add("Warning! Asset " @ %assetItem.assetName @ ", type " @ %assetItem.assetType @ " has a naming collisions with asset " @ %assetItemB.assetName @ ", type " @ %assetItemB.assetType);

         return true;
      }
      
      if(ImportAssetTree.isParentItem(%id))
      {
         %childItem = ImportAssetTree.getChild(%id);
         
         //recurse!
         %this.checkAssetForCollision(%assetItem, %childItem); 
      }

      %id = ImportAssetTree.getNextSibling(%id);
   }
   
   return false;
}

//
function ImportAssetWindow::deleteImportingAsset(%this, %assetItem)
{
   %item = ImportAssetTree.findItemByObjectId(%assetItem);
   
   ImportActivityLog.add("Deleting Importing Asset " @ %assetItem.assetName @ " and all it's child items");
   
   ImportAssetTree.removeAllChildren(%item);
   ImportAssetTree.removeItem(%item);

   schedule(10, 0, "refreshImportAssetWindow");
   //ImportAssetWindow.refresh();
   //ImportAssetOptionsWindow.setVisible(0);
}

//
function ImportAssetWindow::ImportAssets(%this)
{
   //do the actual importing, now!
   %assetCount = ImportAssetWindow.importAssetFinalListArray.count();
   
   //get the selected module data
   %moduleName = AssetImportTargetModule.getText();
   
   %module = ModuleDatabase.findModule(%moduleName, 1);
   
   if(!isObject(%module))
   {
      MessageBoxOK( "Error!", "No module selected. You must select or create a module for the assets to be added to.");
      return;
   }
   
   %id = ImportAssetTree.getChild(1);
   
   %this.doImportAssets(%id);
   
   //force an update of any and all modules so we have an up-to-date asset list
   AssetBrowser.refresh();
   Canvas.popDialog(AssetImportCtrl);
   AssetBrowser.isAssetReImport = false;
}

function ImportAssetWindow::doImportAssets(%this, %id)
{
   while(%id > 0)
   {
      %assetItem = ImportAssetTree.getItemObject(%id);
      
      if(!isObject(%assetItem) || %assetItem.skip)
      {
         %id = ImportAssetTree.getNextSibling(%id);
         continue;  
      }
      
      %assetType = %assetItem.AssetType;
      %filePath = %assetItem.filePath;
      %assetName = %assetItem.assetName;
      %assetImportSuccessful = false;
      %assetId = %moduleName@":"@%assetName;
      
      if(%assetType $= "ImageAsset")
      {
         AssetBrowser.importImageAsset(%assetItem);
      }
      else if(%assetType $= "ShapeAsset")
      {
         AssetBrowser.importShapeAsset(%assetItem);
      }
      else if(%assetType $= "AnimationAsset")
      {
         %assetPath = "data/" @ %moduleName @ "/ShapeAnimations";
         %assetFullPath = %assetPath @ "/" @ fileName(%filePath);
         
         %newAsset = new ShapeAnimationAsset()
         {
            assetName = %assetName;
            versionId = 1;
            fileName = %assetFullPath;
            originalFilePath = %filePath;
            animationFile = %assetFullPath;
            animationName = %assetName;
            startFrame = 0;
            endFrame = -1;
            padRotation = false;
            padTransforms = false;
         };

         %assetImportSuccessful = TAMLWrite(%newAsset, %assetPath @ "/" @ %assetName @ ".asset.taml"); 
         
         //and copy the file into the relevent directory
         %doOverwrite = !AssetBrowser.isAssetReImport;
         if(!pathCopy(%filePath, %assetFullPath, %doOverwrite))
         {
            error("Unable to import asset: " @ %filePath);
         }
      }
      else if(%assetType $= "SoundAsset")
      {
         %assetPath = "data/" @ %moduleName @ "/Sounds";
         %assetFullPath = %assetPath @ "/" @ fileName(%filePath);
         
         %newAsset = new SoundAsset()
         {
            assetName = %assetName;
            versionId = 1;
            fileName = %assetFullPath;
            originalFilePath = %filePath;
         };
         
         %assetImportSuccessful = TAMLWrite(%newAsset, %assetPath @ "/" @ %assetName @ ".asset.taml"); 
         
         //and copy the file into the relevent directory
         %doOverwrite = !AssetBrowser.isAssetReImport;
         if(!pathCopy(%filePath, %assetFullPath, %doOverwrite))
         {
            error("Unable to import asset: " @ %filePath);
         }
      }
      else if(%assetType $= "MaterialAsset")
      {
         AssetBrowser.importMaterialAsset(%assetItem);
      }
      else if(%assetType $= "ScriptAsset")
      {
         %assetPath = "data/" @ %moduleName @ "/Scripts";
         %assetFullPath = %assetPath @ "/" @ fileName(%filePath);
         
         %newAsset = new ScriptAsset()
         {
            assetName = %assetName;
            versionId = 1;
            scriptFilePath = %assetFullPath;
            isServerSide = true;
            originalFilePath = %filePath;
         };
         
         %assetImportSuccessful = TAMLWrite(%newAsset, %assetPath @ "/" @ %assetName @ ".asset.taml"); 
         
         //and copy the file into the relevent directory
         %doOverwrite = !AssetBrowser.isAssetReImport;
         if(!pathCopy(%filePath, %assetFullPath, %doOverwrite))
         {
            error("Unable to import asset: " @ %filePath);
         }
      }
      else if(%assetType $= "GUIAsset")
      {
         %assetPath = "data/" @ %moduleName @ "/GUIs";
         %assetFullPath = %assetPath @ "/" @ fileName(%filePath);
         
         %newAsset = new GUIAsset()
         {
            assetName = %assetName;
            versionId = 1;
            GUIFilePath = %assetFullPath;
            scriptFilePath = "";
            originalFilePath = %filePath;
         };
         
         %assetImportSuccessful = TAMLWrite(%newAsset, %assetPath @ "/" @ %assetName @ ".asset.taml"); 
         
         //and copy the file into the relevent directory
         %doOverwrite = !AssetBrowser.isAssetReImport;
         if(!pathCopy(%filePath, %assetFullPath, %doOverwrite))
         {
            error("Unable to import asset: " @ %filePath);
         }
      }
      
      if(%assetImportSuccessful)
      {
         %moduleDef = ModuleDatabase.findModule(%moduleName,1);
         
         if(!AssetBrowser.isAssetReImport)
            AssetDatabase.addDeclaredAsset(%moduleDef, %assetPath @ "/" @ %assetName @ ".asset.taml");
         else
            AssetDatabase.refreshAsset(%assetId);
      }
      
      if(ImportAssetTree.isParentItem(%id))
      {
         %childItem = ImportAssetTree.getChild(%id);
         
         //recurse!
         %this.doImportAssets(%childItem); 
      }

      %id = ImportAssetTree.getNextSibling(%id);
   }
}

function ImportAssetWindow::resolveIssue(%this, %assetItem)
{
   //Ok, we actually have a warning, so lets resolve
   if(%assetItem.statusType $= "DuplicateImportAsset" || %assetItem.statusType $= "DuplicateAsset")
   {
      %resolutionAction = getAssetImportConfigValue("General/DuplicatAutoResolution", "AutoPrune");
      
      %humanReadableStatus = %assetItem.statusType $= "DuplicateImportAsset" ? "Duplicate Import Asset" : "Duplicate Asset";
      
      if(%resolutionAction $= "AutoPrune")
      {
         %this.deleteImportingAsset(%assetItem);
         %this.prunedDuplicateAssets++;
         
         ImportActivityLog.add("Asset " @ %assetItem.assetName @ " was Autopruned due to " @ %humanReadableStatus);
      }
      else if(%resolutionAction $= "AutoRename")
      {
         ImportActivityLog.add("Asset " @ %assetItem.assetName @ " was Auto-Renamed due to " @ %humanReadableStatus);
         
         %noNum = stripTrailingNumber(%assetItem.assetName);
         %num = getTrailingNumber(%assetItem.assetName);
         
         if(%num == -1)
         {
            %assetItem.assetName = %noNum @ "1";  
         }
         else
         {
            %num++;
            %assetItem.assetName = %noNum @ %num; 
         }
         
         ImportActivityLog.add("   New name is " @ %assetItem.assetName);
         
         %this.autoRenamedAssets++;
      }
   }
   else if(%assetItem.statusType $= "MissingFile")
   {
      if(getAssetImportConfigValue("General/AutomaticallyPromptMissingFiles", "0") == 1)
      {
         %this.findMissingFile(%assetItem);
      }
   }
}

function ImportAssetWindow::findMissingFile(%this, %assetItem)
{
   if(%assetItem.assetType $= "ShapeAsset")
      %filters = "Shape Files(*.dae, *.cached.dts)|*.dae;*.cached.dts";
   else if(%assetItem.assetType $= "ImageAsset")
      %filters = "Images Files(*.jpg,*.png,*.tga,*.bmp,*.dds)|*.jpg;*.png;*.tga;*.bmp;*.dds";
      
   %dlg = new OpenFileDialog()
   {
      Filters        = %filters;
      DefaultPath    = $Pref::WorldEditor::LastPath;
      DefaultFile    = "";
      ChangePath     = true;
      OverwritePrompt = true;
      forceRelativePath = false;
      fileName="";
      //MultipleFiles = true;
   };

   %ret = %dlg.Execute();
   
   if ( %ret )
   {
      $Pref::WorldEditor::LastPath = filePath( %dlg.FileName );
      %fullPath = %dlg.FileName;//makeRelativePath( %dlg.FileName, getMainDotCSDir() );
   }   
   
   %dlg.delete();
   
   if ( !%ret )
      return;
      
   %assetItem.filePath = %fullPath;
   %assetItem.assetName = fileBase(%assetItem.filePath);
   
   if(%assetItem.assetType $= "ImageAsset")
   {
      //See if we have anything important to update for our material parent(if we have one)
      %treeItem = ImportAssetTree.findItemByObjectId(%assetItem);
      %parentItem = ImportAssetTree.getParentItem(%treeItem);
      
      if(%parentItem != 0)
      {
         %parentAssetItem = ImportAssetTree.getItemObject(%parentItem);
         if(%parentAssetItem.assetType $= "MaterialAsset")
         {
            AssetBrowser.prepareImportMaterialAsset(%parentAssetItem);              
         }
      }
   }
   
   ImportAssetWindow.refresh();
}
//

//
function ImportAssetWindow::toggleLogWindow()
{
   if(AssetBrowserImportLog.isAwake())
   {
      Canvas.popDialog(AssetBrowserImportLog);
      return;
   }
   else
   {
      Canvas.pushDialog(AssetBrowserImportLog);
   }
      
   ImportLogTextList.clear();
   for(%i=0; %i < ImportActivityLog.count(); %i++)
   {
      ImportLogTextList.addRow(%i, ImportActivityLog.getKey(%i));
   }
}
//

//
function ImportAssetModuleList::onWake(%this)
{
   %this.refresh();
}

function ImportAssetModuleList::refresh(%this)
{
   %this.clear();
   
   //First, get our list of modules
   %moduleList = ModuleDatabase.findModules();
   
   %count = getWordCount(%moduleList);
   for(%i=0; %i < %count; %i++)
   {
      %moduleName = getWord(%moduleList, %i);
      %this.add(%moduleName.ModuleId, %i);  
   }
}
//

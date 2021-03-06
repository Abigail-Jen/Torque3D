function AssetBrowser::prepareImportImageAsset(%this, %assetItem)
{
   if(getAssetImportConfigValue("Images/GenerateMaterialOnImport", "1") == 1 && %assetItem.parentAssetItem $= "")
   {
      //First, see if this already has a suffix of some sort based on our import config logic. Many content pipeline tools like substance automatically appends them
      %foundSuffixType = ImportAssetWindow.parseImageSuffixes(%assetItem);
      
      if(%foundSuffixType $= "")
      {
         %noSuffixName = %assetItem.AssetName;
      }
      else
      {
         %suffixPos = strpos(strlwr(%assetItem.AssetName), strlwr(%assetItem.imageSuffixType), 0);
         %noSuffixName = getSubStr(%assetItem.AssetName, 0, %suffixPos);
      }
   
      //Check if our material already exists
      //First, lets double-check that we don't already have an
      %materialAsset = ImportAssetWindow.findImportingAssetByName(%noSuffixName);
      %cratedNewMaterial = false;
      
      if(%materialAsset == 0)
      {
         %filePath = %assetItem.filePath;
         if(%filePath !$= "")
            %materialAsset = AssetBrowser.addImportingAsset("MaterialAsset", "", "", %noSuffixName);
            
         %materialAsset.filePath = filePath(%assetItem.filePath) @ "/" @ %noSuffixName;
            
         %cratedNewMaterial = true;
      }
      
      if(isObject(%materialAsset))
      {
         //Establish parentage
         %itemId = ImportAssetTree.findItemByObjectId(%assetItem);
         %materialItemId = ImportAssetTree.findItemByObjectId(%materialAsset);
         
         %assetItem.parentId = %materialItemId;
         %assetItem.parentAssetItem = %materialAsset;
         
         ImportAssetTree.reparentItem(%itemId, %materialItemId);
         
         ImportAssetWindow.assetHeirarchyChanged = true;
         
         ImportAssetTree.buildVisibleTree(true);
      }
      
      //Lets do some cleverness here. If we're generating a material we can parse like assets being imported(similar file names) but different suffixes
      //if we find these, we'll just populate into the original's material
      
      //If we need to append the diffuse suffix and indeed didn't find a suffix on the name, do that here
      if(%foundSuffixType $= "")
      {
         if(getAssetImportConfigValue("Images/UseDiffuseSuffixOnOriginImg", "1") == 1)
         {
            if(%foundSuffixType $= "")
            {
               %diffuseToken = getToken(getAssetImportConfigValue("Images/DiffuseTypeSuffixes", ""), ",", 0);
               %assetItem.AssetName = %assetItem.AssetName @ %diffuseToken;
            }
         }
         else
         {
            //We need to ensure that our image asset doesn't match the same name as the material asset, so if we're not trying to force the diffuse suffix
            //we'll give it a generic one
            if(%materialAsset.assetName $= %assetItem.assetName)
            {
               %assetItem.AssetName = %assetItem.AssetName @ "_image";
            }
         }
         
         %foundSuffixType = "diffuse";
      }
      
      if(%foundSuffixType !$= "")
      {
         //otherwise, if we have some sort of suffix, we'll want to figure out if we've already got an existing material, and should append to it  
         
         if(getAssetImportConfigValue("Materials/PopulateMaterialMaps", "1") == 1)
         {
            if(%foundSuffixType $= "diffuse")
               %assetItem.ImageType = "Abledo";
            else if(%foundSuffixType $= "normal")
               %assetItem.ImageType = "Normal";
            else if(%foundSuffixType $= "metalness")
               %assetItem.ImageType = "metalness";
            else if(%foundSuffixType $= "roughness")
               %assetItem.ImageType = "roughness";
            else if(%foundSuffixType $= "specular")
               %assetItem.ImageType = "specular";
            else if(%foundSuffixType $= "AO")
               %assetItem.ImageType = "AO";
            else if(%foundSuffixType $= "composite")
               %assetItem.ImageType = "composite";
         }
      }
      
      //If we JUST created this material, we need to do a process pass on it to do any other setup for it
      if(%cratedNewMaterial)
      {
         AssetBrowser.prepareImportMaterialAsset(%materialAsset);
      }
   }
   
   %assetItem.processed = true;
}

function AssetBrowser::inspectImportingImageAsset(%this, %assetItem)
{
   AssetImportCtrl-->NewAssetsInspector.startGroup("Image");
   AssetImportCtrl-->NewAssetsInspector.addField("ImageType", "Image Type", "list", "Intended usage case of this image. Used to map to material slots and set up texture profiles.", "GUI", 
                                                      "Albedo,Normal,Composite,Roughness,AO,Metalness,Glow,GUI,Particle,Decal", %assetItem);
                                                      
   AssetImportCtrl-->NewAssetsInspector.endGroup();                                                
}

function AssetBrowser::importImageAsset(%this, %assetItem)
{
   %moduleName = AssetImportTargetModule.getText();
   
   %assetType = %assetItem.AssetType;
   %filePath = %assetItem.filePath;
   %assetName = %assetItem.assetName;
   %assetImportSuccessful = false;
   %assetId = %moduleName@":"@%assetName;
   
   %assetPath = AssetBrowser.dirHandler.currentAddress @ "/";
   
   %assetFullPath = %assetPath @ "/" @ fileName(%filePath);
   
   %newAsset = new ImageAsset()
   {
      assetName = %assetName;
      versionId = 1;
      imageFile = fileName(%filePath);
      originalFilePath = %filePath;
   };
   
   %assetImportSuccessful = TAMLWrite(%newAsset, %assetPath @ "/" @ %assetName @ ".asset.taml"); 
   
   //and copy the file into the relevent directory
   %doOverwrite = !AssetBrowser.isAssetReImport;
   if(!pathCopy(%filePath, %assetFullPath, %doOverwrite))
   {
      error("Unable to import asset: " @ %filePath);
      return;
   }
   
   %moduleDef = ModuleDatabase.findModule(%moduleName,1);
         
   if(!AssetBrowser.isAssetReImport)
      AssetDatabase.addDeclaredAsset(%moduleDef, %assetPath @ "/" @ %assetName @ ".asset.taml");
   else
      AssetDatabase.refreshAsset(%assetId);
}

function AssetBrowser::buildImageAssetPreview(%this, %assetDef, %previewData)
{
   %previewData.assetName = %assetDef.assetName;
   %previewData.assetPath = %assetDef.scriptFile;
   //%previewData.doubleClickCommand = "EditorOpenFileInTorsion( "@%previewData.assetPath@", 0 );";
   
   %imageFilePath = %assetDef.getImageFilename();
   if(isFile(%imageFilePath))
      %previewData.previewImage = %imageFilePath;
   else
      %previewData.previewImage = "core/rendering/images/unavailable";
   
   %previewData.assetFriendlyName = %assetDef.assetName;
   %previewData.assetDesc = %assetDef.description;
   %previewData.tooltip = %assetDef.friendlyName @ "\n" @ %assetDef;
}

//Renames the asset
function AssetBrowser::renameImageAsset(%this, %assetDef, %newAssetName)
{
   %newFilename = renameAssetLooseFile(%assetDef.imageFile, %newAssetName);
   
   if(!%newFilename $= "")
      return;

   %assetDef.imageFile = %newFilename;
   %assetDef.saveAsset();
   
   renameAssetFile(%assetDef, %newAssetName);
}

//Deletes the asset
function AssetBrowser::deleteImageAsset(%this, %assetDef)
{
   AssetDatabase.deleteAsset(%assetDef.getAssetId(), true);
}

//Moves the asset to a new path/module
function AssetBrowser::moveImageAsset(%this, %assetDef, %destination)
{
   %currentModule = AssetDatabase.getAssetModule(%assetDef.getAssetId());
   %targetModule = AssetBrowser.getModuleFromAddress(%destination);
   
   %newAssetPath = moveAssetFile(%assetDef, %destination);
   
   if(%newAssetPath $= "")
      return false;

   moveAssetLooseFile(%assetDef.imageFile, %destination);
   
   AssetDatabase.removeDeclaredAsset(%assetDef.getAssetId());
   AssetDatabase.addDeclaredAsset(%targetModule, %newAssetPath);
}

function GuiInspectorTypeImageAssetPtr::onControlDropped( %this, %payload, %position )
{
   Canvas.popDialog(EditorDragAndDropLayer);
   
   // Make sure this is a color swatch drag operation.
   if( !%payload.parentGroup.isInNamespaceHierarchy( "AssetPreviewControlType_AssetDrop" ) )
      return;

   %assetType = %payload.dragSourceControl.parentGroup.assetType;
   
   if(%assetType $= "ImageAsset")
   {
      echo("DROPPED A IMAGE ON AN IMAGE ASSET COMPONENT FIELD!");  
   }
   
   EWorldEditor.isDirty = true;
}
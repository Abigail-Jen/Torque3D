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

#include "gui/editor/inspector/variableInspector.h"
#include "console/engineAPI.h"

GuiVariableInspector::GuiVariableInspector()
{
}

GuiVariableInspector::~GuiVariableInspector()
{
}

IMPLEMENT_CONOBJECT(GuiVariableInspector);

ConsoleDocClass( GuiVariableInspector,
   "@brief GUI dedicated to variable viewing/manipulation\n\n"
   "Mostly used in console system, internal use only.\n\n"
   "@internal"
);

void GuiVariableInspector::loadVars( String searchStr )
{     
   clearGroups();

   GuiInspectorVariableGroup *group = new GuiInspectorVariableGroup();

   group->setHeaderHidden( true );
   group->setCanCollapse( false );
   group->mParent = this;
   group->setCaption( "Global Variables" );
   group->mSearchString = searchStr;

   group->registerObject();
   mGroups.push_back( group );
   addObject( group );
 
   group->inspectGroup();
}

void GuiVariableInspector::update()
{
   clearGroups();

   for (U32 i = 0; i < mFields.size(); i++)
   {
      //first, get the var's group name. if the group exists, we'll add to it's list
      GuiInspectorVariableGroup *group = nullptr;

      for (U32 g = 0; g < mGroups.size(); g++)
      {
         if (mGroups[g]->getCaption().equal(mFields[i].mGroup))
         {
            group = static_cast<GuiInspectorVariableGroup*>(mGroups[g]);
            break;
         }
      }

      if (group == nullptr)
      {
         group = new GuiInspectorVariableGroup();

         group->setHeaderHidden(false);
         group->setCanCollapse(true);
         group->mParent = this;
         group->setCaption(mFields[i].mGroup);

         group->registerObject();
         mGroups.push_back(group);
         addObject(group);
      }
      
      group->addField(&mFields[i]);
   }

   //And now, cue our update for the groups themselves
   for (U32 g = 0; g < mGroups.size(); g++)
   {
      mGroups[g]->inspectGroup();
   }
}

void GuiVariableInspector::addField(const char* name, const char* label, const char* typeName, const char* description, const char* defaultValue, SimObject* ownerObj, const char* groupName)
{
   VariableField newField;
   newField.mFieldName = StringTable->insert(name);
   newField.mFieldLabel = StringTable->insert(label);
   newField.mFieldTypeName = StringTable->insert(typeName);
   newField.mFieldDescription = StringTable->insert(description);
   newField.mDefaultValue = StringTable->insert(defaultValue);
   newField.mGroup = StringTable->insert(groupName);

   newField.mOwnerObject = ownerObj;

   //
   //find the field type
   S32 fieldTypeMask = -1;

   if (newField.mFieldTypeName == StringTable->insert("int"))
      fieldTypeMask = TypeS32;
   else if (newField.mFieldTypeName == StringTable->insert("float"))
      fieldTypeMask = TypeF32;
   else if (newField.mFieldTypeName == StringTable->insert("vector"))
      fieldTypeMask = TypePoint3F;
   //else if (fieldType == StringTable->insert("material"))
   //   fieldTypeMask = TypeMaterialName;
   else if (newField.mFieldTypeName == StringTable->insert("image"))
      fieldTypeMask = TypeImageFilename;
   else if (newField.mFieldTypeName == StringTable->insert("shape"))
      fieldTypeMask = TypeShapeFilename;
   else if (newField.mFieldTypeName == StringTable->insert("bool"))
      fieldTypeMask = TypeBool;
   else if (newField.mFieldTypeName == StringTable->insert("object"))
      fieldTypeMask = TypeSimObjectPtr;
   else if (newField.mFieldTypeName == StringTable->insert("string"))
      fieldTypeMask = TypeString;
   else if (newField.mFieldTypeName == StringTable->insert("colorI"))
      fieldTypeMask = TypeColorI;
   else if (newField.mFieldTypeName == StringTable->insert("colorF"))
      fieldTypeMask = TypeColorF;
   else if (newField.mFieldTypeName == StringTable->insert("ease"))
      fieldTypeMask = TypeEaseF;
   else
      fieldTypeMask = -1;

   newField.mFieldType = fieldTypeMask;
   //

   mFields.push_back(newField);

   update();
}

void GuiVariableInspector::clearFields()
{
   mFields.clear();
   update();
}

void GuiVariableInspector::setFieldEnabled(const char* name, bool enabled)
{
   for (U32 i = 0; i < mFields.size(); i++)
   {
      if (mFields[i].mFieldName == name)
      {
         mFields[i].mEnabled = enabled;
         return;
      }
   }

   update();
}

DefineConsoleMethod(GuiVariableInspector, addField, void, (const char* name, const char* label, const char* typeName, const char* description, const char* defaultValue, SimObject* ownerObj, const char* groupName), 
   ("","","","","", nullAsType<SimObject*>(), ""), "loadVars( searchString )")
{
   if (name == "" || typeName == "")
      return;

   object->addField(name, label, typeName, description, defaultValue, ownerObj, groupName);
}

DefineConsoleMethod(GuiVariableInspector, update, void, (), , "loadVars( searchString )")
{
   object->update();
}

DefineConsoleMethod(GuiVariableInspector, clearFields, void, (), , "loadVars( searchString )")
{
   object->clearFields();
}

DefineConsoleMethod(GuiVariableInspector, setFieldEnabled, void, (const char* name, bool enabled), (true), "loadVars( searchString )")
{
   object->setFieldEnabled(name, enabled);
}

DefineConsoleMethod( GuiVariableInspector, loadVars, void, ( const char * searchString ), , "loadVars( searchString )" )
{
   object->loadVars( searchString );
}
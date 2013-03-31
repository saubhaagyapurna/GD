/** \file
 *  Game Develop
 *  2008-2013 Florian Rival (Florian.Rival@gmail.com)
 */

#if defined(GD_IDE_ONLY)

#include "CppCodeEvent.h"
#include <iostream>
#include <fstream>
#include <wx/filename.h>
#include <wx/dcmemory.h>
#include <wx/log.h>
#include "GDL/RuntimeScene.h"
#include "GDL/OpenSaveGame.h"
#include "GDL/tinyxml/tinyxml.h"
#include "GDL/Events/EventsCodeGenerator.h"
#include "GDL/Events/ExpressionsCodeGeneration.h"
#include "GDL/Events/EventsCodeNameMangler.h"
#include "GDL/Events/EventsCodeGenerationContext.h"
#include "GDCore/IDE/EventsRenderingHelper.h"
#include "GDL/IDE/Dialogs/EditCppCodeEvent.h"
#include "GDCore/IDE/EventsEditorItemsAreas.h"
#include "GDCore/IDE/EventsEditorSelection.h"
#include "GDL/IDE/CodeCompiler.h"
#include "GDL/SourceFile.h"
#include "GDL/XmlMacros.h"

std::string CppCodeEvent::GenerateEventCode(Game & game, Scene & scene, EventsCodeGenerator & codeGenerator, EventsCodeGenerationContext & parentContext)
{
    //Note: The associated source file is compiled separately ( it is recognized as a Source File dependency by
    //DependenciesAnalyzer and compiled by CodeCompilationHelpers);

    //Generate the code to call the associated source file
    std::string functionPrototype = "void "+functionToCall+"("+ (passSceneAsParameter ? "RuntimeScene & scene" :"") + ((passSceneAsParameter && passObjectListAsParameter) ? ", ":"") + (passObjectListAsParameter ? "std::vector<Object*> objectsList" :"") + ");";
    codeGenerator.AddGlobalDeclaration(functionPrototype+"\n");

    std::string outputCode;
    outputCode += "{";

    //Prepare objects list if needed
    if ( passObjectListAsParameter )
    {
        vector< gd::ObjectGroup >::const_iterator globalGroup = find_if(game.GetObjectGroups().begin(), game.GetObjectGroups().end(), bind2nd(gd::GroupHasTheSameName(), objectToPassAsParameter));
        vector< gd::ObjectGroup >::const_iterator sceneGroup = find_if(scene.GetObjectGroups().begin(), scene.GetObjectGroups().end(), bind2nd(gd::GroupHasTheSameName(), objectToPassAsParameter));

        std::vector<std::string> realObjects; //With groups, we may have to generate condition for more than one object list.
        if ( globalGroup != game.GetObjectGroups().end() )
            realObjects = (*globalGroup).GetAllObjectsNames();
        else if ( sceneGroup != scene.GetObjectGroups().end() )
            realObjects = (*sceneGroup).GetAllObjectsNames();
        else
            realObjects.push_back(objectToPassAsParameter);

        if ( realObjects.empty() ) return "";

        outputCode += "std::vector<Object*> functionObjects;";
        for (unsigned int i = 0;i<realObjects.size();++i)
        {
            parentContext.ObjectsListNeeded(realObjects[i]);
            outputCode += "functionObjects.insert("+ string(i == 0 ? "functionObjects.begin()" : "functionObjects.end()") +", "+ManObjListName(realObjects[i])+".begin(), "+ManObjListName(realObjects[i])+".end());";
        }
    }

    std::string functionCall = functionToCall+"("+ (passSceneAsParameter ? "*runtimeContext->scene" :"") +((passSceneAsParameter && passObjectListAsParameter) ? ", ":"")+(passObjectListAsParameter ? "functionObjects" :"") + ");";
    outputCode += ""+functionCall+"\n";

    outputCode += "}";
    return outputCode;
}

void CppCodeEvent::EnsureAssociatedSourceFileIsUpToDate(Game & parentGame) const
{
    std::string outputFile(CodeCompiler::GetInstance()->GetOutputDirectory()+"GD"+ToString(this)+"SourceFile.cpp");

    vector< boost::shared_ptr<GDpriv::SourceFile> >::const_iterator sourceFileIter =
        find_if(parentGame.externalSourceFiles.begin(), parentGame.externalSourceFiles.end(), bind2nd(GDpriv::ExternalSourceFileHasName(), associatedGDManagedSourceFile));

    boost::shared_ptr<GDpriv::SourceFile> sourceFile;
    if ( sourceFileIter != parentGame.externalSourceFiles.end() ) sourceFile = *sourceFileIter;

    //First check if the associated source file exists in the GD project.
    if ( sourceFile == boost::shared_ptr<GDpriv::SourceFile>() )
    {
        //If there is no associated source file existing, then create a new one
        boost::shared_ptr<GDpriv::SourceFile> associatedSourceFile(new GDpriv::SourceFile);
        associatedSourceFile->SetGDManaged(true);

        parentGame.externalSourceFiles.push_back(associatedSourceFile);
        sourceFile = parentGame.externalSourceFiles.back();
    }

    if (sourceFile == boost::shared_ptr<GDpriv::SourceFile>() )
    {
        wxLogError(_("Unable to create a virtual source file for a C++ Code event.\nPlease report this error to Game Develop developer."));
        return;
    }

    //Then check if the associated source file is up to date
    if ( sourceFile->GetFileName() != outputFile )
    {
        sourceFile->SetFileName(outputFile);
        associatedGDManagedSourceFile = outputFile;
    }
    else if ( wxFileExists(outputFile) )
    {
        wxFileName sourceFileInfo(outputFile);
        if ( sourceFileInfo.GetModificationTime().GetTicks() >= lastChangeTimeStamp )
            return;
    }

    //The associated source file is non existing or not up to date: Regenerate it.
    //It will be compiled ( see CodeCompilationHelpers ) as it will be detected ( by DependenciesAnalyzer )
    //as a SourceFile dependency which is not up to date.
    std::ofstream file;
    file.open( outputFile.c_str() );
    file << GenerateAssociatedFileCode();
    file.close();
}

std::string CppCodeEvent::GenerateAssociatedFileCode() const
{
    std::string functionPrototype = "void "+functionToCall+"("+ (passSceneAsParameter ? "RuntimeScene & scene" :"") +((passSceneAsParameter && passObjectListAsParameter) ? ", ":"")+ (passObjectListAsParameter ? "std::vector<Object*> objectsList" :"") + ")";
    std::string output;
    if (passSceneAsParameter ) output += "#include \"GDL/RuntimeScene.h\"\n";
    if (passObjectListAsParameter ) output += "#include \"GDL/Object.h\"\n";
    for (unsigned int i = 0;i<includeFiles.size();++i) output += "#include "+includeFiles[i]+"\n";

    output += functionPrototype+"\n";
    output += "{\n";
    output += inlineCode;
    output += "}\n";

    return output;
}

/**
 * Render the event in the bitmap
 */
void CppCodeEvent::Render(wxDC & dc, int x, int y, unsigned int width, EventsEditorItemsAreas & areas, EventsEditorSelection & selection)
{
    gd::EventsRenderingHelper * renderingHelper = gd::EventsRenderingHelper::GetInstance();
    const int titleTextHeight = 20;

    //Draw header rectangle
    wxRect headerRect(x, y, width, GetRenderedHeight(width));
    renderingHelper->DrawNiceRectangle(dc, headerRect);

    //Header
    dc.SetFont( renderingHelper->GetNiceFont().Bold()  );
    dc.SetTextForeground(wxColour(0,0,0));
    dc.DrawText( (displayedName.empty() ? _("C++ code") : _("C++ code:")) + displayedName, x + 4, y + 3 );

    if ( codeDisplayedInEditor )
    {
        dc.SetFont( renderingHelper->GetFont() );
        dc.SetBrush(renderingHelper->GetActionsRectangleFillBrush());
        dc.SetPen(renderingHelper->GetActionsRectangleOutlinePen());

        dc.DrawRectangle(wxRect(x + 4, y + 3 + titleTextHeight + 2, width-8, GetRenderedHeight(width)-(3 + titleTextHeight + 5)));
        dc.DrawLabel( inlineCode, wxNullBitmap, wxRect(x + 4, y + 3 + titleTextHeight + 4, width-2, GetRenderedHeight(width)));
    }
}

unsigned int CppCodeEvent::GetRenderedHeight(unsigned int width) const
{
    if ( eventHeightNeedUpdate )
    {
        gd::EventsRenderingHelper * renderingHelper = gd::EventsRenderingHelper::GetInstance();
        renderedHeight = 20;

        if ( codeDisplayedInEditor )
        {
            wxMemoryDC fakeDC;
            fakeDC.SetFont(renderingHelper->GetFont());
            renderedHeight += fakeDC.GetMultiLineTextExtent(inlineCode).GetHeight();
            renderedHeight += 15; //Borders
        }
        eventHeightNeedUpdate = false;
    }

    return renderedHeight;
}

gd::BaseEvent::EditEventReturnType CppCodeEvent::EditEvent(wxWindow* parent_, Game & game_, Scene & scene_, gd::MainFrameWrapper & mainFrameWrapper_)
{
    EditCppCodeEvent dialog(parent_, *this, game_, scene_);
    int returned = dialog.ShowModal();

    if ( returned == 0 )
        return Cancelled;
    else
    {
        //Force recreation of the assocaited source file
        wxFileName outputFile(associatedGDManagedSourceFile);
        outputFile.MakeAbsolute(wxFileName::FileName(game_.GetProjectFile()).GetPath());
        if ( wxFileExists(outputFile.GetFullPath()) ) wxRemoveFile(outputFile.GetFullPath());

        EnsureAssociatedSourceFileIsUpToDate(game_);

        if (returned == 2)
            return ChangesMadeButNoNeedForEventsRecompilation;
        else
            return ChangesMade;
    }
}

void CppCodeEvent::SaveToXml(TiXmlElement * elem) const
{
    GD_CURRENT_ELEMENT_SAVE_ATTRIBUTE_STRING("FunctionToCall", functionToCall);
    GD_CURRENT_ELEMENT_SAVE_ATTRIBUTE_BOOL("FunctionNameAutogenerated", functionNameAutogenerated);
    GD_CURRENT_ELEMENT_SAVE_ATTRIBUTE_STRING("InlineCode", inlineCode);
    GD_CURRENT_ELEMENT_SAVE_ATTRIBUTE_STRING("AssociatedGDManagedSourceFile", associatedGDManagedSourceFile);

    GD_CURRENT_ELEMENT_SAVE_ATTRIBUTE_BOOL("PassSceneAsParameter", passSceneAsParameter);
    GD_CURRENT_ELEMENT_SAVE_ATTRIBUTE_BOOL("PassObjectListAsParameter", passObjectListAsParameter);
    GD_CURRENT_ELEMENT_SAVE_ATTRIBUTE_STRING("ObjectToPassAsParameter", objectToPassAsParameter);

    GD_CURRENT_ELEMENT_SAVE_ATTRIBUTE_BOOL("CodeDisplayedInEditor", codeDisplayedInEditor);
    GD_CURRENT_ELEMENT_SAVE_ATTRIBUTE_STRING("DisplayedName", displayedName);

    TiXmlElement * includesElem = new TiXmlElement( "Includes" );
    elem->LinkEndChild( includesElem );
    for ( unsigned int i = 0;i < includeFiles.size();++i)
    {
        TiXmlElement * includeElem = new TiXmlElement( "Include" );
        includesElem->LinkEndChild( includeElem );

        includeElem->SetAttribute("value", includeFiles[i].c_str());
    }

    TiXmlElement * dependenciesElem = new TiXmlElement( "Dependencies" );
    elem->LinkEndChild( dependenciesElem );
    for ( unsigned int i = 0;i < dependencies.size();++i)
    {
        TiXmlElement * dependencyElem = new TiXmlElement( "Dependency" );
        dependenciesElem->LinkEndChild( dependencyElem );

        dependencyElem->SetAttribute("sourceFile", dependencies[i].c_str());
    }
    GD_CURRENT_ELEMENT_SAVE_ATTRIBUTE("lastChangeTimeStamp", lastChangeTimeStamp);
}

void CppCodeEvent::LoadFromXml(const TiXmlElement * elem)
{
    GD_CURRENT_ELEMENT_LOAD_ATTRIBUTE_STRING("FunctionToCall", functionToCall);
    GD_CURRENT_ELEMENT_LOAD_ATTRIBUTE_BOOL("FunctionNameAutogenerated", functionNameAutogenerated);
    GD_CURRENT_ELEMENT_LOAD_ATTRIBUTE_STRING("InlineCode", inlineCode);
    GD_CURRENT_ELEMENT_LOAD_ATTRIBUTE_STRING("AssociatedGDManagedSourceFile", associatedGDManagedSourceFile);

    GD_CURRENT_ELEMENT_LOAD_ATTRIBUTE_BOOL("PassSceneAsParameter", passSceneAsParameter);
    GD_CURRENT_ELEMENT_LOAD_ATTRIBUTE_BOOL("PassObjectListAsParameter", passObjectListAsParameter);
    GD_CURRENT_ELEMENT_LOAD_ATTRIBUTE_STRING("ObjectToPassAsParameter", objectToPassAsParameter);

    GD_CURRENT_ELEMENT_LOAD_ATTRIBUTE_BOOL("CodeDisplayedInEditor", codeDisplayedInEditor);
    GD_CURRENT_ELEMENT_LOAD_ATTRIBUTE_STRING("DisplayedName", displayedName);

    includeFiles.clear();
    const TiXmlElement * includesElem = elem->FirstChildElement( "Includes" );
    if ( includesElem != NULL)
    {
        const TiXmlElement * includeElem = includesElem->FirstChildElement();
        while(includeElem)
        {
            includeFiles.push_back(includeElem->Attribute("value") != NULL ? includeElem->Attribute("value") : "");

            includeElem = includeElem->NextSiblingElement();
        }
    }

    dependencies.clear();
    const TiXmlElement * dependenciesElem = elem->FirstChildElement( "Dependencies" );
    if ( dependenciesElem != NULL)
    {
        const TiXmlElement * dependencyElem = dependenciesElem->FirstChildElement();
        while(dependencyElem)
        {
            dependencies.push_back(dependencyElem->Attribute("sourceFile") != NULL ? dependencyElem->Attribute("sourceFile") : "");

            dependencyElem = dependencyElem->NextSiblingElement();
        }
    }

    int ilastChangeTimeStamp = lastChangeTimeStamp;
    GD_CURRENT_ELEMENT_LOAD_ATTRIBUTE_INT("lastChangeTimeStamp", ilastChangeTimeStamp);
    lastChangeTimeStamp = ilastChangeTimeStamp;
}

void CppCodeEvent::AutogenerateFunctionName()
{
    boost::shared_ptr<gd::BaseEvent> event = originalEvent.lock();

    if ( functionNameAutogenerated && event != boost::shared_ptr<gd::BaseEvent>() )
        functionToCall = "GDCppCode"+ToString(event.get());
}

/**
 * Initialize from another CppCodeEvent.
 * Used by copy ctor and assignement operator
 */
void CppCodeEvent::Init(const CppCodeEvent & event)
{
    includeFiles = event.includeFiles;
    dependencies = event.dependencies;
    functionToCall = event.functionToCall;
    functionNameAutogenerated = event.functionNameAutogenerated;
    inlineCode = event.inlineCode;
    associatedGDManagedSourceFile = event.associatedGDManagedSourceFile;
    lastChangeTimeStamp = 0;

    passSceneAsParameter = event.passSceneAsParameter;
    passObjectListAsParameter = event.passObjectListAsParameter;
    objectToPassAsParameter = event.objectToPassAsParameter;

    codeDisplayedInEditor = event.codeDisplayedInEditor;
    displayedName = event.displayedName;

    if ( functionNameAutogenerated ) AutogenerateFunctionName();
}

/**
 * Custom copy operator
 */
CppCodeEvent::CppCodeEvent(const CppCodeEvent & event) :
BaseEvent(event)
{
    Init(event);
}

/**
 * Custom assignement operator
 */
CppCodeEvent& CppCodeEvent::operator=(const CppCodeEvent & event)
{
    if ( this != &event )
    {
        BaseEvent::operator=(event);
        Init(event);
    }

    return *this;
}

CppCodeEvent::CppCodeEvent() :
BaseEvent(),
functionNameAutogenerated(true),
inlineCode("scene.SetBackgroundColor(100,100,240);\n"),
lastChangeTimeStamp(0),
passSceneAsParameter(true),
passObjectListAsParameter(false),
codeDisplayedInEditor(true)
{
    includeFiles.push_back("<iostream>");
    includeFiles.push_back("\"GDL/CommonTools.h\"");

    if ( functionNameAutogenerated ) AutogenerateFunctionName();
}

#endif

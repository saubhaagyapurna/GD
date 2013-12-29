/** \file
 *  Game Develop
 *  2008-2013 Florian Rival (Florian.Rival@gmail.com)
 */

#include <map>
#include <vector>
#include <string>
#include "GDCore/IDE/Dialogs/ProjectExtensionsDialog.h"
#include "GDCore/IDE/Dialogs/ProjectUpdateDialog.h"
#include "GDCore/IDE/Dialogs/ChooseVariableDialog.h"
#include "GDCore/IDE/ArbitraryResourceWorker.h"
#include "GDCore/PlatformDefinition/Platform.h"
#include "GDCore/PlatformDefinition/PlatformExtension.h"
#include "GDCore/PlatformDefinition/Layout.h"
#include "GDCore/PlatformDefinition/ExternalEvents.h"
#include "GDCore/PlatformDefinition/ExternalLayout.h"
#include "GDCore/PlatformDefinition/SourceFile.h"
#include "GDCore/PlatformDefinition/ImageManager.h"
#include "GDCore/PlatformDefinition/Object.h"
#include "GDCore/PlatformDefinition/ResourcesManager.h"
#include "GDCore/PlatformDefinition/ChangesNotifier.h"
#include "GDCore/Events/ExpressionMetadata.h"
#include "GDCore/IDE/MetadataProvider.h"
#include "GDCore/IDE/PlatformManager.h"
#include "GDCore/CommonTools.h"
#include "GDCore/TinyXml/tinyxml.h"
#include "GDCore/Tools/VersionWrapper.h"
#include "Project.h"
#if defined(GD_IDE_ONLY)
#include <wx/propgrid/propgrid.h>
#include <wx/settings.h>
#include <wx/log.h>
#include <wx/intl.h>
#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/advprops.h>
#include <wx/settings.h>
#include <wx/filename.h>
#endif

using namespace std;

namespace gd
{

Project::Project() :
    useExternalSourceFiles(false),
    #if defined(GD_IDE_ONLY)
    name(_("Project")),
    #endif
    windowWidth(800),
    windowHeight(600),
    maxFPS(60),
    minFPS(10),
    verticalSync(false),
    imageManager(boost::shared_ptr<gd::ImageManager>(new ImageManager))
    #if defined(GD_IDE_ONLY)
    ,currentPlatform(NULL),
    GDMajorVersion(gd::VersionWrapper::Major()),
    GDMinorVersion(gd::VersionWrapper::Minor()),
    dirty(false)
    #endif
{
    imageManager->SetGame(this);
    #if defined(GD_IDE_ONLY)
    //Game use builtin extensions by default
    extensionsUsed.push_back("BuiltinObject");
    extensionsUsed.push_back("BuiltinAudio");
    extensionsUsed.push_back("BuiltinVariables");
    extensionsUsed.push_back("BuiltinTime");
    extensionsUsed.push_back("BuiltinMouse");
    extensionsUsed.push_back("BuiltinKeyboard");
    extensionsUsed.push_back("BuiltinJoystick");
    extensionsUsed.push_back("BuiltinCamera");
    extensionsUsed.push_back("BuiltinWindow");
    extensionsUsed.push_back("BuiltinFile");
    extensionsUsed.push_back("BuiltinNetwork");
    extensionsUsed.push_back("BuiltinScene");
    extensionsUsed.push_back("BuiltinAdvanced");
    extensionsUsed.push_back("Sprite");
    extensionsUsed.push_back("BuiltinCommonInstructions");
    extensionsUsed.push_back("BuiltinCommonConversions");
    extensionsUsed.push_back("BuiltinStringInstructions");
    extensionsUsed.push_back("BuiltinMathematicalTools");
    extensionsUsed.push_back("BuiltinExternalLayouts");
    #endif

    #if !defined(GD_IDE_ONLY)
    platforms.push_back(&CppPlatform::Get());
    #endif
}

Project::~Project()
{
    //dtor
}

boost::shared_ptr<gd::Object> Project::CreateObject(const std::string & type, const std::string & name, const std::string & platformName)
{
    for (unsigned int i = 0;i<platforms.size();++i)
    {
        if ( !platformName.empty() && platforms[i]->GetName() != platformName ) continue;

        boost::shared_ptr<gd::Object> object = platforms[i]->CreateObject(type, name);
        if ( object ) return object;
    }

    return boost::shared_ptr<gd::Object>();
}

gd::Automatism* Project::CreateAutomatism(const std::string & type, const std::string & platformName)
{
    for (unsigned int i = 0;i<platforms.size();++i)
    {
        if ( !platformName.empty() && platforms[i]->GetName() != platformName ) continue;

        gd::Automatism* automatism = platforms[i]->CreateAutomatism(type);
        if ( automatism ) return automatism;
    }

    return NULL;
}

boost::shared_ptr<gd::AutomatismsSharedData> Project::CreateAutomatismSharedDatas(const std::string & type, const std::string & platformName)
{
    for (unsigned int i = 0;i<platforms.size();++i)
    {
        if ( !platformName.empty() && platforms[i]->GetName() != platformName ) continue;

        boost::shared_ptr<gd::AutomatismsSharedData> automatism = platforms[i]->CreateAutomatismSharedDatas(type);
        if ( automatism ) return automatism;
    }

    return boost::shared_ptr<gd::AutomatismsSharedData>();
}

#if defined(GD_IDE_ONLY)
boost::shared_ptr<gd::BaseEvent> Project::CreateEvent(const std::string & type, const std::string & platformName)
{
    for (unsigned int i = 0;i<platforms.size();++i)
    {
        if ( !platformName.empty() && platforms[i]->GetName() != platformName ) continue;

        boost::shared_ptr<gd::BaseEvent> event = platforms[i]->CreateEvent(type);
        if ( event ) return event;
    }

    return boost::shared_ptr<gd::BaseEvent>();
}

Platform & Project::GetCurrentPlatform() const
{
    if ( currentPlatform == NULL )
        std::cout << "FATAL ERROR: Project has no assigned current platform. GD will crash." << std::endl;

    return *currentPlatform;
}

void Project::AddPlatform(Platform* platform)
{
    for (unsigned int i = 0;i<platforms.size();++i)
    {
        if (platforms[i] == platform)
            return;
    }

    //Add the platform and make it the current one if the game has no other platform.
    platforms.push_back(platform);
    if ( currentPlatform == NULL ) currentPlatform = platform;
}

void Project::SetCurrentPlatform(const std::string & platformName)
{
    for (unsigned int i = 0;i<platforms.size();++i)
    {
        if (platforms[i]->GetName() == platformName)
        {
            currentPlatform = platforms[i];
            return;
        }
    }
}

bool Project::RemovePlatform(const std::string & platformName)
{
    if ( platforms.size() <= 1 ) return false;

    for (unsigned int i = 0;i<platforms.size();++i)
    {
        if (platforms[i]->GetName() == platformName)
        {
            //Remove the platform, ensuring that currentPlatform remains correct.
            if ( currentPlatform == platforms[i] ) currentPlatform = platforms.back();
            if ( currentPlatform == platforms[i] ) currentPlatform = platforms[0];
            platforms.erase(platforms.begin()+i);

            return true;
        }
    }

    return false;
}
#endif

bool Project::HasLayoutNamed(const std::string & name) const
{
    return ( find_if(scenes.begin(), scenes.end(), bind2nd(gd::LayoutHasName(), name)) != scenes.end() );
}
gd::Layout & Project::GetLayout(const std::string & name)
{
    return *(*find_if(scenes.begin(), scenes.end(), bind2nd(gd::LayoutHasName(), name)));
}
const gd::Layout & Project::GetLayout(const std::string & name) const
{
    return *(*find_if(scenes.begin(), scenes.end(), bind2nd(gd::LayoutHasName(), name)));
}
gd::Layout & Project::GetLayout(unsigned int index)
{
    return *scenes[index];
}
const gd::Layout & Project::GetLayout (unsigned int index) const
{
    return *scenes[index];
}
unsigned int Project::GetLayoutPosition(const std::string & name) const
{
    for (unsigned int i = 0;i<scenes.size();++i)
    {
        if ( scenes[i]->GetName() == name ) return i;
    }
    return std::string::npos;
}
unsigned int Project::GetLayoutCount() const
{
    return scenes.size();
}

#if defined(GD_IDE_ONLY)
void Project::SwapLayouts(unsigned int first, unsigned int second)
{
    if ( first >= scenes.size() || second >= scenes.size() )
        return;

    boost::shared_ptr<gd::Layout> firstItem = scenes[first];
    boost::shared_ptr<gd::Layout> secondItem = scenes[second];
    scenes[first] = secondItem;
    scenes[second] = firstItem;
}
#endif

gd::Layout & Project::InsertNewLayout(const std::string & name, unsigned int position)
{
    boost::shared_ptr<gd::Layout> newScene = boost::shared_ptr<gd::Layout>(new Layout);
    if (position<scenes.size())
        scenes.insert(scenes.begin()+position, newScene);
    else
        scenes.push_back(newScene);

    newScene->SetName(name);
    #if defined(GD_IDE_ONLY)
    newScene->UpdateAutomatismsSharedData(*this);
    #endif
    return *newScene;
}

void Project::InsertLayout(const gd::Layout & layout, unsigned int position)
{
    boost::shared_ptr<gd::Layout> newScene = boost::shared_ptr<gd::Layout>(new Layout(layout));
    if (position<scenes.size())
        scenes.insert(scenes.begin()+position, newScene);
    else
        scenes.push_back(newScene);

    #if defined(GD_IDE_ONLY)
    newScene->UpdateAutomatismsSharedData(*this);
    #endif
}

void Project::RemoveLayout(const std::string & name)
{
    std::vector< boost::shared_ptr<gd::Layout> >::iterator scene = find_if(scenes.begin(), scenes.end(), bind2nd(gd::LayoutHasName(), name));
    if ( scene == scenes.end() ) return;

    scenes.erase(scene);
}

#if defined(GD_IDE_ONLY)
bool Project::HasExternalEventsNamed(const std::string & name) const
{
    return ( find_if(externalEvents.begin(), externalEvents.end(), bind2nd(gd::ExternalEventsHasName(), name)) != externalEvents.end() );
}
gd::ExternalEvents & Project::GetExternalEvents(const std::string & name)
{
    return *(*find_if(externalEvents.begin(), externalEvents.end(), bind2nd(gd::ExternalEventsHasName(), name)));
}
const gd::ExternalEvents & Project::GetExternalEvents(const std::string & name) const
{
    return *(*find_if(externalEvents.begin(), externalEvents.end(), bind2nd(gd::ExternalEventsHasName(), name)));
}
gd::ExternalEvents & Project::GetExternalEvents(unsigned int index)
{
    return *externalEvents[index];
}
const gd::ExternalEvents & Project::GetExternalEvents (unsigned int index) const
{
    return *externalEvents[index];
}
unsigned int Project::GetExternalEventsPosition(const std::string & name) const
{
    for (unsigned int i = 0;i<externalEvents.size();++i)
    {
        if ( externalEvents[i]->GetName() == name ) return i;
    }
    return std::string::npos;
}
unsigned int Project::GetExternalEventsCount() const
{
    return externalEvents.size();
}

gd::ExternalEvents & Project::InsertNewExternalEvents(const std::string & name, unsigned int position)
{
    boost::shared_ptr<gd::ExternalEvents> newExternalEvents(new gd::ExternalEvents);
    if (position<externalEvents.size())
        externalEvents.insert(externalEvents.begin()+position, newExternalEvents);
    else
        externalEvents.push_back(newExternalEvents);

    newExternalEvents->SetName(name);
    return *newExternalEvents;
}

void Project::InsertExternalEvents(const gd::ExternalEvents & events, unsigned int position)
{
    if (position<externalEvents.size())
        externalEvents.insert(externalEvents.begin()+position, boost::shared_ptr<gd::ExternalEvents>(new gd::ExternalEvents(events)));
    else
        externalEvents.push_back(boost::shared_ptr<gd::ExternalEvents>(new gd::ExternalEvents(events)));
}

void Project::RemoveExternalEvents(const std::string & name)
{
    std::vector< boost::shared_ptr<gd::ExternalEvents> >::iterator events = find_if(externalEvents.begin(), externalEvents.end(), bind2nd(gd::ExternalEventsHasName(), name));
    if ( events == externalEvents.end() ) return;

    externalEvents.erase(events);
}

void Project::SwapExternalEvents(unsigned int first, unsigned int second)
{
    if ( first >= externalEvents.size() || second >= externalEvents.size() )
        return;

    boost::shared_ptr<gd::ExternalEvents> firstItem = externalEvents[first];
    boost::shared_ptr<gd::ExternalEvents> secondItem = externalEvents[second];
    externalEvents[first] = secondItem;
    externalEvents[second] = firstItem;
}

void Project::SwapExternalLayouts(unsigned int first, unsigned int second)
{
    if ( first >= externalLayouts.size() || second >= externalLayouts.size() )
        return;

    boost::shared_ptr<gd::ExternalLayout> firstItem = externalLayouts[first];
    boost::shared_ptr<gd::ExternalLayout> secondItem = externalLayouts[second];
    externalLayouts[first] = secondItem;
    externalLayouts[second] = firstItem;
}
#endif
bool Project::HasExternalLayoutNamed(const std::string & name) const
{
    return ( find_if(externalLayouts.begin(), externalLayouts.end(), bind2nd(gd::ExternalLayoutHasName(), name)) != externalLayouts.end() );
}
gd::ExternalLayout & Project::GetExternalLayout(const std::string & name)
{
    return *(*find_if(externalLayouts.begin(), externalLayouts.end(), bind2nd(gd::ExternalLayoutHasName(), name)));
}
const gd::ExternalLayout & Project::GetExternalLayout(const std::string & name) const
{
    return *(*find_if(externalLayouts.begin(), externalLayouts.end(), bind2nd(gd::ExternalLayoutHasName(), name)));
}
gd::ExternalLayout & Project::GetExternalLayout(unsigned int index)
{
    return *externalLayouts[index];
}
const gd::ExternalLayout & Project::GetExternalLayout (unsigned int index) const
{
    return *externalLayouts[index];
}
unsigned int Project::GetExternalLayoutPosition(const std::string & name) const
{
    for (unsigned int i = 0;i<externalLayouts.size();++i)
    {
        if ( externalLayouts[i]->GetName() == name ) return i;
    }
    return std::string::npos;
}

unsigned int Project::GetExternalLayoutsCount() const
{
    return externalLayouts.size();
}

gd::ExternalLayout & Project::InsertNewExternalLayout(const std::string & name, unsigned int position)
{
    boost::shared_ptr<gd::ExternalLayout> newExternalLayout = boost::shared_ptr<gd::ExternalLayout>(new gd::ExternalLayout);
    if (position<externalLayouts.size())
        externalLayouts.insert(externalLayouts.begin()+position, newExternalLayout);
    else
        externalLayouts.push_back(newExternalLayout);

    newExternalLayout->SetName(name);
    return *newExternalLayout;
}

void Project::InsertExternalLayout(const gd::ExternalLayout & layout, unsigned int position)
{
    boost::shared_ptr<gd::ExternalLayout> newLayout(new gd::ExternalLayout(layout));

    if (position<externalLayouts.size())
        externalLayouts.insert(externalLayouts.begin()+position, newLayout);
    else
        externalLayouts.push_back(newLayout);
}

void Project::RemoveExternalLayout(const std::string & name)
{
    std::vector< boost::shared_ptr<gd::ExternalLayout> >::iterator externalLayout = find_if(externalLayouts.begin(), externalLayouts.end(), bind2nd(gd::ExternalLayoutHasName(), name));
    if ( externalLayout == externalLayouts.end() ) return;

    externalLayouts.erase(externalLayout);
}

#if defined(GD_IDE_ONLY)
//Compatibility with GD2010498
void OpenImagesFromGD2010498(gd::Project & game, const TiXmlElement * imagesElem, const TiXmlElement * dossierElem)
{
    //Images
    while ( imagesElem )
    {
        ImageResource image;

        if ( imagesElem->Attribute( "nom" ) != NULL ) { image.SetName(imagesElem->Attribute( "nom" )); }
        if ( imagesElem->Attribute( "fichier" ) != NULL ) {image.GetFile() = imagesElem->Attribute( "fichier" ); }

        image.smooth = true;
        if ( imagesElem->Attribute( "lissage" ) != NULL && string(imagesElem->Attribute( "lissage" )) == "false")
                image.smooth = false;

        image.alwaysLoaded = false;
        if ( imagesElem->Attribute( "alwaysLoaded" ) != NULL && string(imagesElem->Attribute( "alwaysLoaded" )) == "true")
                image.alwaysLoaded = true;

        game.GetResourcesManager().AddResource(image);
        imagesElem = imagesElem->NextSiblingElement();
    }

    //Dossiers d'images
    while ( dossierElem )
    {
        if ( dossierElem->Attribute( "nom" ) != NULL )
        {
            game.GetResourcesManager().CreateFolder( dossierElem->Attribute( "nom" ) );
            ResourceFolder & folder = game.GetResourcesManager().GetFolder(dossierElem->Attribute( "nom" ));

            const TiXmlElement *elemDossier = dossierElem;
            if ( elemDossier->FirstChildElement( "Contenu" ) != NULL )
            {
                elemDossier = elemDossier->FirstChildElement( "Contenu" )->FirstChildElement();
                while ( elemDossier )
                {
                    if ( elemDossier->Attribute( "nom" ) != NULL ) { folder.AddResource(elemDossier->Attribute( "nom" ), game.GetResourcesManager()); }

                    elemDossier = elemDossier->NextSiblingElement();
                }
            }
        }

        dossierElem = dossierElem->NextSiblingElement();
    }
}
//End of compatibility code


//Compatibility with GD2.x
class SpriteObjectsPositionUpdater : public gd::InitialInstanceFunctor
{
public:
    SpriteObjectsPositionUpdater(gd::Project & project_, gd::Layout & layout_) :
        project(project_),
        layout(layout_)
    {};
    virtual ~SpriteObjectsPositionUpdater() {};

    virtual void operator()(gd::InitialInstance & instance)
    {
        gd::Object * object = NULL;
        if ( layout.HasObjectNamed(instance.GetObjectName()))
            object = &layout.GetObject(instance.GetObjectName());
        else if ( project.HasObjectNamed(instance.GetObjectName()))
            object = &project.GetObject(instance.GetObjectName());
        else return;

        if ( object->GetType() != "Sprite") return;
        if ( !instance.HasCustomSize() ) return;

        wxSetWorkingDirectory(wxFileName::FileName(project.GetProjectFile()).GetPath());
        object->LoadResources(project, layout);

        sf::Vector2f size = object->GetInitialInstanceDefaultSize(instance, project, layout);

        instance.SetX(instance.GetX() + size.x/2 - instance.GetCustomWidth()/2 );
        instance.SetY(instance.GetY() + size.y/2 - instance.GetCustomHeight()/2 );
    }

private:
    gd::Project & project;
    gd::Layout & layout;
};
//End of compatibility code
#endif

void Project::LoadFromXml(const TiXmlElement * rootElement)
{
    if ( rootElement == NULL ) return;

    const TiXmlElement * elem = rootElement->FirstChildElement();

    //Checking version
    #if defined(GD_IDE_ONLY)
    wxString updateText;
    GDMajorVersion = 0;
    GDMinorVersion = 0;
    int build = 0;
    int revision = 0;
    if ( elem != NULL )
    {
        int major = 0;
        int minor = 0;
        elem->QueryIntAttribute( "Major", &major );
        elem->QueryIntAttribute( "Minor", &minor );
        elem->QueryIntAttribute( "Build", &build );
        elem->QueryIntAttribute( "Revision", &revision );
        GDMajorVersion = major;
        GDMinorVersion = minor;
        if ( GDMajorVersion > gd::VersionWrapper::Major() )
        {
            wxLogWarning( _( "The version of Game Develop used to create this game seems to be a new version.\nGame Develop may fail to open the game, or data may be missing.\nYou should check if a new version of Game Develop is available." ) );
        }
        else
        {
            if ( (GDMajorVersion == gd::VersionWrapper::Major() && GDMinorVersion >  gd::VersionWrapper::Minor()) ||
                 (GDMajorVersion == gd::VersionWrapper::Major() && GDMinorVersion == gd::VersionWrapper::Minor() && build >  gd::VersionWrapper::Build()) ||
                 (GDMajorVersion == gd::VersionWrapper::Major() && GDMinorVersion == gd::VersionWrapper::Minor() && build == gd::VersionWrapper::Build() && revision > gd::VersionWrapper::Revision()) )
            {
                wxLogWarning( _( "The version of Game Develop used to create this game seems to be greater.\nGame Develop may fail to open the game, or data may be missing.\nYou should check if a new version of Game Develop is available." ) );
            }
        }

        //Compatibility code
        if ( GDMajorVersion <= 1 )
        {
            wxLogError(_("The game was saved with version of Game Develop which is too old. Please open and save the game with one of the first version of Game Develop 2. You will then be able to open your game with this Game Develop version."));
            return;
        }
        //End of Compatibility code
    }
    #endif

    elem = rootElement->FirstChildElement( "Info" );
    if ( elem ) LoadProjectInformationFromXml(elem);

    //Compatibility code
    #if defined(GD_IDE_ONLY)
    if ( GDMajorVersion < 2 || (GDMajorVersion == 2 && GDMinorVersion == 0 && build <= 10498) )
    {
        OpenImagesFromGD2010498(*this,
                                rootElement->FirstChildElement( "Images" )->FirstChildElement(),
                                rootElement->FirstChildElement( "DossierImages" )->FirstChildElement());
    }
    #endif
    //End of Compatibility code

    //Compatibility code
    #if defined(GD_IDE_ONLY)
    if ( GDMajorVersion < 3 )
    {
        updateText += _("Sprite scaling has changed since GD 2: The resizing is made so that the origin point of the object won't move whatever the scale of the object.\n");
        updateText += _("You may have to slightly change the position of some objects if you have changed their size.\n\n");
        updateText += _("Thank you for your understanding.\n");

    }
    #endif
    //End of Compatibility code

    //Compatibility code
    #if defined(GD_IDE_ONLY)
    if ( GDMajorVersion < 2 || (GDMajorVersion == 2 && GDMinorVersion <= 1 && build <= 10822) )
    {
        if ( std::find(GetUsedExtensions().begin(), GetUsedExtensions().end(), "BuiltinExternalLayouts") == GetUsedExtensions().end() )
            GetUsedExtensions().push_back("BuiltinExternalLayouts");
    }
    #endif

    resourcesManager.LoadFromXml(rootElement->FirstChildElement( "Resources" ));

    //Global objects
    elem = rootElement->FirstChildElement( "Objects" );
    if ( elem )
        LoadObjectsFromXml(*this, elem);

    #if defined(GD_IDE_ONLY)
    //Global object groups
    elem = rootElement->FirstChildElement( "ObjectGroups" );
    if ( elem )
        gd::ObjectGroup::LoadFromXml(GetObjectGroups(), elem);
    #endif

    //Global variables
    elem = rootElement->FirstChildElement( "Variables" );
    if ( elem ) GetVariables().LoadFromXml(elem);

    //Scenes
    if ( rootElement->FirstChildElement( "Scenes" ) ) {
        firstLayout = rootElement->FirstChildElement( "Scenes" )->Attribute("firstScene") ?
            rootElement->FirstChildElement( "Scenes" )->Attribute("firstScene") : "";

        elem = rootElement->FirstChildElement( "Scenes" ) ? rootElement->FirstChildElement( "Scenes" )->FirstChildElement() : NULL;
        while ( elem )
        {
            std::string layoutName = elem->Attribute( "nom" ) != NULL ? elem->Attribute( "nom" ) : "";

            //Add a new layout
            boost::shared_ptr<gd::Layout> layout = boost::shared_ptr<gd::Layout>(new gd::Layout);
            if ( layout )
            {
                scenes.push_back(layout);
                scenes.back()->SetName(layoutName);
                scenes.back()->LoadFromXml(*this, elem);

                //Compatibility code with GD 2.x
                #if defined(GD_IDE_ONLY)
                if ( GDMajorVersion <= 2 )
                {
                    SpriteObjectsPositionUpdater updater(*this, *scenes.back());
                    gd::InitialInstancesContainer & instances = scenes.back()->GetInitialInstances();
                    instances.IterateOverInstances(updater);

                }
                #endif
                //End of compatibility code
            }
            else
                std::cout << "ERROR : Unable to create a layout when loading a project!" << std::endl;

            elem = elem->NextSiblingElement();
        }
    }

    #if defined(GD_IDE_ONLY)
    //External events
    elem = rootElement->FirstChildElement( "ExternalEvents" );
    if ( elem )
    {
        const TiXmlElement * externalEventsElem = elem->FirstChildElement();

        while ( externalEventsElem )
        {
            gd::ExternalEvents & externalEvents = InsertNewExternalEvents(externalEventsElem->Attribute("Name") ? externalEventsElem->Attribute("Name") : "", GetExternalEventsCount() );
            externalEvents.LoadFromXml(*this, externalEventsElem);

            externalEventsElem = externalEventsElem->NextSiblingElement();
        }
    }

    elem = rootElement->FirstChildElement( "ExternalSourceFiles" );
    if ( elem )
    {
        const TiXmlElement * sourceFileElem = elem->FirstChildElement( "SourceFile" );
        while (sourceFileElem)
        {
            boost::shared_ptr<gd::SourceFile> newSourceFile(new gd::SourceFile);
            newSourceFile->LoadFromXml(sourceFileElem);
            externalSourceFiles.push_back(newSourceFile);

            sourceFileElem = sourceFileElem->NextSiblingElement();
        }
    }
    #endif

    elem = rootElement->FirstChildElement( "ExternalLayouts" );
    if ( elem )
    {
        const TiXmlElement * externalLayoutElem = elem->FirstChildElement( "ExternalLayout" );
        while (externalLayoutElem)
        {
            boost::shared_ptr<gd::ExternalLayout> newExternalLayout(new gd::ExternalLayout);
            newExternalLayout->LoadFromXml(externalLayoutElem);
            externalLayouts.push_back(newExternalLayout);

            externalLayoutElem = externalLayoutElem->NextSiblingElement();
        }
    }

    #if defined(GD_IDE_ONLY) 
    if (!updateText.empty()) //TODO
    {
        ProjectUpdateDialog updateDialog(NULL, updateText);
        updateDialog.ShowModal();
    }

    dirty = false;
    #endif
}

void Project::LoadProjectInformationFromXml(const TiXmlElement * elem)
{
    if ( elem->FirstChildElement( "Nom" ) != NULL ) { SetName( elem->FirstChildElement( "Nom" )->Attribute( "value" ) ); }
    if ( elem->FirstChildElement( "WindowW" ) != NULL ) { SetDefaultWidth(ToInt(elem->FirstChildElement( "WindowW" )->Attribute( "value"))); }
    if ( elem->FirstChildElement( "WindowH" ) != NULL ) { SetDefaultHeight(ToInt(elem->FirstChildElement( "WindowH" )->Attribute( "value"))); }
    if ( elem->FirstChildElement( "FPSmax" ) != NULL ) { SetMaximumFPS(ToInt(elem->FirstChildElement( "FPSmax" )->Attribute( "value" ))); }
    if ( elem->FirstChildElement( "FPSmin" ) != NULL ) { SetMinimumFPS(ToInt(elem->FirstChildElement( "FPSmin" )->Attribute( "value" ))); }

    SetVerticalSyncActivatedByDefault( false );
    if ( elem->FirstChildElement( "verticalSync" ) != NULL )
    {
        string result = elem->FirstChildElement( "verticalSync" )->Attribute("value");
        if ( result == "true")
            SetVerticalSyncActivatedByDefault(true);
    }

    if ( elem->FirstChildElement( "Extensions" ) != NULL )
    {
        const TiXmlElement * extensionsElem = elem->FirstChildElement( "Extensions" )->FirstChildElement();
        while (extensionsElem)
        {
            if ( extensionsElem->Attribute("name") )
            {
                std::string extensionName = extensionsElem->Attribute("name");
                if ( find(GetUsedExtensions().begin(), GetUsedExtensions().end(), extensionName ) == GetUsedExtensions().end() )
                    GetUsedExtensions().push_back(extensionName);
            }

            extensionsElem = extensionsElem->NextSiblingElement();
        }
    }

    #if defined(GD_IDE_ONLY)
    if ( elem->FirstChildElement( "Auteur" ) != NULL ) { SetAuthor( elem->FirstChildElement( "Auteur" )->Attribute( "value" ) ); }
    if ( elem->FirstChildElement( "LatestCompilationDirectory" ) != NULL && elem->FirstChildElement( "LatestCompilationDirectory" )->Attribute( "value" ) != NULL )
        SetLastCompilationDirectory( elem->FirstChildElement( "LatestCompilationDirectory" )->Attribute( "value" ) );

    if ( elem->FirstChildElement( "Platforms" ) )
    {
        std::string current = elem->FirstChildElement( "Platforms" )->Attribute("current") ? elem->FirstChildElement( "Platforms" )->Attribute("current") : "";

        for (const TiXmlElement * platformElem = elem->FirstChildElement( "Platforms" )->FirstChildElement( "Platform" );
             platformElem;
             platformElem = platformElem->NextSiblingElement())
        {
            std::string name = platformElem->Attribute("name") ? platformElem->Attribute("name") : "";
            gd::Platform * platform = gd::PlatformManager::GetInstance()->GetPlatform(name);

            if ( platform ) {
                AddPlatform(platform);
                if ( platform->GetName() == current ) currentPlatform = platform;
            }
            else {
                std::cout << "Platform \"" << name << "\" is unknown." << std::endl;
            }
        }
    }
    else
    {
        //Compatibility with GD2.x
        platforms.push_back(gd::PlatformManager::GetInstance()->GetPlatform("Game Develop C++ platform"));
        currentPlatform = platforms.back();
    }

    if ( elem->Attribute("winExecutableFilename") ) winExecutableFilename = elem->Attribute("winExecutableFilename");
    if ( elem->Attribute("winExecutableIconFile") ) winExecutableIconFile = elem->Attribute("winExecutableIconFile");
    if ( elem->Attribute("linuxExecutableFilename") ) linuxExecutableFilename = elem->Attribute("linuxExecutableFilename");
    if ( elem->Attribute("macExecutableFilename") ) macExecutableFilename = elem->Attribute("macExecutableFilename");
    if ( elem->Attribute( "useExternalSourceFiles" )  != NULL )
        useExternalSourceFiles = ToString(elem->Attribute( "useExternalSourceFiles" )) == "true";
    #endif

    return;
}

#if defined(GD_IDE_ONLY)
void Project::SaveToXml(TiXmlElement * root) const
{
    TiXmlElement * version = new TiXmlElement( "GDVersion" );
    root->LinkEndChild( version );
    version->SetAttribute( "Major", ToString( gd::VersionWrapper::Major() ).c_str() );
    version->SetAttribute( "Minor", ToString( gd::VersionWrapper::Minor() ).c_str() );
    version->SetAttribute( "Build", ToString( gd::VersionWrapper::Build() ).c_str() );
    version->SetAttribute( "Revision", ToString( gd::VersionWrapper::Revision() ).c_str() );
    GDMajorVersion = gd::VersionWrapper::Major();
    GDMinorVersion = gd::VersionWrapper::Minor();

    TiXmlElement * infos = new TiXmlElement( "Info" );
    root->LinkEndChild( infos );

    //Info du jeu
    TiXmlElement * info;
    {
        info = new TiXmlElement( "Nom" );
        infos->LinkEndChild( info );
        info->SetAttribute( "value", GetName().c_str() );
        info = new TiXmlElement( "Auteur" );
        infos->LinkEndChild( info );
        info->SetAttribute( "value", GetAuthor().c_str() );

        TiXmlElement * extensions = new TiXmlElement( "Extensions" );
        infos->LinkEndChild( extensions );
        for (unsigned int i =0;i<GetUsedExtensions().size();++i)
        {
            TiXmlElement * extension = new TiXmlElement( "Extension" );
            extensions->LinkEndChild( extension );
            extension->SetAttribute("name", GetUsedExtensions().at(i).c_str());
        }

        TiXmlElement * platformsElem = new TiXmlElement( "Platforms" );
        if ( currentPlatform ) platformsElem->SetAttribute("current", currentPlatform->GetName().c_str());
        infos->LinkEndChild( platformsElem );
        for (unsigned int i =0;i<platforms.size();++i)
        {
            TiXmlElement * platform = new TiXmlElement( "Platform" );
            platformsElem->LinkEndChild( platform );
            platform->SetAttribute("name", platforms[i]->GetName().c_str() );
        }

        info = new TiXmlElement( "WindowW" );
        infos->LinkEndChild( info );
        info->SetAttribute( "value", GetMainWindowDefaultWidth() );
        info = new TiXmlElement( "WindowH" );
        infos->LinkEndChild( info );
        info->SetAttribute( "value", GetMainWindowDefaultHeight() );
        info = new TiXmlElement( "Portable" );
        infos->LinkEndChild( info );
        info = new TiXmlElement( "LatestCompilationDirectory" );
        infos->LinkEndChild( info );
        info->SetAttribute( "value", GetLastCompilationDirectory().c_str() );
    }
    {
        TiXmlElement * elem = infos;
        elem->SetAttribute("winExecutableFilename", winExecutableFilename.c_str());
        elem->SetAttribute("winExecutableIconFile", winExecutableIconFile.c_str());
        elem->SetAttribute("linuxExecutableFilename", linuxExecutableFilename.c_str());
        elem->SetAttribute("macExecutableFilename", macExecutableFilename.c_str());
        elem->SetAttribute("useExternalSourceFiles", useExternalSourceFiles ? "true" : "false");
    }


    info = new TiXmlElement( "FPSmax" );
    infos->LinkEndChild( info );
    info->SetAttribute( "value", GetMaximumFPS() );
    info = new TiXmlElement( "FPSmin" );
    infos->LinkEndChild( info );
    info->SetAttribute( "value", GetMinimumFPS() );

    info = new TiXmlElement( "verticalSync" );
    infos->LinkEndChild( info );
    info->SetAttribute( "value", IsVerticalSynchronizationEnabledByDefault() ? "true" : "false" );

    //Ressources
    TiXmlElement * resources = new TiXmlElement( "Resources" );
    root->LinkEndChild( resources );
    resourcesManager.SaveToXml(resources);

    //Global objects
    TiXmlElement * objects = new TiXmlElement( "Objects" );
    root->LinkEndChild( objects );
    SaveObjectsToXml(objects);

    //Global object groups
    TiXmlElement * globalObjectGroups = new TiXmlElement( "ObjectGroups" );
    root->LinkEndChild( globalObjectGroups );
    gd::ObjectGroup::SaveToXml(GetObjectGroups(), globalObjectGroups);

    //Global variables
    TiXmlElement * variables = new TiXmlElement( "Variables" );
    root->LinkEndChild( variables );
    GetVariables().SaveToXml(variables);

    //Scenes
    TiXmlElement * scenes = new TiXmlElement( "Scenes" );
    root->LinkEndChild( scenes );
    scenes->SetAttribute("firstScene", firstLayout.c_str());
    for ( unsigned int i = 0;i < GetLayoutCount();i++ )
    {
        TiXmlElement * scene = new TiXmlElement( "Scene" );
        scenes->LinkEndChild( scene );
        GetLayout(i).SaveToXml(scene);
    }

    //External events
    TiXmlElement * externalEvents = new TiXmlElement( "ExternalEvents" );
    root->LinkEndChild( externalEvents );
    for ( unsigned int j = 0;j < GetExternalEventsCount();j++ )
    {
        TiXmlElement * externalEventsElem = new TiXmlElement( "ExternalEvents" );
        externalEvents->LinkEndChild( externalEventsElem );
        GetExternalEvents(j).SaveToXml(externalEventsElem);
    }

    //External layouts
    TiXmlElement * externalLayoutsElem = new TiXmlElement( "ExternalLayouts" );
    root->LinkEndChild( externalLayoutsElem );
    for (unsigned int i = 0;i<externalLayouts.size();++i)
    {
        TiXmlElement * externalLayout = new TiXmlElement( "ExternalLayout" );
        externalLayoutsElem->LinkEndChild( externalLayout );
        externalLayouts[i]->SaveToXml(externalLayout);
    }

    //External source files
    TiXmlElement * externalSourceFilesElem = new TiXmlElement( "ExternalSourceFiles" );
    root->LinkEndChild( externalSourceFilesElem );
    for (unsigned int i = 0;i<externalSourceFiles.size();++i)
    {
        TiXmlElement * sourceFile = new TiXmlElement( "SourceFile" );
        externalSourceFilesElem->LinkEndChild( sourceFile );
        externalSourceFiles[i]->SaveToXml(sourceFile);
    }

    #if defined(GD_IDE_ONLY)
    dirty = false;
    #endif
}

void Project::ExposeResources(gd::ArbitraryResourceWorker & worker)
{
    //Add project resources
    std::vector<std::string> resources = GetResourcesManager().GetAllResourcesList();
    for ( unsigned int i = 0;i < resources.size() ;i++ )
    {
        if ( GetResourcesManager().GetResource(resources[i]).UseFile() )
            worker.ExposeResource(GetResourcesManager().GetResource(resources[i]).GetFile());
    }
    wxSafeYield();

    //Add layouts resources
    for ( unsigned int s = 0;s < GetLayoutCount();s++ )
    {
        for (unsigned int j = 0;j<GetLayout(s).GetObjectsCount();++j) //Add objects resources
        	GetLayout(s).GetObject(j).ExposeResources(worker);

        LaunchResourceWorkerOnEvents(*this, GetLayout(s).GetEvents(), worker);
    }
    //Add external events resources
    for ( unsigned int s = 0;s < GetExternalEventsCount();s++ )
    {
        LaunchResourceWorkerOnEvents(*this, GetExternalEvents(s).GetEvents(), worker);
    }
    wxSafeYield();

    //Add global objects resources
    for (unsigned int j = 0;j<GetObjectsCount();++j)
        GetObject(j).ExposeResources(worker);
    wxSafeYield();
}

void Project::PopulatePropertyGrid(wxPropertyGrid * grid)
{
    grid->Append( new wxPropertyCategory(_("Properties")) );
    grid->Append( new wxStringProperty(_("Name of the project"), wxPG_LABEL, GetName()) );
    grid->Append( new wxStringProperty(_("Author"), wxPG_LABEL, GetAuthor()) );
    grid->Append( new wxStringProperty(_("Globals variables"), wxPG_LABEL, _("Click to edit...")) );
    grid->Append( new wxStringProperty(_("Extensions"), wxPG_LABEL, _("Click to edit...")) );
    grid->Append( new wxPropertyCategory(_("Window")) );
    grid->Append( new wxUIntProperty(_("Width"), wxPG_LABEL, GetMainWindowDefaultWidth()) );
    grid->Append( new wxUIntProperty(_("Height"), wxPG_LABEL, GetMainWindowDefaultHeight()) );
    grid->Append( new wxBoolProperty(_("Vertical Synchronization"), wxPG_LABEL, IsVerticalSynchronizationEnabledByDefault()) );
    grid->Append( new wxBoolProperty(_("Limit the framerate"), wxPG_LABEL, GetMaximumFPS() != -1) );
    grid->Append( new wxIntProperty(_("Maximum FPS"), wxPG_LABEL, GetMaximumFPS()) );
    grid->Append( new wxUIntProperty(_("Minimum FPS"), wxPG_LABEL, GetMinimumFPS()) );

    grid->SetPropertyCell(_("Globals variables"), 1, _("Click to edit..."), wxNullBitmap, wxSystemSettings::GetColour(wxSYS_COLOUR_HOTLIGHT ));
    grid->SetPropertyReadOnly(_("Globals variables"));
    grid->SetPropertyCell(_("Extensions"), 1, _("Click to edit..."), wxNullBitmap, wxSystemSettings::GetColour(wxSYS_COLOUR_HOTLIGHT ));
    grid->SetPropertyReadOnly(_("Extensions"));

    if ( GetMaximumFPS() == -1 )
    {
        grid->GetProperty(_("Maximum FPS"))->Enable(false);
        grid->GetProperty(_("Maximum FPS"))->SetValue("");
    }
    else
        grid->GetProperty(_("Maximum FPS"))->Enable(true);

    grid->Append( new wxPropertyCategory(_("Generation")) );
    grid->Append( new wxStringProperty(_("Windows executable name"), wxPG_LABEL, winExecutableFilename) );
    grid->Append( new wxImageFileProperty(_("Windows executable icon"), wxPG_LABEL, winExecutableIconFile) );
    grid->Append( new wxStringProperty(_("Linux executable name"), wxPG_LABEL, linuxExecutableFilename) );
    grid->Append( new wxStringProperty(_("Mac OS executable name"), wxPG_LABEL, macExecutableFilename) );

    grid->Append( new wxPropertyCategory(_("C++ features")) );
    grid->Append( new wxBoolProperty(_("Activate the use of C++ source files"), wxPG_LABEL, useExternalSourceFiles) );
}

void Project::UpdateFromPropertyGrid(wxPropertyGrid * grid)
{
    if ( grid->GetProperty(_("Name of the project")) != NULL)
        SetName(gd::ToString(grid->GetProperty(_("Name of the project"))->GetValueAsString()));
    if ( grid->GetProperty(_("Author")) != NULL)
        SetAuthor(gd::ToString(grid->GetProperty(_("Author"))->GetValueAsString()));
    if ( grid->GetProperty(_("Width")) != NULL)
        SetDefaultWidth(grid->GetProperty(_("Width"))->GetValue().GetInteger());
    if ( grid->GetProperty(_("Height")) != NULL)
        SetDefaultHeight(grid->GetProperty(_("Height"))->GetValue().GetInteger());
    if ( grid->GetProperty(_("Vertical Synchronization")) != NULL)
        SetVerticalSyncActivatedByDefault(grid->GetProperty(_("Vertical Synchronization"))->GetValue().GetBool());
    if ( grid->GetProperty(_("Limit the framerate")) != NULL && !grid->GetProperty(_("Limit the framerate"))->GetValue().GetBool())
        SetMaximumFPS(-1);
    else if ( grid->GetProperty(_("Maximum FPS")) != NULL)
        SetMaximumFPS(grid->GetProperty(_("Maximum FPS"))->GetValue().GetInteger());
    if ( grid->GetProperty(_("Minimum FPS")) != NULL)
        SetMinimumFPS(grid->GetProperty(_("Minimum FPS"))->GetValue().GetInteger());

    if ( grid->GetProperty(_("Windows executable name")) != NULL)
        winExecutableFilename = gd::ToString(grid->GetProperty(_("Windows executable name"))->GetValueAsString());
    if ( grid->GetProperty(_("Windows executable icon")) != NULL)
        winExecutableIconFile = gd::ToString(grid->GetProperty(_("Windows executable icon"))->GetValueAsString());
    if ( grid->GetProperty(_("Linux executable name")) != NULL)
        linuxExecutableFilename = gd::ToString(grid->GetProperty(_("Linux executable name"))->GetValueAsString());
    if ( grid->GetProperty(_("Mac OS executable name")) != NULL)
        macExecutableFilename = gd::ToString(grid->GetProperty(_("Mac OS executable name"))->GetValueAsString());
    if ( grid->GetProperty(_("Activate the use of C++ source files")) != NULL)
        useExternalSourceFiles =grid->GetProperty(_("Activate the use of C++ source files"))->GetValue().GetBool();
}

void Project::OnChangeInPropertyGrid(wxPropertyGrid * grid, wxPropertyGridEvent & event)
{
    if (event.GetPropertyName() == _("Limit the framerate") )
        grid->EnableProperty(_("Maximum FPS"), grid->GetProperty(_("Limit the framerate"))->GetValue().GetBool());

    UpdateFromPropertyGrid(grid);
}

void Project::OnSelectionInPropertyGrid(wxPropertyGrid * grid, wxPropertyGridEvent & event)
{
    if ( event.GetColumn() == 1) //Manage button-like properties
    {
        if ( event.GetPropertyName() == _("Extensions") )
        {
            gd::ProjectExtensionsDialog dialog(NULL, *this);
            dialog.ShowModal();
        }
        else if ( event.GetPropertyName() == _("Globals variables") )
        {
            gd::ChooseVariableDialog dialog(NULL, GetVariables(), /*editingOnly=*/true);
            dialog.SetAssociatedProject(this);
            dialog.ShowModal();
        }
    }
}


bool Project::SaveToFile(const std::string & filename)
{
    //Create document structure
    TiXmlDocument doc;
    TiXmlDeclaration* decl = new TiXmlDeclaration( "1.0", "ISO-8859-1", "" );
    doc.LinkEndChild( decl );

    TiXmlElement * root = new TiXmlElement( "Project" );
    doc.LinkEndChild( root );

    SaveToXml(root);

    //Wrie XML to file
    if ( !doc.SaveFile( filename.c_str() ) )
    {
        wxLogError( _( "Unable to save file ")+filename+_("!\nCheck that the drive has enough free space, is not write-protected and that you have read/write permissions." ) );
        return false;
    }

    return true;
}

bool Project::LoadFromFile(const std::string & filename)
{
    //Load document structure
    TiXmlDocument doc;
    if ( !doc.LoadFile(filename.c_str()) )
    {
        wxString errorTinyXmlDesc = doc.ErrorDesc();
        wxString error = _( "Error while loading :" ) + "\n" + errorTinyXmlDesc + "\n\n" +_("Make sure the file exists and that you have the right to open the file.");

        wxLogError( error );
        return false;
    }

    SetProjectFile(filename);

    TiXmlHandle hdl( &doc );
    LoadFromXml(hdl.FirstChildElement().Element());

    #if defined(GD_IDE_ONLY)
    dirty = false;
    #endif
    return true;
}

bool Project::ValidateObjectName(const std::string & name)
{
    std::string allowedCharacter = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
    return !(name.find_first_not_of(allowedCharacter) != std::string::npos);
}

std::string Project::GetBadObjectNameWarning()
{
    return gd::ToString(_("Please use only letters, digits\nand underscores ( _ )."));
}
#endif

Project::Project(const Project & other)
{
    Init(other);
}

Project& Project::operator=(const Project & other)
{
    if ( this != &other )
        Init(other);

    return *this;
}

void Project::Init(const gd::Project & game)
{
    //Some properties
    name = game.name;
    windowWidth = game.windowWidth;
    windowHeight = game.windowHeight;
    maxFPS = game.maxFPS;
    minFPS = game.minFPS;
    verticalSync = game.verticalSync;

    #if defined(GD_IDE_ONLY)
    author = game.author;
    latestCompilationDirectory = game.latestCompilationDirectory;
    extensionsUsed = game.GetUsedExtensions();
    objectGroups = game.objectGroups;

    GDMajorVersion = game.GDMajorVersion;
    GDMinorVersion = game.GDMinorVersion;

    currentPlatform = game.currentPlatform;
    #endif
    platforms = game.platforms;

    //Resources
    resourcesManager = game.resourcesManager;
    imageManager = boost::shared_ptr<ImageManager>(new ImageManager(*game.imageManager));
    imageManager->SetGame(this);

    GetObjects().clear();
    for (unsigned int i =0;i<game.GetObjects().size();++i)
    	GetObjects().push_back( boost::shared_ptr<gd::Object>(game.GetObjects()[i]->Clone()) );

    scenes.clear();
    for (unsigned int i =0;i<game.scenes.size();++i)
    	scenes.push_back( boost::shared_ptr<gd::Layout>(new gd::Layout(*game.scenes[i])) );

    #if defined(GD_IDE_ONLY)
    externalEvents.clear();
    for (unsigned int i =0;i<game.externalEvents.size();++i)
    	externalEvents.push_back( boost::shared_ptr<gd::ExternalEvents>(new gd::ExternalEvents(*game.externalEvents[i])) );
    #endif

    externalLayouts.clear();
    for (unsigned int i =0;i<game.externalLayouts.size();++i)
    	externalLayouts.push_back( boost::shared_ptr<gd::ExternalLayout>(new gd::ExternalLayout(*game.externalLayouts[i])) );

    useExternalSourceFiles = game.useExternalSourceFiles;

    #if defined(GD_IDE_ONLY)
    externalSourceFiles.clear();
    for (unsigned int i =0;i<game.externalSourceFiles.size();++i)
    	externalSourceFiles.push_back( boost::shared_ptr<gd::SourceFile>(new gd::SourceFile(*game.externalSourceFiles[i])) );
    #endif

    variables = game.GetVariables();

    #if defined(GD_IDE_ONLY)
    gameFile = game.GetProjectFile();
    imagesChanged = game.imagesChanged;

    winExecutableFilename = game.winExecutableFilename;
    winExecutableIconFile = game.winExecutableIconFile;
    linuxExecutableFilename = game.linuxExecutableFilename;
    macExecutableFilename = game.macExecutableFilename;
    #endif
}

}
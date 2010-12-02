/*
 *  Gazebo - Outdoor Multi-Robot Simulator
 *  Copyright (C) 2003
 *     Nate Koenig & Andrew Howard
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
/* Desc: Middleman between OGRE and Gazebo
 * Author: Nate Koenig
 * Date: 13 Feb 2006
 * CVS: $Id$
 */
#include <stdint.h>

#include <Ogre.h>
#include <OgreDataStream.h>
#include <OgreLogManager.h>
#include <OgreWindowEventUtilities.h>

#include <sys/types.h>
#include <dirent.h>
#include <iostream>
#include <string.h>

#include "gazebo_config.h"

#include "Events.hh"
#include "Scene.hh"
#include "Grid.hh"
#include "OgreVisual.hh"
#include "UserCamera.hh"
#include "OgreMovableText.hh"
#include "OgreHUD.hh"
#include "Entity.hh"
#include "GazeboError.hh"
#include "GazeboMessage.hh"
#include "GazeboConfig.hh"
#include "Global.hh"
#include "XMLConfig.hh"
#include "Simulator.hh"
#include "OgreFrameListener.hh"
#include "OgreCreator.hh"
#include "RTShaderSystem.hh"
#include "OgreAdaptor.hh"

#include "Timer.hh"

using namespace gazebo;

////////////////////////////////////////////////////////////////////////////////
/// Constructor
OgreAdaptor::OgreAdaptor()
{
  // Create a new log manager and prevent output from going to stdout
  this->logManager = new Ogre::LogManager();
  this->logManager->createLog("Ogre.log", true, false, false);

  this->logManager=NULL;
  this->root=NULL;
  //this->frameListener = NULL;

  this->dummyDisplay = false;
}

////////////////////////////////////////////////////////////////////////////////
/// Destructor
OgreAdaptor::~OgreAdaptor()
{
  if (this->dummyDisplay)
  {
    glXDestroyContext(this->dummyDisplay, this->dummyContext);
    XDestroyWindow(this->dummyDisplay, this->dummyWindowId);
    XCloseDisplay(this->dummyDisplay);
  }
}

////////////////////////////////////////////////////////////////////////////////
// Closes and free
void OgreAdaptor::Close()
{
  this->Fini();
}

////////////////////////////////////////////////////////////////////////////////
/// Load the parameters for Ogre
void OgreAdaptor::Load(XMLConfigNode *rootNode)
{
  if (this->root)
    return;

  // Make the root
  try
  {
    this->root = new Ogre::Root();
  }
  catch (Ogre::Exception e)
  {
    gzthrow("Unable to create an Ogre rendering environment, no Root ");
  }

  // Load all the plugins
  this->LoadPlugins();

  // Setup the rendering system, and create the context
  this->SetupRenderSystem();

  // Initialize the root node, and don't create a window
  this->root->initialise(false);

  // Setup the available resources
  this->SetupResources();

  Scene *scene = new Scene("primary_scene");
  scene->Load(rootNode->GetChild("ogre", "rendering"));
  scene->CreateGrid( 10, 1, 0.03, Color(1,1,1,1));
  this->scenes.push_back( scene );  

  scene = new Scene("viewer_scene");
  scene->SetType(Scene::GENERIC);
  scene->SetAmbientColor(Color(0.5, 0.5, 0.5));
  scene->SetBackgroundColor(Color(0.5, 0.5, 0.5, 1.0));
  scene->CreateGrid( 10, 1, 0.03, Color(1,1,1,1));

  this->scenes.push_back( scene );  
}

////////////////////////////////////////////////////////////////////////////////
// Initialize ogre
void OgreAdaptor::Init(XMLConfigNode *rootNode)
{
  XMLConfigNode *node = NULL;
  Ogre::ColourValue ambient;

  if (rootNode)
    node = rootNode->GetChild("ogre", "rendering");

  /// Create a dummy rendering context.
  /// This will allow gazebo to run headless. And it also allows OGRE to 
  /// initialize properly
  if (!Simulator::Instance()->GetGuiEnabled())
  {
    this->dummyDisplay = XOpenDisplay(0);
    if (!this->dummyDisplay) 
      gzthrow(std::string("Can't open display: ") + XDisplayName(0) + "\n");

    int screen = DefaultScreen(this->dummyDisplay);

    int attribList[] = {GLX_RGBA, GLX_DOUBLEBUFFER, GLX_DEPTH_SIZE, 16, 
                        GLX_STENCIL_SIZE, 8, None };

    this->dummyVisual = glXChooseVisual(this->dummyDisplay, screen, 
                                        (int *)attribList);

    this->dummyWindowId = XCreateSimpleWindow(this->dummyDisplay, 
        RootWindow(this->dummyDisplay, screen), 0, 0, 1, 1, 0, 0, 0);

    this->dummyContext = glXCreateContext(this->dummyDisplay, 
                                          this->dummyVisual, NULL, 1);

    glXMakeCurrent(this->dummyDisplay, this->dummyWindowId, this->dummyContext);

    std::stringstream stream;
    stream << (int32_t)this->dummyWindowId;
    OgreCreator::Instance()->CreateWindow( stream.str(), 1,1);
  }

  // Set default mipmap level (NB some APIs ignore this)
  Ogre::TextureManager::getSingleton().setDefaultNumMipmaps( 5 );
  
  // init the resources
  Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();

  Ogre::MaterialManager::getSingleton().setDefaultTextureFiltering(Ogre::TFO_ANISOTROPIC);

  if (this->HasGLSL())
    RTShaderSystem::Instance()->Init();

  for (unsigned int i=0; i < 1; i++)
    this->scenes[i]->Init(this->root);

/*
  // Ambient lighting
  this->sceneMgr->setAmbientLight(ambient);

  // Not sure if this does something useful.
  if (**(this->shadowsP))
  {
    this->sceneMgr->setShadowTechnique( Ogre::SHADOWTYPE_STENCIL_MODULATIVE );
    this->sceneMgr->setShadowTextureSettings(512,2);
    this->sceneMgr->setShadowColour(Ogre::ColourValue(0.5,0.5,0.5));

    this->sceneMgr->setShadowTexturePixelFormat(Ogre::PF_FLOAT16_R);
    this->sceneMgr->setShadowTextureSelfShadow(true);
    this->sceneMgr->setShadowCasterRenderBackFaces(false);

    this->sceneMgr->setShadowFarDistance(20);
  }

  // Add a sky dome to our scene
  if (node && node->GetChild("sky"))
  {
    this->skyMaterialP->Load(node->GetChild("sky"));
    OgreCreator::CreateSky(**(this->skyMaterialP));
  }

  // Add fog. This changes the background color
  if (node)
    OgreCreator::CreateFog(node->GetChild("fog"));

  // Set up the world geometry link
  if (this->sceneType==SCENE_BSP)
  {
    if (node)
      this->worldGeometry = node->GetString("bsp","",1);

    try
    {
      this->sceneMgr->setWorldGeometry(this->worldGeometry);
    }
    catch (Ogre::Exception e)
    {
      gzmsg(-1) << "Unable to load BSP geometry." << e.getDescription() << "\n";
      exit(-1);
    }
  }

  this->raySceneQuery = this->sceneMgr->createRayQuery( Ogre::Ray() );
  this->raySceneQuery->setSortByDistance(true);
  this->raySceneQuery->setQueryMask(Ogre::SceneManager::ENTITY_TYPE_MASK);
*/

  if (this->HasGLSL())
  {
    //RTShaderSystem::Instance()->Init();
    RTShaderSystem::Instance()->UpdateShaders();
  }
}


////////////////////////////////////////////////////////////////////////////////
/// Finalize
void OgreAdaptor::Fini()
{
  if (this->HasGLSL())
    RTShaderSystem::Instance()->Fini();
}
 
////////////////////////////////////////////////////////////////////////////////
// Save
void OgreAdaptor::Save(std::string &prefix, std::ostream &stream)
{
  stream << prefix << "<rendering:ogre>\n";
  this->scenes[0]->Save(prefix,stream);
  stream << prefix << "</rendering:ogre>\n";
}

////////////////////////////////////////////////////////////////////////////////
// Load plugins
void OgreAdaptor::LoadPlugins()
{
  std::list<std::string>::iterator iter;
  std::list<std::string> ogrePaths=Simulator::Instance()->GetGazeboConfig()->GetOgrePaths();
 
  for (iter=ogrePaths.begin(); 
       iter!=ogrePaths.end(); ++iter)
  {
    std::string path(*iter);
    DIR *dir=opendir(path.c_str()); 

    if (dir == NULL)
    {
      continue;
    }
    closedir(dir);

    std::vector<std::string> plugins;
    std::vector<std::string>::iterator piter;

    plugins.push_back(path+"/RenderSystem_GL.so");
    plugins.push_back(path+"/Plugin_ParticleFX.so");
    plugins.push_back(path+"/Plugin_BSPSceneManager.so");
    plugins.push_back(path+"/Plugin_OctreeSceneManager.so");
    plugins.push_back(path+"/Plugin_CgProgramManager.so");

    for (piter=plugins.begin(); piter!=plugins.end(); piter++)
    {
      try
      {
        // Load the plugin into OGRE
        this->root->loadPlugin(*piter);
      }
      catch (Ogre::Exception e)
      {
        std::string description("Unable to load Ogre Plugin[");
        description.append(*piter);
        description.append("]...Skipping.");
        gzerr(0) << description << "\n";
      }
    }
  }

}

////////////////////////////////////////////////////////////////////////////////
// Setup resources
void OgreAdaptor::SetupResources()
{
  std::vector<std::string> archNames;
  std::vector<std::string>::iterator aiter;
  std::list<std::string>::iterator iter;

  for (iter=Simulator::Instance()->GetGazeboConfig()->GetGazeboPaths().begin();
       iter!=Simulator::Instance()->GetGazeboConfig()->GetGazeboPaths().end(); iter++)
  {
    DIR *dir;
    if ((dir=opendir((*iter).c_str())) == NULL)
    {
      continue;
    }
    closedir(dir);

    archNames.push_back((*iter)+"/");
    archNames.push_back((*iter)+"/Media");
    archNames.push_back((*iter)+"/Media/fonts");
    archNames.push_back((*iter)+"/Media/rtshaderlib");
    archNames.push_back((*iter)+"/Media/materials/programs");
    archNames.push_back((*iter)+"/Media/materials/scripts");
    archNames.push_back((*iter)+"/Media/materials/textures");
    archNames.push_back((*iter)+"/Media/models");
    archNames.push_back((*iter)+"/Media/sets");
    archNames.push_back((*iter)+"/Media/maps");

    //we want to add all the material files of the sets
    if ((dir=opendir(((*iter)+"/Media/sets").c_str()))!= NULL)
    {
      std::string filename;
      struct dirent *dir_entry_p;
      while ( (dir_entry_p = readdir(dir))!=NULL )
      {
        filename =(*iter)+"/Media/sets/"+ dir_entry_p->d_name;
        archNames.push_back(filename);
      }
      closedir(dir);
    }

    for (aiter=archNames.begin(); aiter!=archNames.end(); aiter++)
    {
      try
      {
        Ogre::ResourceGroupManager::getSingleton().addResourceLocation( *aiter, "FileSystem", "General");
      }
      catch (Ogre::Exception)
      {
        gzthrow("Unable to load Ogre Resources.\nMake sure the resources path in the world file is set correctly.");
      }
    }
  }

}

////////////////////////////////////////////////////////////////////////////////
// Setup render system
void OgreAdaptor::SetupRenderSystem()
{
  Ogre::RenderSystem *renderSys;
  const Ogre::RenderSystemList *rsList;

  // Set parameters of render system (window size, etc.)
#if OGRE_VERSION_MAJOR == 1 && OGRE_VERSION_MINOR == 6
    rsList = this->root->getAvailableRenderers();
#else
    rsList = &(this->root->getAvailableRenderers());
#endif

  int c = 0;

  renderSys = NULL;

  do
  {
    if (c == (int)rsList->size())
      break;

    renderSys = rsList->at(c);
    c++;
  }
  while (renderSys->getName().compare("OpenGL Rendering Subsystem")!= 0);

  if (renderSys == NULL)
    gzthrow( "unable to find rendering system" );

  // We operate in windowed mode
  renderSys->setConfigOption("Full Screen","No");

  /// We used to allow the user to set the RTT mode to PBuffer, FBO, or Copy. 
  ///   Copy is slow, and there doesn't seem to be a good reason to use it
  ///   PBuffer limits the size of the renderable area of the RTT to the
  ///           size of the first window created.
  ///   FBO seem to be the only good option
  renderSys->setConfigOption("RTT Preferred Mode", "FBO");

  renderSys->setConfigOption("FSAA", "2");

  /* Print out the list of options
  Ogre::ConfigOptionMap map = renderSys->getConfigOptions();
  Ogre::ConfigOptionMap::iterator iter;

  printf("KEYS-------------------------\n");
  for (iter = map.begin(); iter != map.end(); iter++)
  {
    std::cout << "Key[" << iter->first << "] Name[" << iter->second.name << "] Value[" << iter->second.currentValue << "]\n";
  }
  */

  this->root->setRenderSystem(renderSys);

}

////////////////////////////////////////////////////////////////////////////////
/// Get a scene 
Scene *OgreAdaptor::GetScene(unsigned int index)
{
  if (index < this->scenes.size())
    return this->scenes[index];
  else
  {
/*
    DIAGNOSTICTIMER(timer("UpdateCameras: Non-UserCamera update",6));
    boost::recursive_mutex::scoped_lock model_render_lock(*Simulator::Instance()->GetMRMutex());
    boost::recursive_mutex::scoped_lock model_delete_lock(*Simulator::Instance()->GetMDMutex());
    for (iter = this->cameras.begin(); iter != this->cameras.end(); iter++)
    {
      if (dynamic_cast<UserCamera*>((*iter)) == NULL)
        (*iter)->Render();
    }
*/
    std::cerr << "Invalid Scene Index[" << index << "]\n";
    return NULL;
  }
/*
  // Must update the user camera's last.
  {
    DIAGNOSTICTIMER(timer("UpdateCameras: UserCamera update",6));
    for (iter = this->cameras.begin(); iter != this->cameras.end(); iter++)
    {
      userCam = dynamic_cast<UserCamera*>((*iter));
      if (userCam)
        userCam->Update();
    }
  }

  {
    DIAGNOSTICTIMER(timer("UpdateCameras: _fireFrameEnded",6));
    this->root->_fireFrameEnded();
  }
*/
}

////////////////////////////////////////////////////////////////////////////////
/// Get the number of scene 
unsigned int OgreAdaptor::GetSceneCount() const
{
/*
  Entity *entity = NULL;
  Ogre::Camera *ogreCam = camera->GetOgreCamera();
  Ogre::Vector3 camPos = ogreCam->getPosition();

  Ogre::Real closest_distance = -1.0f;
  Ogre::Ray mouseRay = ogreCam->getCameraToViewportRay(
      (float)mousePos.x / ogreCam->getViewport()->getActualWidth(), 
      (float)mousePos.y / ogreCam->getViewport()->getActualHeight() );

  this->raySceneQuery->setRay( mouseRay );

  // Perform the scene query
  Ogre::RaySceneQueryResult &result = this->raySceneQuery->execute();
  Ogre::RaySceneQueryResult::iterator iter = result.begin();
  Ogre::Entity *closestEntity = NULL;

  for (iter = result.begin(); iter != result.end(); iter++)
  {
    // is the result a MovableObject
    if (iter->movable && iter->movable->getMovableType().compare("Entity") == 0)
    {
      Ogre::Entity *pentity = static_cast<Ogre::Entity*>(iter->movable);

      // mesh data to retrieve         
      size_t vertex_count;
      size_t index_count;
      Ogre::Vector3 *vertices;
      unsigned long *indices;

      // Get the mesh information
      OgreCreator::GetMeshInformation(pentity->getMesh(), vertex_count, 
          vertices, index_count, indices,             
          pentity->getParentNode()->_getDerivedPosition(),
          pentity->getParentNode()->_getDerivedOrientation(),
          pentity->getParentNode()->_getDerivedScale());

      bool new_closest_found = false;
      for (int i = 0; i < static_cast<int>(index_count); i += 3)
      {
        int vert0 = indices[i];
        int vert1 = indices[i+1];
        int vert2 = indices[i+2];

        if (vert0 < vertex_count && vert1 < vertex_count && vert2 < vertex_count)
        {
          // check for a hit against this triangle
          std::pair<bool, Ogre::Real> hit = Ogre::Math::intersects(mouseRay, vertices[indices[i]], vertices[indices[i+1]], vertices[indices[i+2]], true, false);

          // if it was a hit check if its the closest
          if (hit.first)
          {
            if ((closest_distance < 0.0f) || (hit.second < closest_distance))
            {
              // this is the closest so far, save it off
              closest_distance = hit.second; 
              new_closest_found = true;
            }
          }
        }
        else
        {
          fprintf(stderr,"OgreAdapter Ray Trace Select Entity Error: vertex indices (%d,%d,%d) out of range! [>%d]\n",vert0,vert1,vert2,(int)vertex_count);
        }
      }

      delete [] vertices;
      delete [] indices;

      if (new_closest_found)
      {
        closestEntity = pentity;
        break;
      }
    }
  }

  mod = "";
  if (closestEntity)
  {
    if (closestEntity->getUserAny().getType() == typeid(std::string))
      mod = Ogre::any_cast<std::string>(closestEntity->getUserAny());

    OgreVisual* const* vis = Ogre::any_cast<OgreVisual*>(&closestEntity->getUserAny());

    if (vis && (*vis)->GetOwner())
    {
      entity = (*vis)->GetOwner();
      return entity;
    }
  }

  return NULL;
*/
  return this->scenes.size();
}

////////////////////////////////////////////////////////////////////////////////
/// Update all the scenes 
void OgreAdaptor::UpdateScenes()
{
  Events::renderStartSignal();

  this->root->_fireFrameStarted();

  OgreCreator::Instance()->Update();

  for (unsigned int i=0; i < this->scenes.size(); i++)
    this->scenes[i]->UpdateCameras();

  this->root->_fireFrameRenderingQueued();

  this->root->_fireFrameEnded();
}


////////////////////////////////////////////////////////////////////////////////
// Returns true if the graphics card support GLSL
bool OgreAdaptor::HasGLSL()
{
  const Ogre::RenderSystemCapabilities *capabilities;
  Ogre::RenderSystemCapabilities::ShaderProfiles profiles;
  Ogre::RenderSystemCapabilities::ShaderProfiles::const_iterator iter;

  capabilities = this->root->getRenderSystem()->getCapabilities();
  profiles = capabilities->getSupportedShaderProfiles();

  iter = std::find(profiles.begin(), profiles.end(), "glsl");

  // Print all the shader profiles
  /*std::cout << "Shader profiles:\n";
  for (iter = profiles.begin(); iter != profiles.end(); iter++)
  {
    std::cout << *iter << "\n";
  }*/

  return iter != profiles.end();
}

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
/* Desc: Heightmap geometry
 * Author: Nate Keonig
 * Date: 12 May 2009
 * SVN: $Id$
 */

#include <Ogre.h>
#include <iostream>
#include <string.h>
#include <math.h>

#include "Scene.hh"
#include "Image.hh"
#include "GazeboError.hh"
#include "OgreAdaptor.hh"
#include "Simulator.hh"
#include "OgreHeightmap.hh"

using namespace gazebo;

//////////////////////////////////////////////////////////////////////////////
// Constructor
OgreHeightmap::OgreHeightmap(unsigned int sceneIndex)
{
  this->scene = OgreAdaptor::Instance()->GetScene(sceneIndex);
}


//////////////////////////////////////////////////////////////////////////////
// Destructor
OgreHeightmap::~OgreHeightmap()
{
  this->scene->GetManager()->destroyQuery(this->rayQuery);
}

//////////////////////////////////////////////////////////////////////////////
/// get height at a point
float OgreHeightmap::GetHeightAt(const Vector2<float> &pos)
{
  Ogre::Vector3 pos3(pos.x, this->terrainSize.z,pos.y);

  this->ray.setOrigin(pos3);
  this->rayQuery->setRay(this->ray);
  this->distToTerrain = 0;
  this->rayQuery->execute(this);

  return this->terrainSize.z - this->distToTerrain;
}

////////////////////////////////////////////////////////////////////////////////
/// Overloaded Ogre function for Ray Scene Queries
bool OgreHeightmap::queryResult(Ogre::MovableObject *obj, Ogre::Real dist)
{
  return false;
}

////////////////////////////////////////////////////////////////////////////////
/// Overloaded Ogre function for Ray Scene Queries
bool OgreHeightmap::queryResult(Ogre::SceneQuery::WorldFragment *frag, Ogre::Real dist)
{
  this->distToTerrain = dist;
  return false;
}

////////////////////////////////////////////////////////////////////////////////
// Load the heightmap
void OgreHeightmap::Load( std::string imageFilename, 
                          std::string worldTexture, 
                          std::string detailTexture,
                          Vector3 _terrainSize)
{
  std::ostringstream stream;
  unsigned int terrainVertSize;
  int tileSize;
  Image img;

  this->terrainSize = _terrainSize;

   // Use the image to get the size of the heightmap
  img.Load(imageFilename);

  // Width and height must be the same
  if (img.GetWidth() != img.GetHeight())
  {
    gzthrow("Heightmap image must be square\n");
  }

  terrainVertSize = img.GetWidth();

  float nf = (float)(log(terrainVertSize-1)/log(2));
  int ni = (int)(log(terrainVertSize-1)/log(2));

  // Make sure the heightmap image size is (2^n)+1 in size
  if ( nf - ni != 0)
  {
    gzthrow("Heightmap image size must be (2^n)+1\n");
  }

  // Calculate a good tile size
  tileSize = (int)(pow( 2,  ni/2 ));

  if (tileSize <= 2)
  {
    tileSize = 4;
  }

  tileSize++;

  /*std::cout << "ODE Scale[" << this->odeScale << "]\n";
  std::cout << "Terrain Image[" << this->imageFilenameP->GetValue() << "] Size[" << this->terrainSize << "]\n";
  printf("Terrain Size[%f %f %f]\n", this->terrainSize.x, this->terrainSize.y, this->terrainSize.z);
  printf("VertSize[%d] Tile Size[%d]\n", terrainVertSize, tileSize);
  */

  stream << "WorldTexture=" << worldTexture << "\n";
  //The detail texture
  stream << "DetailTexture=" << detailTexture << "\n";
  // number of times the detail texture will tile in a terrain tile
  stream << "DetailTile=3\n";
  // Heightmap source
  stream << "PageSource=Heightmap\n";
  // Heightmap-source specific settings
  stream << "Heightmap.image=" << imageFilename << "\n";
  // How large is a page of tiles (in vertices)? Must be (2^n)+1
  stream << "PageSize=" << terrainVertSize << "\n";
  // How large is each tile? Must be (2^n)+1 and be smaller than PageSize
  stream << "TileSize=" << tileSize << "\n";
  // The maximum error allowed when determining which LOD to use
  stream << "MaxPixelError=4\n";
  // The size of a terrain page, in world units
  stream << "PageWorldX=" << this->terrainSize.x << "\n";
  stream << "PageWorldZ=" << this->terrainSize.y << "\n";
  // Maximum height of the terrain
  stream << "MaxHeight="<< this->terrainSize.z << "\n";
  // Upper LOD limit
  stream << "MaxMipMapLevel=2\n";

  // Create a data stream for loading the terrain into Ogre
  char *mstr = strdup(stream.str().c_str());

  Ogre::DataStreamPtr dataStream(
    new Ogre::MemoryDataStream(mstr,strlen(mstr)) );

  // Set the static terrain in Ogre
  this->scene->GetManager()->setWorldGeometry(dataStream);

  // HACK to make the terrain oriented properly
  Ogre::SceneNode *tnode = this->scene->GetManager()->getSceneNode("Terrain");
  tnode->pitch(Ogre::Degree(90));
  tnode->translate(Ogre::Vector3(-this->terrainSize.x*0.5, this->terrainSize.y*0.5, 0));

  // Setup the ray scene query, which is used to determine the heights of
  // the vertices for ODE
  this->ray = Ogre::Ray(Ogre::Vector3::ZERO, Ogre::Vector3::NEGATIVE_UNIT_Y);
  this->rayQuery = this->scene->GetManager()->createRayQuery(this->ray);
  this->rayQuery->setQueryTypeMask(Ogre::SceneManager::WORLD_GEOMETRY_TYPE_MASK);
  this->rayQuery->setWorldFragmentType(Ogre::SceneQuery::WFT_SINGLE_INTERSECTION);

  free(mstr);
}

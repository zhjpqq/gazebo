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
/* Desc: The Simulator; Top level managing object
 * Author: Jordi Polo
 * Date: 3 Jan 2008
 */

#ifndef SIMULATOR_HH
#define SIMULATOR_HH

#include <string>
#include <boost/thread.hpp>

#include "SingletonT.hh"

namespace boost
{
  class recursive_mutex;
}

namespace gazebo
{
/// \addtogroup gazebo_server
/// \brief The World
/// \{

// Forward declarations
  class GuiAPI;
  class Server;
  class SimulationIface;
  class XMLConfig;
  class XMLConfigNode;
  class GazeboConfig;
  class OgreAdaptor;
  class Entity;
  class Body;
  class Model;

/// \brief The World
/*
 * Top level class: Takes care of World Gui SimulatorIface and Server
 * 
 */
  class Simulator : public SingletonT<Simulator>
  {
    /// \brief Private constructor
    private: Simulator();

    /// \brief Private destructor
    private: ~Simulator();

    /// \brief Closes the present simulation, frees the resources 
    public: void Close();

    /// \brief Load the world configuration file 
    public: void Load(const std::string &worldFileName, unsigned int serverId );

    /// \brief Save the world configuration file
    public: void Save(const std::string& filename=std::string());

    /// \brief Initialize the simulation
    public: void Init( );

    /// \brief Finalize the simulation
    public: void Fini( );

    /// \brief Main simulation loop, when this loop ends the simulation finish
    public: void MainLoop();

    /// \brief Gets the local configuration for this computer
    public: GazeboConfig *GetGazeboConfig() const;

    /// \brief Gets our current GUI interface
    public: OgreAdaptor *GetRenderEngine() const;

    /// \brief Returns the state of the simulation true if paused
    public: bool IsPaused() const;

    /// \brief Set whether the simulation is paused
    public: void SetPaused(bool p);

    /// \brief Get the number of iterations
    public: unsigned long GetIterations() const;
/*
    /// \brief Set the number of iterations
    public: static void SetIterations(unsigned long count);

    /// \brief Increment the number of iterations
    public: static void IncIterations();
*/
    /// Get the simulation time
    /// \return The simulation time
    public: double GetSimTime() const;

    /// Get the pause time
    /// \return The pause time
    public: double GetPauseTime() const;

    /// Get the start time
    /// \return The start time
    public: double GetStartTime() const;

    /// Get the real time (elapsed time)
    /// \return The real time
    public: double GetRealTime() const;

    /// \brief Get the wall clock time
    /// \return The wall clock time
    public: double GetWallTime() const;

    //User Iteractions
    /// \brief Simulator finished by the user
    public: void SetUserQuit();

    /// \brief Return true if the user has pased
    public: bool GetUserPause() const;

    /// \brief Set whether the user has paused
    public: void SetUserPause(bool pause);
    
    /// \brief Return true if the step has incremented
    public: bool GetUserStepInc() const;

    /// \brief Set whether the step has incremented
    public: void SetUserStepInc(bool step);

    /// \brief True if the gui is to be used
    public: void SetGuiEnabled( bool enabled );

    /// \brief Return true if the gui is enabled
    public: bool GetGuiEnabled() const;

    /// \brief True if the gui is to be used
    public: void SetRenderEngineEnabled( bool enabled );

    /// \brief Return true if the gui is enabled
    public: bool GetRenderEngineEnabled() const;

    /// \brief Set the length of time the simulation should run.
    public: void SetTimeout(double time);

    /// \brief Set the physics enabled/disabled
    public: void SetPhysicsEnabled(bool enabled);

    /// \brief Get the physics enabled/disabled
    public: bool GetPhysicsEnabled() const;

    /// \brief Set the selected entity
    public: void SetSelectedEntity( Entity *ent );

    /// \brief Set the selected body
    public: void SetSelectedBody( Body *bod );

    /// \brief Get the selected entity
    public: Entity *GetSelectedEntity() const;

    /// \brief Get the model that contains the entity
    public: Model *GetParentModel( Entity *entity ) const;

    /// \brief Get the body that contains the entity
    public: Body *GetParentBody( Entity *entity ) const;

    /// \brief Get the simulator mutex
    public:boost::recursive_mutex *GetMutex();

    /// \brief Function to run gui. Used by guiThread
    private: void PhysicsLoop();

    ///pointer to the XML Data
    private: XMLConfig *xmlFile;

    /// Pointer to the selected Gui 
    private: GuiAPI *gui;

    private: OgreAdaptor *renderEngine;

    private: GazeboConfig *gazeboConfig;

    /// Flag to know if we have a simulation loaded
    private: bool loaded;

    /// Flag set if simulation is paused
    private: bool pause;

    ///  Count of the number of iterations
    private: unsigned long iterations;

    /// Current simulation time
    private: double simTime, pauseTime, startTime;
    private: double prevPhysicsTime, prevRenderTime;

    //upper limits on updating
    //how many updates we have done in this slot
    private: int physicsUpdates;

    // when the slot started
    private: double checkpoint;

    // render updates
    private: int  renderUpdates;

    // UserIteractions 
    /// \brief Set to true to pause the simulation
    private: bool userPause;

    /// Set to true to increment the simulation once. This is only
    ///  valid when paused.
    private: bool userStepInc;

    //The user has somewhat signaled the end of the program
    private: bool userQuit;

    /// True if the GUI is enabled
    private: bool guiEnabled;

    /// True if the Rendering Engine is enabled
    private: bool renderEngineEnabled;

    /// True if physics is enabled
    private: bool physicsEnabled;

    /// Length of time the simulation should run
    private: double timeout;

    /// Length of time the simulator is allowed to run
    private: double updateTime;

    /// The entity currently selected by the user
    private: Entity *selectedEntity;
    private: Body *selectedBody;

    /// Thread in which to run the gui
    private: boost::thread *physicsThread;

    private: boost::recursive_mutex *mutex;

    //Singleton implementation
    private: friend class DestroyerT<Simulator>;
    private: friend class SingletonT<Simulator>;
};

/// \}
}

#endif

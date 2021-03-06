/*
 * Software License Agreement (BSD License)
 *
 *   Pose Estimation Library (PEL) - https://bitbucket.org/Tabjones/pose-estimation-library
 *   Copyright (c) 2014-2015, Federico Spinelli (fspinelli@gmail.com)
 *   All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of the copyright holder(s) nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef PEL_DATABASE_DATABASE_CREATOR_H_
#define PEL_DATABASE_DATABASE_CREATOR_H_

#include <pel/common.h>
#include <pel/param_handler.h>
#include <fstream>
#include <pcl/io/pcd_io.h>
#include <boost/range/algorithm/copy.hpp>
#include <boost/range/algorithm/sort.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>

namespace pel
{
  class Database;
  /**\brief Creates a Database from acquired poses, uses inherited parameters handle.
   *
   * Example usage:
   * \code
   * #include <pel/database/database_creator.h>
   * #include <pel/database/database_io.h>
   * #include <pel/database/database.h>
   * //...
   * pel::Database db; //Empty Database
   * pel::DatabaseCreator creator;
   * db = creator.create("path_to_pcd_poses");
   * //... optionally save the newly created database to disk
   * pel::DatabaseWriter writer;
   * writer.save ("db_location", db);
   * //... Create another one with different parameters
   * creator.setParam("use_esf", 0); //Disable ESF descriptor
   * db = creator.create("path_to_pcd_poses");
   * \endcode
   * \note When creating databases, please note that database poses and target of pose estimation should have the same parameters.
   * In the above example, the second database does not contain ESF descriptor, hence also the Pose Estimation procedure must disable ESF, when using that database.
   * \author Federico Spinelli
   */
  class DatabaseCreator : public ParamHandler
  {
    public:
      /**\brief Empty Constructor
      */
      DatabaseCreator () {}

      /**\brief Empty Destructor.
      */
      virtual ~DatabaseCreator () {}

      /**\brief Create a database from a set of poses
       * \param[in] path_clouds Path to a directory that contains pcds of poses to create database from.
       * \returns Database created from supplied poses, or empty one if failed.
       *
       * \note Each pcd file must have unique names, must contain only the point cloud of the object pose and it must be expressed in a local reference frame.
       * The local reference frame can be any, but must be the same for all the poses of the same object. For example you could chose as origin the object centroid,
       * as _z-axis_ the "up" direction and as _x-axis_ an object feature (like and handle).
       * Then all the poses representing the same object must retain the same reference frame. For more information on this you can look at the \ref databse "Database section of the User Guide".
       * \note Building a database from scratch could take several minutes, depending on how many poses are supplied, and your processing power.
       * \note Default parameters are used, if you want to change them use methods inherited by ParamHandler.
       */
      Database
      create (boost::filesystem::path path_clouds);

  };

}
#endif //PEL_DATABASE_DATABASE_CREATOR_H_

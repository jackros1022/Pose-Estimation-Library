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
 * * Neither the name of copyright holder(s) nor the names of its
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

#include <pel/pose_estimation.h>

namespace pel
{
  /* Disable it for now... TODO
  PoseEstimation::PoseEstimation ()
  {
    query_set_ = candidates_found_ = refinement_done_ = false;
    feature_count_ = 4;
    PtC a;
    query_cloud_=a.makeShared();
    params_["verbosity"]=1;
    params_["useVFH"]=params_["useESF"]=params_["useCVFH"]=params_["useOURCVFH"]=1;
    params_["progBisection"]=params_["downsampling"]=1;
    params_["vgridLeafSize"]=0.005f;
    params_["upsampling"]=params_["filtering"]=0;
    params_["kNeighbors"]=20;
    params_["maxIterations"]=200;
    params_["progItera"]=10;
    params_["icpReciprocal"]=1;
    params_["progFraction"]=0.5f;
    params_["rmseThreshold"]=0.003f;
    params_["mlsPolyOrder"]=2;
    params_["mlsPointDensity"]=200;
    params_["mlsPolyFit"]=1;
    params_["mlsSearchRadius"]=0.05f;
    params_["filterMeanK"]=50;
    params_["filterStdDevMulThresh"]=3;
    params_["neRadiusSearch"]=0.02;
    params_["cvfhEPSAngThresh"]=7.5;
    params_["cvfhCurvThresh"]=0.025;
    params_["cvfhClustTol"]=0.01;
    params_["cvfhMinPoints"]=50;
    params_["ourcvfhEPSAngThresh"]=7.5;
    params_["ourcvfhCurvThresh"]=0.025;
    params_["ourcvfhClustTol"]=0.01;
    params_["ourcvfhMinPoints"]=50;
    params_["ourcvfhAxisRatio"]=0.95;
    params_["ourcvfhMinAxisValue"]=0.01;
    params_["ourcvfhRefineClusters"]=1;
  }
  ////////////////////////////////////////////////////////////////////////////////////////
  bool PoseEstimation::setParam(string key, float value)
  {
    int size = params_.size();
    if (value < 0)
    {
      if (params_["verbosity"]>0)
        print_warn("%*s]\tParameter '%s' has a negative value (%g), ignoring...\n", 20,__func__, key.c_str(), value);
      return false;
    }
    params_[key]=value;
    //Check if key was a valid one, since the class has fixed number of parameters,
    //if one was mispelled, now we have one more
    if (params_.size() != size)
    {
      if (params_["verbosity"]>0)
        print_warn("%*s]\tInvalid key parameter '%s', ignoring...\n", 20,__func__, key.c_str());
      params_.erase(key);
      return false;
    }
    else if (params_["verbosity"]>1)
      print_info("%*s]\tSetting parameter: %s=%g\n",20,__func__,key.c_str(),value);
    //Recheck how many features we want
    int count(0);
    if (params_["useVFH"]>=1)
      count++;
    if (params_["useESF"]>=1)
      count++;
    if (params_["useCVFH"]>=1)
      count++;
    if (params_["useOURCVFH"]>=1)
      count++;
    feature_count_ = count;
    if (feature_count_ <=0 && params_["verbosity"]>0)
      print_warn("%*s]\tYou disabled all features, PoseEstimation will not initialize query...\n",20,__func__);
    return true;
  }
  /////////////////////////////////////////////////////////////////////////////////////
  bool PoseEstimation::setParam_ (string key, string& value)
  {
    float f;
    try
    {
      f = stof(value);
    }
    catch (const std::invalid_argument& ia)
    {
      if (params_["verbosity"] > 0)
        print_warn("%*s]\tInvalid %s=%s : %s \n",20,__func__, key.c_str(), value.c_str(), ia.what());
      return false;
    }
    catch (const std::out_of_range& oor)
    {
      if (params_["verbosity"] > 0)
        print_warn("%*s]\tInvalid %s=%s : %s \n", 20,__func__, key.c_str(), value.c_str(), oor.what());
      return false;
    }
    return (this->setParam(key, f) );
  }
  //////////////////////////////////////////////////////////////////////////////////////
  int PoseEstimation::initParams(boost::filesystem::path config_file)
  {
    if ( exists(config_file) && is_regular_file(config_file))
    {
      if (extension(config_file)==".conf")
      {
        ifstream file (config_file.string().c_str());
        string line;
        if (file.is_open())
        {
          int count (0);
          while (getline (file, line))
          {
            trim(line); //remove white spaces from line
            if (line.empty())
            {
              //do nothing empty line ...
              continue;
            }
            else if (line.compare(0,1,"%") == 0 )
            {
              //do nothing comment line ...
              continue;
            }
            else
            {
              vector<string> vst;
              //split the line to get a key and a token
              split(vst, line, boost::is_any_of("="), boost::token_compress_on);
              if (vst.size()!=2)
              {
                if (params_["verbosity"]>0)
                  print_warn("%*s]\tInvalid configuration line (%s), ignoring... Must be [Token]=[Value]\n", 20,__func__, line.c_str());
                continue;
              }
              if (setParam_(vst.at(0), vst.at(1)))
              {
                //success
                ++count;
              }
            }
          }//end of config file
          file.close();
          return count;
        }
        else
        {
          print_error("%*s]\tCannot open config file! (%s)\n", 20,__func__, config_file.string().c_str());
          return (-1);
        }
      }
      else
      {
        print_error("%*s]\tConfig file provided (%s) has no valid extension! (must be .conf)\n", 20,__func__, config_file.string().c_str());
        return (-1);
      }
    }
    else
    {
      print_error("%*s]\tPath to configuration file is not valid ! (%s)\n", 20,__func__, config_file.string().c_str());
      return (-1);
    }
  }
  ////////////////////////////////////////////////////////////////////////////////////////////////////
  int PoseEstimation::initParams(boost::shared_ptr<parameters> map)
  {
    //set parameters one by one so we can spot wrong key or values
    if (map)
    {
      try
      {
        int count (0);
        for (auto& x : *map)
        {
          if( setParam(x.first, x.second) )
            ++count;
        }
        return count;
      }
      catch (...)
      {
        print_error("%*s]\tUnexpected error in setting parameters...\n",20,__func__);
        return (-1);
      }
    }
    else
    {
      if (params_["verbosity"]>0)
        print_warn("%*s]\tProvided map of parameters looks empty, ignoring...\n");
      return (-1);
    }
  }
  void PoseEstimation::filtering_()
  {
    StopWatch timer;
    if (params_["verbosity"] >1)
    {
      print_info("%*s]\tSetting Statistical Outlier Filter to preprocess query cloud...\n",20,__func__);
      print_info("%*s]\tSetting mean K to %g\n",20,__func__, params_["filterMeanK"]);
      print_info("%*s]\tSetting Standard Deviation multiplier to %g\n",20,__func__, params_["filterStdDevMulThresh"]);
      timer.reset();
    }
    PtC::Ptr filtered (new PtC);
    StatisticalOutlierRemoval<Pt> fil;
    fil.setMeanK (params_["filterMeanK"]);
    fil.setStddevMulThresh (params_["filterStdDevMulThresh"]);
    fil.setInputCloud(query_cloud_);
    fil.filter(*filtered);
    copyPointCloud(*filtered, *query_cloud_);
    if (params_["verbosity"]>1)
    {
      print_info("%*s]\tTotal time elapsed during filter: ",20,__func__);
      print_value("%g", timer.getTime());
      print_info(" ms\n");
    }
  }
  //////////////////////////////////////////////////////////////////////////////////////////////////
  void PoseEstimation::upsampling_()
  {
    StopWatch timer;
    if (params_["verbosity"] >1)
    {
      print_info("%*s]\tSetting MLS with Random Uniform Density to preprocess query cloud...\n",20,__func__);
      print_info("%*s]\tSetting polynomial order to %g\n",20,__func__, params_["mlsPolyOrder"]);
      string t = params_["mlsPolyFit"] ? "true" : "false";
      print_info("%*s]\tSetting polynomial fit to %s\n",20,__func__, t.c_str());
      print_info("%*s]\tSetting desired point density to %g\n",20,__func__, params_["mlsPointDensity"]);
      print_info("%*s]\tSetting search radius to %g\n",20,__func__, params_["mlsSearchRadius"]);
      timer.reset();
    }
    PtC::Ptr upsampled (new PtC);
    search::KdTree<Pt>::Ptr tree (new search::KdTree<Pt>);
    MovingLeastSquares<Pt, Pt> mls;
    mls.setInputCloud(query_cloud_);
    mls.setSearchMethod(tree);
    mls.setUpsamplingMethod (MovingLeastSquares<Pt, Pt>::RANDOM_UNIFORM_DENSITY);
    mls.setComputeNormals (false);
    mls.setPolynomialOrder(params_["mlsPolyOrder"]);
    mls.setPolynomialFit(params_["mlsPolyFit"]);
    mls.setSearchRadius(params_["mlsSearchRadius"]);
    mls.setPointDensity(params_["mlsPointDensity"]);
    mls.process(*upsampled);
    copyPointCloud(*upsampled, *query_cloud_);
    if (params_["verbosity"]>1)
    {
      print_info("%*s]\tTotal time elapsed during upsampling: ",20,__func__);
      print_value("%g", timer.getTime());
      print_info(" ms\n");
    }
  }
  ////////////////////////////////////////////////////////////////////////////////////////////////////
  void PoseEstimation::downsampling_()
  {
    StopWatch timer;
    if (params_["verbosity"] >1)
    {
      print_info("%*s]\tSetting Voxel Grid to preprocess query cloud...\n",20,__func__);
      print_info("%*s]\tSetting Leaf Size to %g\n",20,__func__, params_["vgridLeafSize"]);
      timer.reset();
    }
    PtC::Ptr downsampled (new PtC);
    VoxelGrid<Pt> vg;
    vg.setInputCloud(query_cloud_);
    vg.setLeafSize (params_["vgridLeafSize"], params_["vgridLeafSize"], params_["vgridLeafSize"]);
    vg.setDownsampleAllData (true);
    vg.filter(*downsampled);
    copyPointCloud(*downsampled, *query_cloud_);
    if (params_["verbosity"]>1)
    {
      print_info("%*s]\tTotal time elapsed during downsampling: ",20,__func__);
      print_value("%g", timer.getTime());
      print_info(" ms\n");
    }
  }
  /////////////////////////////////////////////////////////////////////////////////////////////////
  bool PoseEstimation::computeNormals_()
  {
    StopWatch timer;
    if (params_["verbosity"]>1)
    {
      print_info("%*s]\tSetting normal estimation to calculate query normals...\n",20,__func__);
      print_info("%*s]\tSetting a neighborhood radius of %g\n",20,__func__, params_["neRadiusSearch"]);
      timer.reset();
    }
    NormalEstimationOMP<Pt, Normal> ne;
    search::KdTree<Pt>::Ptr tree (new search::KdTree<Pt>);
    ne.setSearchMethod(tree);
    ne.setRadiusSearch(params_["neRadiusSearch"]);
    ne.setNumberOfThreads(0); //use pcl autoallocation
    ne.setInputCloud(query_cloud_);
    ne.useSensorOriginAsViewPoint();
    ne.compute(normals_);
    if (params_["verbosity"]>1)
    {
      print_info("%*s]\tTotal time elapsed during normal estimation: ",20,__func__);
      print_value("%g", timer.getTime());
      print_info(" ms\n");
    }
    return true;
  }
  //////////////////////////////////////////////////////////////////////////////////////////////
  void PoseEstimation::computeVFH_()
  {
    StopWatch timer;
    if (params_["verbosity"]>1)
    {
      print_info("%*s]\tEstimating VFH feature of query...\n",20,__func__);
      timer.reset();
    }
    VFHEstimation<Pt, Normal, VFHSignature308> vfhE;
    search::KdTree<Pt>::Ptr tree (new search::KdTree<Pt>);
    vfhE.setSearchMethod(tree);
    vfhE.setInputCloud (query_cloud_);
    vfhE.setViewPoint (query_cloud_->sensor_origin_(0), query_cloud_->sensor_origin_(1), query_cloud_->sensor_origin_(2));
    vfhE.setInputNormals (normals_.makeShared());
    vfhE.compute (vfh_);
    if (params_["verbosity"]>1)
    {
      print_info("%*s]\tTotal time elapsed during VFH estimation: ",20,__func__);
      print_value("%g", timer.getTime());
      print_info(" ms\n");
    }
  }
  ////////////////////////////////////////////////////////////////////////////////////
  void PoseEstimation::computeESF_()
  {
    StopWatch timer;
    if (params_["verbosity"]>1)
    {
      print_info("%*s]\tEstimating ESF feature of query...\n",20,__func__);
      timer.reset();
    }
    ESFEstimation<Pt, ESFSignature640> esfE;
    search::KdTree<Pt>::Ptr tree (new search::KdTree<Pt>);
    esfE.setSearchMethod(tree);
    esfE.setInputCloud (query_cloud_);
    esfE.compute (esf_);
    if (params_["verbosity"]>1)
    {
      print_info("%*s]\tTotal time elapsed during ESF estimation: ",20,__func__);
      print_value("%g", timer.getTime());
      print_info(" ms\n");
    }
  }
  //////////////////////////////////////////////////////////////////////////////////////
  void PoseEstimation::computeCVFH_()
  {
    StopWatch timer;
    if (params_["verbosity"]>1)
    {
      print_info("%*s]\tEstimating CVFH feature of query...\n",20,__func__);
      print_info("%*s]\tUsing Angle Threshold of %g degress for normal deviation\n",20,__func__,params_["cvfhEPSAngThresh"]);
      print_info("%*s]\tUsing Curvature Threshold of %g\n",20,__func__,params_["cvfhCurvThresh"]);
      print_info("%*s]\tUsing Cluster Tolerance of %g\n",20,__func__,params_["cvfhClustTol"]);
      print_info("%*s]\tConsidering a minimum of %g points for a cluster\n",20,__func__,params_["cvfhMinPoints"]);
      timer.reset();
    }
    CVFHEstimation<Pt, Normal, VFHSignature308> cvfhE;
    search::KdTree<Pt>::Ptr tree (new search::KdTree<Pt>);
    cvfhE.setSearchMethod(tree);
    cvfhE.setInputCloud (query_cloud_);
    cvfhE.setViewPoint (query_cloud_->sensor_origin_(0), query_cloud_->sensor_origin_(1), query_cloud_->sensor_origin_(2));
    cvfhE.setInputNormals (normals_.makeShared());
    cvfhE.setEPSAngleThreshold(params_["cvfhEPSAngThresh"]*D2R); //angle needs to be supplied in radians
    cvfhE.setCurvatureThreshold(params_["cvfhCurvThresh"]);
    cvfhE.setClusterTolerance(params_["cvfhClustTol"]);
    cvfhE.setMinPoints(params_["cvfhMinPoints"]);
    cvfhE.setNormalizeBins(false);
    cvfhE.compute (cvfh_);
    if (params_["verbosity"]>1)
    {
      print_info("%*s]\tTotal of %d clusters were found on query\n",20,__func__, cvfh_.points.size());
      print_info("%*s]\tTotal time elapsed during CVFH estimation: ",20,__func__);
      print_value("%g", timer.getTime());
      print_info(" ms\n");
    }
  }
  //////////////////////////////////////////////////////////////////////////////////////////////////
  void PoseEstimation::computeOURCVFH_()
  {
    StopWatch timer;
    if (params_["verbosity"]>1)
    {
      print_info("%*s]\tEstimating OURCVFH feature of query...\n",20,__func__);
      print_info("%*s]\tUsing Angle Threshold of %g degress for normal deviation\n",20,__func__,params_["ourcvfhEPSAngThresh"]);
      print_info("%*s]\tUsing Curvature Threshold of %g\n",20,__func__,params_["ourcvfhCurvThresh"]);
      print_info("%*s]\tUsing Cluster Tolerance of %g\n",20,__func__,params_["ourcvfhClustTol"]);
      print_info("%*s]\tConsidering a minimum of %g points for a cluster\n",20,__func__,params_["ourcvfhMinPoints"]);
      print_info("%*s]\tUsing Axis Ratio of %g and Min Axis Value of %g during SGURF disambiguation\n",20,__func__,params_["ourcvfhAxisRatio"],params_["ourcvfhMinAxisValue"]);
      print_info("%*s]\tUsing Refinement Factor of %g for clusters\n",20,__func__,params_["ourcvfhRefineClusters"]);
      timer.reset();
    }
    OURCVFHEstimation<Pt, Normal, VFHSignature308> ourcvfhE;
    search::KdTree<Pt>::Ptr tree (new search::KdTree<Pt>);
    PointCloud<Pt>::Ptr cloud (new PointCloud<Pt>);
    copyPointCloud(*query_cloud_, *cloud);
    ourcvfhE.setSearchMethod(tree);
    ourcvfhE.setInputCloud (cloud);
    ourcvfhE.setViewPoint (cloud->sensor_origin_(0), cloud->sensor_origin_(1), cloud->sensor_origin_(2));
    ourcvfhE.setInputNormals (normals_.makeShared());
    ourcvfhE.setEPSAngleThreshold(params_["ourcvfhEPSAngThresh"]*D2R); //angle needs to be supplied in radians
    ourcvfhE.setCurvatureThreshold(params_["ourcvfhCurvThresh"]);
    ourcvfhE.setClusterTolerance(params_["ourcvfhClustTol"]);
    ourcvfhE.setMinPoints(params_["ourcvfhMinPoints"]);
    ourcvfhE.setAxisRatio(params_["ourcvfhAxisRatio"]);
    ourcvfhE.setMinAxisValue(params_["ourcvfhMinAxisValue"]);
    ourcvfhE.setRefineClusters(params_["ourcvfhRefineClusters"]);
    ourcvfhE.compute (ourcvfh_);
    if (params_["verbosity"]>1)
    {
      print_info("%*s]\tTotal of %d clusters were found on query\n",20,__func__, ourcvfh_.points.size());
      print_info("%*s]\tTotal time elapsed during OURCVFH estimation: ",20,__func__);
      print_value("%g", timer.getTime());
      print_info(" ms\n");
    }
  }
  /////////////////////////////////////////////////////////////////////////////////////////////////
  bool PoseEstimation::initQuery_()
  {
    StopWatch timer;
    if (params_["verbosity"]>1)
      timer.reset();
    if (feature_count_ <=0)
    {
      print_error("%*s]\tCannot initialize query, zero features chosen to estimate\n",20,__func__);
      return false;
    }
    if (query_cloud_->points.size() <= 0)
    {
      print_error("%*s]\tCannot initialize query, point cloud with zero points given",20,__func__);
      return false;
    }
    Eigen::Vector3f s_orig ( query_cloud_->sensor_origin_(0),  query_cloud_->sensor_origin_(1),  query_cloud_->sensor_origin_(2) );
    Eigen::Quaternionf s_orie = query_cloud_->sensor_orientation_;
    if (s_orig.isZero(1e-5) && s_orie.isApprox(Eigen::Quaternionf::Identity(), 1e-5) )
    {
      if (params_["verbosity"]>1)
        print_info("%*s]\tInitializing query expressed in sensor reference frame.\n",20,__func__);
      this->local_query_ = false;
    }
    else
    {
      if (params_["verbosity"]>1)
        print_info("%*s]\tInitializing query expressed in local object reference frame.\n",20,__func__);
      print_warn("%*s]\tQuery in local reference frame is not supported yet, you will most likely get wrong results.\n",20,__func__);
      this->local_query_ = true;
    }
    //Check if a filter is needed
    if (params_["filtering"] >= 1)
      filtering_();
    //Check if upsampling is needed
    if (params_["upsampling"] >= 1)
      upsampling_();
    //Check if downsampling is needed
    if (params_["downsampling"] >= 1)
      downsampling_();
    vfh_.clear();
    esf_.clear();
    cvfh_.clear();
    ourcvfh_.clear();
    //Check if we need ESF descriptor
    if (params_["useESF"] >= 1)
      computeESF_();
    //Check if we need Normals
    if (params_["useVFH"] >=1 || params_["useCVFH"] >=1 || params_["useOURCVFH"] >=1)
    {
      if (computeNormals_())
      {
        //And consequently other descriptors
        if (params_["useVFH"] >=1)
          computeVFH_();
        if (params_["useCVFH"] >=1)
          computeCVFH_();
        if (params_["useOURCVFH"] >=1)
          computeOURCVFH_();
      }
      else
        return false;
    }
    if (params_["verbosity"]>1)
    {
      print_info("%*s]\tQuery succesfully set and initialized:\n",20,__func__);
      print_info("%*s]\t",20,__func__);
      print_value("%s", query_name_.c_str());
      print_info(" with ");
      print_value("%d", query_cloud_->points.size());
      print_info(" points\n");
      print_info("%*s]\tTotal time elapsed to initialize a query: ",20,__func__);
      print_value("%g", timer.getTime());
      print_info(" ms\n");
    }
    return true;
  }
  /////////////////////////////////////////////////////////////////////////////////////
  void PoseEstimation::setQuery(string str, PtC& cl)
  {
    query_set_ = false;
    query_name_ = str;
    copyPointCloud(cl, *query_cloud_);
    if (initQuery_())
      query_set_ = true;
  }
  /////////////////////////////////////////////////////////////////////////////////////
  void PoseEstimation::setQuery(string str, PtC::Ptr clp)
  {
    query_set_ = false;
    query_name_ = str;
    copyPointCloud(*clp, *query_cloud_);
    if (initQuery_())
      query_set_ = true;
  }
  /////////////////////////////////////////////////////////////////////////////////////////
  void PoseEstimation::printParams()
  {
    if (params_["verbosity"]>1)
      print_info("%*s]\tPrinting current list of parameters:\n",20,__func__);
    for (auto &x : params_)
      cout<< x.first.c_str() <<"="<< x.second<<endl;
  }
  /////////////////////////////////////////////////////////////////////////////////////////
  void PoseEstimation::setDatabase(PoseDB& database)
  {
    if (database.isEmpty())
    {
      print_warn("%*s]\tProvided database is empty or invalid, skipping new database setting...\n",20,__func__);
      return;
    }
    database_.clear();
    db_set_= false;
    StopWatch timer;
    timer.reset();
    database_ = database;
    db_set_ = true;
    if (params_["verbosity"]>1)
    {
      print_info("%*s]\tDatabase set in ",20,__func__);
      print_value("%g", timer.getTime());
      print_info(" ms\n");
      print_info("%*s]\tTotal number of poses found: ",20,__func__);
      print_value("%d\n",database_.names_.size());
    }
  }
  /////////////////////////////////////////////////////////////////////////////////////
  void PoseEstimation::setDatabase(boost::filesystem::path dbPath)
  {
    if (!exists(dbPath) || !is_directory(dbPath))
    {
      print_warn("%*s]\t%s is not a directory or does not exists, skipping new database setting...\n",20,__func__);
      return;
    }
    database_.clear();
    db_set_=false;
    StopWatch timer;
    timer.reset();
    if (database_.load(dbPath))
    {
      db_set_ = true;
      if (params_["verbosity"]>1)
      {
        print_info("%*s]\tDatabase loaded and set from location %s in ",20,__func__,dbPath.string().c_str());
        print_value("%g", timer.getTime());
        print_info(" ms\n");
        print_info("%*s]\tTotal number of poses found: ",20,__func__);
        print_value("%d\n",database_.names_.size());
      }
    }
  }
  bool PoseEstimation::findCandidate_(vector<Candidate>& list, string name, float& dist)
  {
    if (list.empty())
    {
      return false;
    }
    for (vector<Candidate>::iterator it=list.begin(); it!=list.end(); ++it)
    {
      if (it->name_.compare(name)==0)
      {
        dist = it->normalized_distance_;
        list.erase(it);
        return true;
      }
    }
    return false;
  }
  ////////////////////////////////////////////////////////////////////////////////////////
  bool PoseEstimation::generateLists()
  {
    int k = params_["kNeighbors"];
    if (!db_set_)
    {
      print_error("%*s]\tDatabase is not set, set it with setDatabase first! Exiting...\n",20,__func__);
      return false;
    }
    if (!query_set_)
    {
      print_error("%*s]\tQuery is not set, set it with setQuery first! Exiting...\n",20,__func__);
      return false;
    }
    if (k > database_.names_.size())
    {
      print_error("%*]\tNot enough candidates to select in database, kNeighbors is bigger than database size, aborting...\n",20,__func__);
      return false;
    }
    if (params_["verbosity"]>1)
      print_info("%*s]\tStarting Candidate list(s) generation...\n",20,__func__);
    StopWatch timer,t;
    timer.reset();
    vfh_list_.clear();
    esf_list_.clear();
    cvfh_list_.clear();
    ourcvfh_list_.clear();
    composite_list_.clear();
    if (params_["useVFH"]>=1)
    {
      if (params_["verbosity"]>1)
        print_info("%*s]\tGenerating List of Candidates, based on VFH... ",20,__func__);
      t.reset();
      try
      {
        flann::Matrix<float> vfh_query (new float[1*308],1,308);
        for (size_t j=0; j < 308; ++j)
          vfh_query[0][j]= vfh_.points[0].histogram[j];
        flann::Matrix<int> match_id (new int[1*k],1,k);
        flann::Matrix<float> match_dist (new float[1*k],1,k);
        database_.vfh_idx_->knnSearch (vfh_query, match_id, match_dist, k, SearchParams(256));
        for (size_t i=0; i<k; ++i)
        {
          string name= database_.names_[match_id[0][i]];
          Candidate c(name,database_.clouds_[match_id[0][i]]);
          c.rank_=i+1;
          c.distance_=match_dist[0][i];
          c.normalized_distance_=(match_dist[0][i] - match_dist[0][0])/(match_dist[0][k-1] - match_dist[0][0]);
          vfh_list_.push_back(c);
        }
      }
      catch (...)
      {
        if (params_["verbosity"]>1)
          print_error("ERROR\n");
        print_error("%*s]\tError Computing VFH list\n",20,__func__);
        return false;
      }
      if (params_["verbosity"]>1)
      {
        print_value("%g",t.getTime());
        print_info(" ms elapsed\n");
      }
    }
    if (params_["useESF"]>=1)
    {
      if (params_["verbosity"]>1)
        print_info("%*s]\tGenerating List of Candidates, based on ESF... ",20,__func__);
      t.reset();
      try
      {
        flann::Matrix<float> esf_query (new float[1*640],1,640);
        for (size_t j=0; j < 640; ++j)
          esf_query[0][j]= esf_.points[0].histogram[j];
        flann::Matrix<int> match_id (new int[1*k],1,k);
        flann::Matrix<float> match_dist (new float[1*k],1,k);
        database_.esf_idx_->knnSearch (esf_query, match_id, match_dist, k, SearchParams(256) );
        for (size_t i=0; i<k; ++i)
        {
          string name= database_.names_[match_id[0][i]];
          Candidate c(name,database_.clouds_[match_id[0][i]]);
          c.rank_=i+1;
          c.distance_=match_dist[0][i];
          c.normalized_distance_=(match_dist[0][i] - match_dist[0][0])/(match_dist[0][k-1] - match_dist[0][0]);
          esf_list_.push_back(c);
        }
      }
      catch (...)
      {
        if (params_["verbosity"]>1)
          print_error("ERROR\n");
        print_error("%*s]\tError Computing ESF list\n",20,__func__);
        return false;
      }
      if (params_["verbosity"]>1)
      {
        print_value("%g",t.getTime());
        print_info(" ms elapsed\n");
      }
    }
    if (params_["useCVFH"]>=1)
    {
      if (params_["verbosity"]>1)
        print_info("%*s]\tGenerating List of Candidates, based on CVFH... ",20,__func__);
      t.reset();
      try
      {
        vector<pair<float, int> > dists;
        if (database_.computeDistFromClusters_(cvfh_.makeShared(), listType::cvfh, dists) )
        {
          sort(dists.begin(), dists.end(),
              [](pair<float, int> const& a, pair<float, int> const& b)
              {
              return (a.first < b.first );
              });
          dists.resize(k);
          for (int i=0; i<k ; ++i)
          {
            Candidate c (database_.names_[dists[i].second], database_.clouds_[dists[i].second] );
            c.rank_ = i+1;
            c.distance_ = dists[i].first;
            c.normalized_distance_ = (dists[i].first - dists[0].first)/(dists[k-1].first - dists[0].first);
            cvfh_list_.push_back(c);
          }
        }
        else
        {
          if (params_["verbosity"]>1)
            print_error("ERROR\n");
          print_error("%*s]\tError generating CVFH list, computeDistFromClusters returned false...\n",20,__func__);
          return false;
        }
      }
      catch (...)
      {
        if (params_["verbosity"]>1)
          print_error("ERROR\n");
        print_error("%*s]\tError computing CVFH list\n",20,__func__);
        return false;
      }
      if (params_["verbosity"]>1)
      {
        print_value("%g",t.getTime());
        print_info(" ms elapsed\n");
      }
    }
    if (params_["useOURCVFH"]>=1)
    {
      if (params_["verbosity"]>1)
        print_info("%*s]\tGenerating List of Candidates, based on OURCVFH... ",20,__func__);
      t.reset();
      try
      {
        vector<pair<float, int> > dists;
        if (database_.computeDistFromClusters_(ourcvfh_.makeShared(), listType::ourcvfh, dists) )
        {
          sort(dists.begin(), dists.end(),
              [](pair<float, int> const& a, pair<float, int> const& b)
              {
              return (a.first < b.first );
              });
          dists.resize(k);
          for (int i=0; i<k ; ++i)
          {
            Candidate c (database_.names_[dists[i].second], database_.clouds_[dists[i].second] );
            c.rank_ = i+1;
            c.distance_ = dists[i].first;
            c.normalized_distance_ = (dists[i].first - dists[0].first)/(dists[k-1].first - dists[0].first);
            ourcvfh_list_.push_back(c);
          }
        }
        else
        {
          if (params_["verbosity"]>1)
            print_error("ERROR\n");
          print_error("%*s]\tError generating OURCVFH list, computeDistFromClusters returned false...\n",20,__func__);
          return false;
        }
      }
      catch (...)
      {
        if (params_["verbosity"]>1)
          print_error("ERROR\n");
        print_error("%*s]\tError computing OURCVFH list\n",20,__func__);
        return false;
      }
      if (params_["verbosity"]>1)
      {
        print_value("%g",t.getTime());
        print_info(" ms elapsed\n");
      }
    }
    //Composite list generation
    if (params_["verbosity"]>1)
      print_info("%*s]\tGenerating Composite List based on previous features... ",20,__func__);
    t.reset();
    vector<Candidate> tmp_esf, tmp_cvfh, tmp_ourcvfh, tmp_vfh;
    if(params_["useVFH"]>0)
    {
      if (feature_count_ == 1)
      {
        boost::copy(vfh_list_, back_inserter(composite_list_) );
        candidates_found_ = true;
        if (params_["verbosity"]>1)
        {
          print_value("%g",t.getTime());
          print_info(" ms elapsed\n");
        }
        return true;
      }
      boost::copy(vfh_list_, back_inserter(tmp_vfh) );
    }
    if(params_["useESF"]>0)
    {
      if (feature_count_ == 1)
      {
        boost::copy(esf_list_, back_inserter(composite_list_) );
        candidates_found_ = true;
        if (params_["verbosity"]>1)
        {
          print_value("%g",t.getTime());
          print_info(" ms elapsed\n");
        }
        return true;
      }
      boost::copy(esf_list_, back_inserter(tmp_esf) );
    }
    if(params_["useCVFH"]>0)
    {
      if (feature_count_ == 1)
      {
        boost::copy(cvfh_list_, back_inserter(composite_list_) );
        candidates_found_ = true;
        if (params_["verbosity"]>1)
        {
          print_value("%g",t.getTime());
          print_info(" ms elapsed\n");
        }
        return true;
      }
      boost::copy(cvfh_list_, back_inserter(tmp_cvfh) );
    }
    if(params_["useOURCVFH"]>0)
    {
      if (feature_count_ == 1)
      {
        boost::copy(ourcvfh_list_, back_inserter(composite_list_) );
        candidates_found_ = true;
        if (params_["verbosity"]>1)
        {
          print_value("%g",t.getTime());
          print_info(" ms elapsed\n");
        }
        return true;
      }
      boost::copy(ourcvfh_list_, back_inserter(tmp_ourcvfh) );
    }
    if (params_["useVFH"]>0 && !tmp_vfh.empty())
    {
      for (vector<Candidate>::iterator it=tmp_vfh.begin(); it!=tmp_vfh.end(); ++it)
      {
        float d_tmp;
        if (params_["useESF"]>0)
        {
          if (findCandidate_(tmp_esf, it->name_, d_tmp))
            it->normalized_distance_+= d_tmp;
          else
            it->normalized_distance_ += 1;
        }
        if (params_["useCVFH"]>0)
        {
          if (findCandidate_(tmp_cvfh, it->name_, d_tmp))
            it->normalized_distance_+= d_tmp;
          else
            it->normalized_distance_+=1;
        }
        if (params_["useOURCVFH"]>0)
        {
          if (findCandidate_(tmp_ourcvfh, it->name_, d_tmp))
            it->normalized_distance_+= d_tmp;
          else
            it->normalized_distance_+=1;
        }
        it->normalized_distance_ /= feature_count_;
        composite_list_.push_back(*it);
      }
    }
    //Still some candidates in ESF that were not in VFH
    if (params_["useESF"]>0 && !tmp_esf.empty())
    {
      for (vector<Candidate>::iterator it=tmp_esf.begin(); it!=tmp_esf.end(); ++it)
      {
        float d_tmp;
        if (params_["useCVFH"]>0)
        {
          if (findCandidate_(tmp_cvfh, it->name_, d_tmp))
            it->normalized_distance_+= d_tmp;
          else
            it->normalized_distance_+=1;
        }
        if (params_["useOURCVFH"]>0)
        {
          if (findCandidate_(tmp_ourcvfh, it->name_, d_tmp))
            it->normalized_distance_+= d_tmp;
          else
            it->normalized_distance_+=1;
        }
        if (params_["useVFH"]>0)
          it->normalized_distance_ += 1;
        it->normalized_distance_ /= feature_count_;
        composite_list_.push_back(*it);
      }
    }
    //Still some candidates in CVFH that were not in VFH and ESF
    if (params_["useCVFH"]>0 && !tmp_cvfh.empty())
    {
      for (vector<Candidate>::iterator it=tmp_cvfh.begin(); it!=tmp_cvfh.end(); ++it)
      {
        float d_tmp;
        if (params_["useOURCVFH"]>0)
        {
          if (findCandidate_(tmp_ourcvfh, it->name_, d_tmp))
            it->normalized_distance_+= d_tmp;
          else
            it->normalized_distance_+=1;
        }
        if (params_["useVFH"]>0)
          it->normalized_distance_ += 1;
        if (params_["useESF"]>0)
          it->normalized_distance_ += 1;
        it->normalized_distance_ /= feature_count_;
        composite_list_.push_back(*it);
      }
    }
    //Still some candidates in OURCVFH that were not in other lists
    if (params_["useOURCVFH"]>0 && !tmp_ourcvfh.empty())
    {
      for (vector<Candidate>::iterator it=tmp_ourcvfh.begin(); it!=tmp_ourcvfh.end(); ++it)
      {
        if (params_["useVFH"]>0)
          it->normalized_distance_ += 1;
        if (params_["useESF"]>0)
          it->normalized_distance_ += 1;
        if (params_["useCVFH"]>0)
          it->normalized_distance_ += 1;
        it->normalized_distance_ /= feature_count_;
        composite_list_.push_back(*it);
      }
    }
    sort(composite_list_.begin(), composite_list_.end(),
        [](Candidate const & a, Candidate const & b)
        {
        return (a.normalized_distance_ < b.normalized_distance_ );
        });
    composite_list_.resize(k);
    for (vector<Candidate>::iterator it=composite_list_.begin(); it!=composite_list_.end(); ++it)
      it->rank_ = it - composite_list_.begin() +1; //write the rank of the candidate in the list
    candidates_found_ = true;
    if (params_["verbosity"]>1)
    {
      print_value("%g",t.getTime());
      print_info(" ms elapsed\n");
      print_info("%*s]\tTotal time elapsed to generate list(s) of candidates: ",20,__func__);
      print_value("%g",timer.getTime());
      print_info(" ms\n");
    }
    return true;
  }
  //////////////////////////////////////////////////////////////////////////////////////////
  bool PoseEstimation::generateLists(boost::filesystem::path dbPath)
  {
    if (!query_set_)
    {
      print_error("%*s]\tQuery is not set, set it with setQuery first! Exiting...\n",20,__func__);
      return false;
    }
    if (db_set_ && params_["verbosity"]>0)
    {
      print_warn("%*s]\tDatabase was already set, but a new path was specified, loading the new database...\n",20,__func__);
      print_warn("%*s\tUse generateLists() without arguments if you want to keep using the previously loaded database\n",20,__func__);
    }
    setDatabase(dbPath);
    return (this->generateLists());
  }
  bool PoseEstimation::generateLists(PoseDB& db)
  {
    if (!query_set_)
    {
      print_error("%*s]\tQuery is not set, set it with setQuery first! Exiting...\n",20,__func__);
      return false;
    }
    if (db_set_ && params_["verbosity"]>0)
    {
      print_warn("%*s]\tDatabase was already set, but a new one was specified, loading the new database...\n",20,__func__);
      print_warn("%*s\tUse generateLists() without arguments if you want to keep using the previously loaded database\n",20,__func__);
    }
    setDatabase(db);
    return (this->generateLists());
  }
  ///////////////////////////////////////////////
  void PoseEstimation::printCandidates()
  {
    if (!candidates_found_)
    {
      if (params_["verbosity"]>0)
        print_warn("%*s]\tList of Candidates are not yet generated, call generateLists before trying to print them!\n",20,__func__);
      return;
    }
    if (params_["verbosity"]>1)
      print_info("%*s]\tPrinting current list of candidates:\n",20,__func__);
    print_info("Query name is: %s\n",query_name_.c_str());
    print_info("%-6s","Rank");
    if (params_["useVFH"] >0)
      print_info("%-30s","VFH");
    if (params_["useESF"] >0)
      print_info("%-30s","ESF");
    if (params_["useCVFH"] >0)
      print_info("%-30s","CVFH");
    if (params_["useOURCVFH"] >0)
      print_info("%-30s","OURCVFH");
    print_info("\n");
    for ( int i=0; i< params_["kNeighbors"]; ++i)
    {
      print_value("%-6d", i+1);
      if (params_["useVFH"] >0)
      {
        print_info("%-15s D:",vfh_list_[i].name_.c_str());
        print_value("%-9g   ",vfh_list_[i].normalized_distance_);
      }
      if (params_["useESF"] >0)
      {
        print_info("%-15s D:",esf_list_[i].name_.c_str());
        print_value("%-9g   ",esf_list_[i].normalized_distance_);
      }
      if (params_["useCVFH"] >0)
      {
        print_info("%-15s D:",cvfh_list_[i].name_.c_str());
        print_value("%-9g   ",cvfh_list_[i].normalized_distance_);
      }
      if (params_["useOURCVFH"] >0)
      {
        print_info("%-15s D:",ourcvfh_list_[i].name_.c_str());
        print_value("%-9g   ",ourcvfh_list_[i].normalized_distance_);
      }
      print_info("\n");
    }
    cout<<endl;
    print_info("%-6s", "Rank");
    print_info("%-30s\n","Composite");
    for (int i=0; i<params_["kNeighbors"]; ++i)
    {
      print_value("%-6d", (int)composite_list_[i].rank_);
      print_info("%-15s D:", composite_list_[i].name_.c_str());
      print_value("%-9g   ", composite_list_[i].normalized_distance_);
      cout<<endl;
    }
  }

  bool PoseEstimation::refineCandidates()
  {
    if (!candidates_found_)
    {
      print_error("%*s]\tList of Candidates are not yet generated, call generateLists first...\n",20,__func__);
      return false;
    }
    if (local_query_)
      print_error("%*s]\tQuery in local reference system in not implemented yet, set another query or transform it, exiting...\n",20,__func__); //TODO implement this
    CentroidPoint<Pt> qct;
    for (int i=0; i<query_cloud_->points.size(); ++i)
      qct.add(query_cloud_->points[i]);
    Pt query_centroid;
    qct.get(query_centroid);
    if (params_["progBisection"]>0)
    {
      //ProgressiveBisection
      if (params_["verbosity"]>1)
        print_info("%*s]\tStarting Progressive Bisection...\n",20,__func__);
      StopWatch timer;
      timer.reset();
      vector<Candidate> list; //make a temporary list to manipulate
      boost::copy(composite_list_, back_inserter(list));
      IterativeClosestPoint<Pt, Pt, float> icp;
      icp.setInputTarget(query_cloud_); //query
      icp.setUseReciprocalCorrespondences(params_["icpReciprocal"]);
      icp.setMaximumIterations (params_["progItera"]); //max iterations to perform
      icp.setTransformationEpsilon (1e-9); //difference between consecutive transformations
      icp.setEuclideanFitnessEpsilon (1e-9); //not using it (sum of euclidean distances between points)
      int steps (0);
      while (list.size() > 1)
      {
        for (vector<Candidate>::iterator it=list.begin(); it!=list.end(); ++it)
        {
          PtC::Ptr aligned (new PtC);
          PtC::Ptr candidate (new PtC);
          copyPointCloud(*(it->cloud_), *candidate);
          candidate->sensor_origin_.setZero();
          candidate->sensor_orientation_.setIdentity();
          //icp align source over target, result in aligned
          icp.setInputSource(candidate); //the candidate
          Eigen::Matrix4f guess;
          if (steps >0)
            guess = it->transformation_;
          else
          {
            Eigen::Matrix4f T_kli, T_cen;
            CentroidPoint<Pt> cct;
            Pt candidate_centroid;
            if (this->database_.isLocal())
            { //database is in local frame
              Eigen::Matrix3f R (it->cloud_->sensor_orientation_);
              Eigen::Vector4f t;
              t = it->cloud_->sensor_origin_;
              T_kli << R(0,0), R(0,1), R(0,2), t(0),
                    R(1,0), R(1,1), R(1,2), t(1),
                    R(2,0), R(2,1), R(2,2), t(2),
                    0,      0,      0,      1;
              PtC::Ptr cloud_in_k (new PtC);
              transformPointCloud(*candidate, *cloud_in_k, T_kli);
              for (int i=0; i< cloud_in_k->points.size(); ++i)
                cct.add(cloud_in_k->points[i]);
              cct.get(candidate_centroid);
              T_cen << 1,0,0, (query_centroid.x - candidate_centroid.x),
                    0,1,0, (query_centroid.y - candidate_centroid.y),
                    0,0,1, (query_centroid.z - candidate_centroid.z),
                    0,0,0, 1;
              guess = T_cen*T_kli;
            }
            else
            { //database is in sensor frame, just get centroid
              for (int i=0; i< it->cloud_->points.size(); ++i)
                cct.add(it->cloud_->points[i]);
              cct.get(candidate_centroid);
              T_cen << 1,0,0, (query_centroid.x - candidate_centroid.x),
                    0,1,0, (query_centroid.y - candidate_centroid.y),
                    0,0,1, (query_centroid.z - candidate_centroid.z),
                    0,0,0, 1;
              guess = T_cen;
            }
          }
          icp.align(*aligned, guess); //initial gross estimation
          it->transformation_ = icp.getFinalTransformation();
          it->rmse_ = sqrt(icp.getFitnessScore());
          if (params_["verbosity"]>1)
          {
            print_info("%*s]\tCandidate: ",20,__func__);
            print_value("%-15s",it->name_.c_str());
            print_info(" just performed %d ICP iterations, its RMSE is: ", (int)params_["progItera"]);
            print_value("%g\n",it->rmse_);
          }
        }
        ++steps;
        //now resort list
        sort(list.begin(), list.end(),
            [](Candidate const &a, Candidate const &b)
            {
            return (a.rmse_ < b.rmse_ );
            });
        //check if candidate falled under rmse threshold, no need to check them all since list is now sorted with min rmse on top
        if (list[0].rmse_ < params_["rmseThreshold"] )
        {
          //convergence
          pose_estimation_.reset();
          pose_estimation_ = boost::make_shared<Candidate>(list[0]);
          refinement_done_=true;
          if (params_["verbosity"]>1)
          {
            print_info("%*s]\tCandidate %s converged with RMSE %g\n",20,__func__,pose_estimation_->name_.c_str(), pose_estimation_->rmse_);
            print_info("%*s]\tFinal transformation is:\n",20,__func__);
            cout<<pose_estimation_->transformation_<<endl;
            print_info("%*s]\tTotal time elapsed in Progressive Bisection: ",20,__func__);
            print_value("%g",timer.getTime());
            print_info(" ms\n");
          }
          return true;
        }
        else
        {
          //no convergence, resize list
          int size = list.size();
          size *= params_["progFraction"];
          list.resize(size);
          if (params_["verbosity"]>1)
            print_info("%*s]\tResizing... Keeping %d%% of previous list\n",20,__func__,(int)(params_["progFraction"]*100));
        }
      }
      //only one candidate remained, he wins!
      pose_estimation_.reset();
      pose_estimation_ = boost::make_shared<Candidate>(list[0]);
      refinement_done_=true;
      if (params_["verbosity"]>1)
      {
        print_info("%*s]\tCandidate %s survived progressive bisection with RMSE %g\n",20,__func__,pose_estimation_->name_.c_str(), pose_estimation_->rmse_);
        print_info("%*s]\tFinal transformation is:\n",20,__func__);
        cout<<pose_estimation_->transformation_<<endl;
        print_info("%*s]\tTotal time elapsed in Progressive Bisection: ",20,__func__);
        print_value("%g",timer.getTime());
        print_info(" ms\n");
      }
      return true;
    }
    else
    {
      //BruteForce
      if (params_["verbosity"]>1)
        print_info("%*s]\tStarting Brute Force...\n",20,__func__);
      StopWatch timer;
      timer.reset();
      IterativeClosestPoint<Pt, Pt, float> icp;
      icp.setInputTarget(query_cloud_); //query
      icp.setUseReciprocalCorrespondences(params_["icpReciprocal"]);
      icp.setMaximumIterations (params_["maxIterations"]); //max iterations to perform
      icp.setTransformationEpsilon (1e-9); //difference between consecutive transformations
      icp.setEuclideanFitnessEpsilon (pow(params_["rmseThreshold"],2));
      for (vector<Candidate>::iterator it=composite_list_.begin(); it!=composite_list_.end(); ++it)
      {
        PtC::Ptr aligned (new PtC);
        PtC::Ptr candidate (new PtC);
        copyPointCloud(*(it->cloud_), *candidate);
        //icp align source over target, result in aligned
        candidate->sensor_origin_.setZero();
        candidate->sensor_orientation_.setIdentity();
        icp.setInputSource(candidate); //the candidate
        Eigen::Matrix4f T_kli, T_cen, guess;
        CentroidPoint<Pt> cct;
        Pt candidate_centroid;
        if (this->database_.isLocal())
        { //database is in local frame
          Eigen::Matrix3f R( it->cloud_->sensor_orientation_ );
          Eigen::Vector4f t;
          t = it->cloud_->sensor_origin_;
          T_kli << R(0,0), R(0,1), R(0,2), t(0),
                R(1,0), R(1,1), R(1,2), t(1),
                R(2,0), R(2,1), R(2,2), t(2),
                0,      0,      0,      1;
          PtC::Ptr cloud_in_k (new PtC);
          transformPointCloud(*(it->cloud_), *cloud_in_k, T_kli);
          for (int i=0; i< cloud_in_k->points.size(); ++i)
            cct.add(cloud_in_k->points[i]);
          cct.get(candidate_centroid);
          T_cen << 1,0,0, (query_centroid.x - candidate_centroid.x),
                0,1,0, (query_centroid.y - candidate_centroid.y),
                0,0,1, (query_centroid.z - candidate_centroid.z),
                0,0,0, 1;
          guess = T_cen*T_kli;
        }
        else
        { //database is in sensor frame, just get centroid
          for (int i=0; i< it->cloud_->points.size(); ++i)
            cct.add(it->cloud_->points[i]);
          cct.get(candidate_centroid);
          T_cen << 1,0,0, (query_centroid.x - candidate_centroid.x),
                0,1,0, (query_centroid.y - candidate_centroid.y),
                0,0,1, (query_centroid.z - candidate_centroid.z),
                0,0,0, 1;
          guess = T_cen;
        }
        icp.align(*aligned, guess);
        it->transformation_ = icp.getFinalTransformation();
        it->rmse_ = sqrt(icp.getFitnessScore());
        if (params_["verbosity"]>1)
        {
          print_info("%*s]\tCandidate: ",20,__func__);
          print_value("%-15s",it->name_.c_str());
          print_info(" just performed ICP alignment, its RMSE is: ");
          print_value("%g\n",it->rmse_);
        }
        if (it->rmse_ < params_["rmseThreshold"])
        {
          //convergence
          pose_estimation_.reset();
          pose_estimation_ = boost::make_shared<Candidate>(*it);
          refinement_done_=true;
          if (params_["verbosity"]>1)
          {
            print_info("%*s]\tCandidate %s converged with RMSE %g\n",20,__func__,pose_estimation_->name_.c_str(), pose_estimation_->rmse_);
            print_info("%*s]\tFinal transformation is:\n",20,__func__);
            cout<<pose_estimation_->transformation_<<endl;
            print_info("%*s]\tTotal time elapsed in Brute Force: ",20,__func__);
            print_value("%g",timer.getTime());
            print_info(" ms\n");
          }
          return true;
        }
      }
      //no candidate converged, pose estimation failed
      pose_estimation_.reset();
      refinement_done_=false;
      if (params_["verbosity"]>0)
        print_warn("%*s]\tCannot find a suitable candidate, try raising the rmse threshold\n",20,__func__);
      if (params_["verbosity"]>1)
      {
        print_info("%*s]\tTotal time elapsed in Brute Force: ",20,__func__);
        print_value("%g",timer.getTime());
        print_info(" ms\n");
      }
      return false;
    }
  }

  bool PoseEstimation::estimate(string name, PtC& cloud, boost::filesystem::path db_path)
  {
    query_set_=candidates_found_=db_set_=refinement_done_ =false;
    setQuery(name, cloud);
    if (generateLists(db_path))
      return (refineCandidates());
    else
      return false;
  }
  bool PoseEstimation::estimate(string name, PtC::Ptr cloud_pointer, boost::filesystem::path db_path)
  {
    query_set_=candidates_found_=db_set_=refinement_done_ =false;
    setQuery(name, cloud_pointer);
    if (generateLists(db_path))
      return (refineCandidates());
    else
      return false;
  }
  bool PoseEstimation::estimate(string name, PtC& cloud, PoseDB& database)
  {
    query_set_=candidates_found_=db_set_=refinement_done_ =false;
    setQuery(name, cloud);
    if ( generateLists(database) )
      return ( refineCandidates() );
    else
      return false;
  }
  bool PoseEstimation::estimate(string name, PtC::Ptr cloud_pointer, PoseDB& database)
  {
    query_set_=candidates_found_=db_set_=refinement_done_ =false;
    setQuery(name, cloud_pointer);
    if ( generateLists(database) )
      return ( refineCandidates() );
    else
      return false;
  }
  void PoseEstimation::printEstimation()
  {
    if (!refinement_done_)
    {
      if (params_["verbosity"]>0)
        print_warn("%*s]\tRefinement is not done yet, or wasn't successful... Printing nothing...\n",20,__func__);
      return;
    }
    if (params_["verbosity"]>1)
      print_info("%*s]\tPrinting current pose estimation information:\n",20,__func__);
    print_info("Query object ");
    print_value("%-15s",query_name_.c_str());
    print_info(" was identified with candidate ");
    print_value("%-15s\n",pose_estimation_->name_.c_str());
    print_info("It was positioned on Rank ");
    print_value("%-3d",pose_estimation_->rank_);
    print_info(" of composite list, with normalized distance of ");
    print_value("%g\n",pose_estimation_->normalized_distance_);
    print_info("Its final RMSE is ");
    print_value("%g\n",pose_estimation_->rmse_);
    print_info("Final Transformation (in candidate reference system):\n");
    cout<<pose_estimation_->transformation_<<endl;
  }
  boost::shared_ptr<parameters> PoseEstimation::getParams()
  {
    parameters p = {{"key",-1}}; //initialize p with something so that copy assigned is performed instead of move (to preserve param_)
    p = params_;
    boost::shared_ptr<parameters> ptr;
    ptr = boost::make_shared<parameters>(p);
    return ptr;
  }
  bool PoseEstimation::getParams(boost::shared_ptr<parameters> params)
  {
    try
    {
      parameters p = {{"key",-1}}; //initialize p with something so that copy assigned is performed instead of move (to preserve param_)
      p = params_;
      boost::shared_ptr<parameters> ptr;
      params = boost::make_shared<parameters>(p);
    }
    catch (...)
    {//something went wrong
      print_error("%*s]\tError copying parameters...\n",20,__func__);
      return false;
    }
    return true;
  }
  bool PoseEstimation::saveEstimation(path file, bool append)
  {
    if (!refinement_done_)
    {
      if (params_["verbosity"]>0)
        print_warn("%*s]\tRefinement is not done yet, or wasn't successful... Saving nothing...\n",20,__func__);
      return false;
    }
    timestamp t(TIME_NOW);
    if (exists(file) && is_directory(file)) //existant directory, save to <query_name>.estimation inside it
    {
      try
      {
        fstream f;
        if (append)
          f.open ( (file.string() + query_name_ + ".estimation").c_str(), fstream::out | fstream::app );
        else
          f.open ( (file.string() + query_name_ + ".estimation").c_str(), fstream::trunc );
        f << "Pose estimation saved on " << (to_simple_string(t)).c_str() <<endl;
        f << "Query object: " << query_name_.c_str() << " is identified with candidate " << pose_estimation_->name_.c_str() <<endl;
        f << "Candidate was on rank " << pose_estimation_->rank_ << " of composite list with normalized distance of " << pose_estimation_->normalized_distance_ <<endl;
        f << "Final RMSE is " << pose_estimation_->rmse_ << endl;
        f << "Final transformation, in candidate reference system: "<<endl;
        f << pose_estimation_ -> transformation_ <<endl<<endl;
        f.close();
      }
      catch (...)
      {
        print_error ("%*s]\tError writing to disk, check if %s is valid path\n",20,__func__,(file.string() + query_name_ + ".estimation").c_str());
        return false;
      }
      if (params_["verbosity"]>1)
        print_info("%*s]\tSaved current pose estimation information in %s\n",20,__func__,(file.string() + query_name_ + ".estimation").c_str());
      return true;
    }
    else if (exists(file) && is_regular_file(file)) //existant file, append or overwrite
    {
      try
      {
        fstream f;
        if (append)
          f.open ( file.string().c_str(), fstream::out | fstream::app );
        else
          f.open ( file.string().c_str(), fstream::trunc );
        f << "Pose estimation saved on " << (to_simple_string(t)).c_str() <<endl;
        f << "Query object: " << query_name_.c_str() << " is identified with candidate " << pose_estimation_->name_.c_str() <<endl;
        f << "Candidate was on rank " << pose_estimation_->rank_ << " of composite list with normalized distance of " << pose_estimation_->normalized_distance_ <<endl;
        f << "Final RMSE is " << pose_estimation_->rmse_ << endl;
        f << "Final transformation, in candidate reference system: "<<endl;
        f << pose_estimation_ -> transformation_ <<endl<<endl;
        f.close();
      }
      catch (...)
      {
        print_error ("%*s]\tError writing to disk, check if %s is valid path\n",20,__func__,file.string().c_str());
        return false;
      }
      if (params_["verbosity"]>1)
        print_info("%*s]\tSaved current pose estimation information in %s\n",20,__func__,file.string().c_str());
      return true;
    }
    else if (file.filename().has_extension() && !exists(file) ) //non existant file, creating it
    {
      if (file.has_parent_path() && !exists(file.parent_path()) ) //also container directory does not exists
      {
        try
        {
          create_directory(file.parent_path());
        }
        catch (...)
        {
          print_error("%*s]\tError creating directory %s\n",20,__func__,file.parent_path().string().c_str());
          return  false;
        }
      }
      try
      {
        ofstream f;
        f.open ( file.string().c_str() );
        f << "Pose estimation saved on " << (to_simple_string(t)).c_str() <<endl;
        f << "Query object: " << query_name_.c_str() << " is identified with candidate " << pose_estimation_->name_.c_str() <<endl;
        f << "Candidate was on rank " << pose_estimation_->rank_ << " of composite list with normalized distance of " << pose_estimation_->normalized_distance_ <<endl;
        f << "Final RMSE is " << pose_estimation_->rmse_ << endl;
        f << "Final transformation, in candidate reference system: "<<endl;
        f << pose_estimation_ -> transformation_ <<endl<<endl;
        f.close();
      }
      catch (...)
      {
        print_error ("%*s]\tError writing to disk, check if %s is valid path\n",20,__func__,file.string().c_str());
        return false;
      }
      if (params_["verbosity"]>1)
        print_info("%*s]\tSaved current pose estimation information in %s\n",20,__func__,file.string().c_str());
      return true;
    }
    else if (!file.filename().has_extension() && !exists(file) ) //non existant directory, create then save inside it
    {
      try
      {
        create_directory(file);
        fstream f;
        if (append)
          f.open ( (file.string() + query_name_ + ".estimation").c_str(), fstream::out | fstream::app );
        else
          f.open ( (file.string() + query_name_ + ".estimation").c_str(), fstream::trunc );
        f << "Pose estimation saved on " << (to_simple_string(t)).c_str() <<endl;
        f << "Query object: " << query_name_.c_str() << " is identified with candidate " << pose_estimation_->name_.c_str() <<endl;
        f << "Candidate was on rank " << pose_estimation_->rank_ << " of composite list with normalized distance of " << pose_estimation_->normalized_distance_ <<endl;
        f << "Final RMSE is " << pose_estimation_->rmse_ << endl;
        f << "Final transformation, in candidate reference system: "<<endl;
        f << pose_estimation_ -> transformation_ <<endl<<endl;
        f.close();
      }
      catch (...)
      {
        print_error("%*s]\tError writing to disk, check if %s is valid path\n",20,__func__,(file.string() + query_name_ + ".estimation" ).c_str() );
        return  false;
      }
      if (params_["verbosity"]>1)
        print_info("%*s]\tSaved current pose estimation information in %s\n",20,__func__,(file.string() + query_name_ + ".estimation").c_str());
      return true;
    }
    else //unexpeted path error
    {
      print_error("%*s]\tError parsing path: %s\n",20,__func__,file.string().c_str() );
      return  false;
    }
  }
  bool PoseEstimation::saveParams(boost::filesystem::path file)
  {
    if (exists(file) && file.has_extension() )
    {
      print_error("%*s]\t%s already exists, not saving there, aborting...\n",20,__func__,file.string().c_str());
      return false;
    }
    else if (!file.has_extension())
    {
      if (params_["verbosity"] > 1)
        print_info("%*s]\tAppending .conf to %s\n",20,__func__,file.string().c_str());
      file = (file.string() + ".conf"); //append extension
      if (exists(file))
      {
        print_error("%*s]\t%s already exists, not saving there, aborting...\n",20,__func__,file.string().c_str());
        return false;
      }
    }
    if (!(file.extension().string().compare(".conf") == 0) && params_["verbosity"]>0 )
      print_warn("%*s]\t%s has extension, but it is not .conf, it will not be a valid configuration file...\n",20,__func__,file.string().c_str());
    if (params_["verbosity"]>1)
      print_info ("%*s]\tWriting into %s ...\n",20,__func__,file.string().c_str());
    ofstream conf;
    conf.open(file.string().c_str());
    if (conf.is_open())
    {
      //write timestamp in file
      timestamp t(TIME_NOW);
      conf<<"% File written on "<< (to_simple_string(t)).c_str()<< " by PoseEstimation::saveParams"<<endl;
      for (auto &x : params_)
        conf<< x.first.c_str() <<"="<< x.second<<endl;
      conf.close();
      return true;
    }
    else
    {
      print_error("%*s]\tError opening %s for writing, aborting...\n",20,__func__,file.string().c_str());
      return false;
    }
  }

  boost::shared_ptr<Candidate> PoseEstimation::getEstimation()
  {
    if (! refinement_done_ )
    {
      if (params_["verbosity"]>0)
        print_warn("%*]\tRefinement has not yet been done, Pose Estimation is not complete, returning empty Candidate\n",20,__func__);
      Candidate c;
      return (boost::make_shared<Candidate> (c) );
    }
    Candidate c;
    c = *pose_estimation_;
    return ( boost::make_shared<Candidate> (c) );
  }
  bool PoseEstimation::getEstimation(boost::shared_ptr<Candidate> est)
  {
    if (! refinement_done_ )
    {
      if (params_["verbosity"]>0)
        print_warn("%*]\tRefinement has not yet been done, Pose Estimation is not complete, returning false\n",20,__func__);
      return false;
    }
    if (!est)
    {
      Candidate c;
      c = *pose_estimation_;
      est = boost::make_shared<Candidate> (c);
    }
    else
    {
      *est = *pose_estimation_;
    }
    return true;
  }

  bool PoseEstimation::getEstimationTransformation(Eigen::Matrix4f& t)
  {
    if (! refinement_done_ )
    {
      if (params_["verbosity"]>0)
        print_warn("%*]\tRefinement has not yet been done, Pose Estimation is not complete, returning false\n",20,__func__);
      return false;
    }
    t = pose_estimation_->transformation_;
    return true;
  }

  bool PoseEstimation::saveCandidates(boost::filesystem::path file)
  {
    if (!candidates_found_)
    {
      if (params_["verbosity"]>0)
        print_warn("%*s]\tList of Candidates are not yet generated, call generateLists before trying to save them!\n",20,__func__);
      return false;
    }
    if (exists(file) && params_["verbosity"]>1)
      print_info("%*s]\tAppending lists of candidates to %s\n",20,__func__,file.string().c_str());
    if (!exists(file) && params_["verbosity"]>1)
      print_info("%*s]\tSaving lists of candidates on %s\n",20,__func__,file.string().c_str());
    timestamp t(TIME_NOW);
    fstream f;
    f.open(file.string().c_str(), fstream::out | fstream::app);
    if (f.is_open())
    {
      f << "List of Candidates generated on "<<to_simple_string(t).c_str()<<" from query object named: "<<query_name_.c_str()<<endl;
      if (params_["useVFH"]>0)
      {
        f << "VFH:"<<endl;
        for (auto c : vfh_list_)
          f << c.name_.c_str() << "\tD: " << c.normalized_distance_<<endl;
      }
      if (params_["useESF"]>0)
      {
        f << "ESF:"<<endl;
        for (auto c : esf_list_)
          f << c.name_.c_str() << "\tD: " << c.normalized_distance_<<endl;
      }
      if (params_["useCVFH"]>0)
      {
        f << "CVFH:"<<endl;
        for (auto c : cvfh_list_)
          f << c.name_.c_str() << "\tD: " << c.normalized_distance_<<endl;
      }
      if (params_["useOURCVFH"]>0)
      {
        f << "OURCVFH:"<<endl;
        for (auto c : ourcvfh_list_)
          f << c.name_.c_str() << "\tD: " << c.normalized_distance_<<endl;
      }
      f << "Composite:"<<endl;
      for (auto c : vfh_list_)
        f << c.name_.c_str() << "\tD: " << c.normalized_distance_<<endl;
      f<<"End of Candidate Lists, saved by PoseEstimation::saveCandidates"<<endl<<endl;
      f.close();
    }
    else
    {
      print_error("%*s]\tError writing to %s...\n",20,__func__,file.string().c_str());
      return false;
    }
    return true;
  }

  bool PoseEstimation::getCandidateList (vector<Candidate>& list, listType type)
  {
    if ( !candidates_found_ )
    {
      if (params_["verbosity"]>0)
        print_warn("%*s]\tLists of Candidates are not generated yet, not copying anything...\n",20,__func__);
      return false;
    }
    if (type == listType::vfh)
    {
      if (params_["useVFH"]>0)
      {
        list.clear();
        boost::copy(vfh_list_, back_inserter(list) );
        return true;
      }
      else if (params_["verbosity"]>0)
        print_warn("%*s]\tVFH list was not generated, because 'useVFH' parameter was set to zero, not copying anything...\n",20,__func__);
      return false;
    }
    else if (type == listType::esf)
    {
      if (params_["useESF"]>0)
      {
        list.clear();
        boost::copy(esf_list_, back_inserter(list) );
        return true;
      }
      else if (params_["verbosity"]>0)
        print_warn("%*s]\tESF list was not generated, because 'useESF' parameter was set to zero, not copying anything...\n",20,__func__);
      return false;
    }
    else if (type == listType::cvfh)
    {
      if (params_["useCVFH"]>0)
      {
        list.clear();
        boost::copy(cvfh_list_, back_inserter(list) );
        return true;
      }
      else if (params_["verbosity"]>0)
        print_warn("%*s]\tCVFH list was not generated, because 'useCVFH' parameter was set to zero, not copying anything...\n",20,__func__);
      return false;
    }
    else if (type == listType::ourcvfh)
    {
      if (params_["useOURCVFH"]>0)
      {
        list.clear();
        boost::copy(ourcvfh_list_, back_inserter(list) );
        return true;
      }
      else if (params_["verbosity"]>0)
        print_warn("%*s]\tOURCVFH list was not generated, because 'useOURCVFH' parameter was set to zero, not copying anything...\n",20,__func__);
      return false;
    }
    else if (type == listType::composite)
    {
      list.clear();
      boost::copy(composite_list_, back_inserter(list) );
      return true;
    }
    else
      print_error("%*s]\tUnknown listType, not copying anything...\n",20,__func__);
    return false;
  }

  void PoseEstimation::printQuery()
  {
    if (!query_set_)
    {
      if (params_["verbosity"]>0)
        print_warn("%*s]\tQuery is not set yet...\n",20,__func__);
      return;
    }
    if (params_["verbosity"]>1)
      print_info("%*s]\tPrinting query informations:\n",20,__func__);
    print_info("Query: ");
    print_value("%s",query_name_.c_str());
    print_info(" with a total of ");
    print_value("%d\n", query_cloud_->points.size());
    print_info("Was preprocessed and now has ");
    print_value("%d",query_cloud_->points.size());
    print_info(" points\n");
    if (vfh_.points.size() != 0)
    {
      print_value("%d ",vfh_.points.size());
      print_info("VFH histogram estimated\n");
    }
    if (esf_.points.size() != 0)
    {
      print_value("%d ",esf_.points.size());
      print_info("ESF histogram estimated\n");
    }
    if (cvfh_.points.size() != 0)
    {
      print_value("%d ",cvfh_.points.size());
      print_info("CVFH histogram(s) estimated\n");
    }
    if (ourcvfh_.points.size() != 0)
    {
      print_value("%d ",ourcvfh_.points.size());
      print_info("OURCVFH histogram(s) estimated\n");
    }
  }

  bool PoseEstimation::getQuery(string& name, PtC::Ptr clp)
  {
    if (!query_set_)
    {
      if (params_["verbosity"]>0)
        print_warn("%*s]\tQuery is not set yet...\n",20,__func__);
      return false;
    }
    name = query_name_;
    if (clp)
      copyPointCloud(*query_cloud_, *clp);
    else
    {
      PtC cl;
      copyPointCloud(*query_cloud_, cl);
      clp = cl.makeShared();
    }
    return true;
  }

  int PoseEstimation::getQueryFeatures(PointCloud<VFHSignature308>::Ptr vfh, PointCloud<VFHSignature308>::Ptr cvfh, PointCloud<VFHSignature308>::Ptr ourcvfh, PointCloud<ESFSignature640>::Ptr esf, PointCloud<Normal>::Ptr normals)
  {
    if (!query_set_)
    {
      if (params_["verbosity"]>0)
        print_warn("%*s]\tQuery is not set yet...\n",20,__func__);
      return 0;
    }
    int count (0);
    try
    {
      if (vfh && vfh_.points.size() != 0)
      {
        copyPointCloud(vfh_, *vfh);
        ++count;
      }
      else if (!vfh && vfh_.points.size() != 0)
      {
        PointCloud<VFHSignature308> cl;
        copyPointCloud(vfh_, cl);
        vfh = cl.makeShared();
        ++count;
      }
      else
      {
        if (params_["verbosity"]>0)
          print_warn("%*s]\tVFH feature was not calculated\n",20,__func__);
      }
      if (cvfh && cvfh_.points.size() != 0)
      {
        copyPointCloud(cvfh_, *cvfh);
        ++count;
      }
      else if (!cvfh && cvfh_.points.size() != 0)
      {
        PointCloud<VFHSignature308> cl;
        copyPointCloud(cvfh_, cl);
        cvfh = cl.makeShared();
        ++count;
      }
      else
      {
        if (params_["verbosity"]>0)
          print_warn("%*s]\tCVFH feature was not calculated\n",20,__func__);
      }
      if (ourcvfh && ourcvfh_.points.size() != 0)
      {
        copyPointCloud(ourcvfh_, *ourcvfh);
        ++count;
      }
      else if (!ourcvfh && ourcvfh_.points.size() != 0)
      {
        PointCloud<VFHSignature308> cl;
        copyPointCloud(ourcvfh_, cl);
        ourcvfh = cl.makeShared();
        ++count;
      }
      else
      {
        if (params_["verbosity"]>0)
          print_warn("%*s]\tOURCVFH feature was not calculated\n",20,__func__);
      }
      if (esf && esf_.points.size() != 0)
      {
        copyPointCloud(esf_, *esf);
        ++count;
      }
      else if (!esf && esf_.points.size() != 0)
      {
        PointCloud<ESFSignature640> cl;
        copyPointCloud(esf_, cl);
        esf = cl.makeShared();
        ++count;
      }
      else
      {
        if (params_["verbosity"]>0)
          print_warn("%*s]\tESF feature was not calculated\n",20,__func__);
      }
      if (normals && normals_.points.size() != 0)
      {
        copyPointCloud(normals_, *normals);
        ++count;
      }
      else if (!normals && normals_.points.size() != 0)
      {
        PointCloud<Normal> cl;
        copyPointCloud(normals_, cl);
        normals = cl.makeShared();
        ++count;
      }
      else
      {
        if (params_["verbosity"]>0)
          print_warn("%*s]\tNormals were not calculated\n",20,__func__);
      }
    }
    catch (...)
    {
      print_error("%*s]\tError copying features...\n",20,__func__);
      return (-1);
    }
    return count;
  }

  void PoseEstimation::viewQuery()
  {
    if (!query_set_)
    {
      if (params_["verbosity"]>0)
        print_warn("%*s]\tQuery is not set yet...\n",20,__func__);
      return;
    }
    visualization::PCLVisualizer viewer;
    visualization::PCLHistogramVisualizer v_vfh, v_esf, v_cvfh, v_ourcvfh;
    if (params_["useVFH"] > 0)
      v_vfh.addFeatureHistogram (vfh_, 308, "vfh");
    if (params_["useESF"] > 0)
      v_esf.addFeatureHistogram (esf_, 640, "esf");
    if (params_["useCVFH"] > 0)
      v_cvfh.addFeatureHistogram (cvfh_, 308, "cvfh");
    if (params_["useOURCVFH"] > 0)
      v_ourcvfh.addFeatureHistogram (ourcvfh_, 308, "ourcvfh");
    viewer.addPointCloud(query_cloud_, "query");
    viewer.addCoordinateSystem(0.2);
    while(!viewer.wasStopped())
    {
      viewer.spinOnce();
      v_vfh.spinOnce();
      v_esf.spinOnce();
      v_cvfh.spinOnce();
      v_ourcvfh.spinOnce();
    }
    viewer.close();
    return;
  }

  void PoseEstimation::viewEstimation()
  {
    if (!refinement_done_)
    {
      if (params_["verbosity"] >0)
        print_warn("%*s]\tEstimation is not done yet...\n",20,__func__);
      return;
    }
    visualization::PCLVisualizer viewer;
    PtC::Ptr pe (new PtC);
    transformPointCloud( *(pose_estimation_->cloud_), *pe, pose_estimation_->transformation_ );
    pe->sensor_origin_.setZero();
    pe->sensor_orientation_.setIdentity();
    visualization::PointCloudColorHandlerCustom<Pt> candidate_color_handler (pe, 0, 255, 0);
    visualization::PointCloudColorHandlerCustom<Pt> query_color_handler (query_cloud_, 255, 0, 0);
    viewer.addPointCloud(query_cloud_, query_color_handler, "query");
    viewer.addCoordinateSystem(0.2);
    viewer.addPointCloud(pe, candidate_color_handler, "estimation");
    while(!viewer.wasStopped())
    {
      viewer.spinOnce();
    }
    viewer.close();
    return;
  }

  bool PoseEstimation::saveTestResults(path file)
  {
    if (!refinement_done_)
    {
      if (params_["verbosity"] >0)
        print_warn("%*s]\tEstimation is not done yet...\n",20,__func__);
      return false;
    }
    if(saveTestResults(file, pose_estimation_->name_))
      return true;
    else
      return false;
  }

  bool PoseEstimation::saveTestResults(path file, string gt)
  {
    if (!refinement_done_)
    {
      if (params_["verbosity"] >0)
        print_warn("%*s]\tEstimation is not done yet...\n",20,__func__);
      return false;
    }
    try
    {
      fstream f;
      f.open ( file.string().c_str(), fstream::out | fstream::app );
      if (vfh_list_.empty())
      {
        f<< -1 <<" ";
      }
      else
      {
        for (int i=0; i<vfh_list_.size();++i)
        {
          if (gt.compare(vfh_list_[i].name_) ==0)
          {
            f << i+1 << " ";
            break;
          }
          if (i==vfh_list_.size()-1)
            f << 0 << " ";
        }
      }
      if (esf_list_.empty())
      {
        f<< -1 <<" ";
      }
      else
      {
        for (int i=0; i<esf_list_.size();++i)
        {
          if (gt.compare(esf_list_[i].name_) ==0)
          {
            f << i+1 << " ";
            break;
          }
          if (i==esf_list_.size()-1)
            f << 0 << " ";
        }
      }
      if (cvfh_list_.empty())
      {
        f<< -1 <<" ";
      }
      else
      {
        for (int i=0; i<cvfh_list_.size();++i)
        {
          if (gt.compare(cvfh_list_[i].name_) ==0)
          {
            f << i+1 << " ";
            break;
          }
          if (i==cvfh_list_.size()-1)
            f << 0 << " ";
        }
      }
      if (ourcvfh_list_.empty())
      {
        f<< -1 <<" ";
      }
      else
      {
        for (int i=0; i<ourcvfh_list_.size();++i)
        {
          if (gt.compare(ourcvfh_list_[i].name_) ==0)
          {
            f << i+1 << " ";
            break;
          }
          if (i==ourcvfh_list_.size()-1)
            f << 0 << " ";
        }
      }
      for (int i=0; i<composite_list_.size();++i)
      {
        if (gt.compare(composite_list_[i].name_) ==0)
        {
          f << i+1 << " ";
          break;
        }
        if (i==composite_list_.size()-1)
          f << 0 << " ";
      }
      if (pose_estimation_->name_.compare(gt) == 0)
      {
        f<< 0 <<endl;
      }
      else
      {
        vector<string> vc, vg;
        split (vc, pose_estimation_->name_, boost::is_any_of("_"), boost::token_compress_on);
        split (vg, gt, boost::is_any_of("_"), boost::token_compress_on);
        if (vc.at(0).compare(vg.at(0)) == 0)
        {
          int lat_c, lat_g, lon_c, lon_g;
          lat_c = stoi (vc.at(1));
          lon_c = stoi (vc.at(2));
          lat_g = stoi (vg.at(1));
          lon_g = stoi (vg.at(2));
          if ( (lat_c >= lat_g-10) && (lat_c <= lat_g+10) && (lon_c >= lon_g-10) && (lon_c <= lon_g+10) )
            f << 1 <<endl;
          else
            f << 2 <<endl;
        }
        else
          f << 3 <<endl;
      }
      f.close();
    }
    catch (...)
    {
      print_error ("%*s]\tError writing to disk, check if %s is valid path and name convetion is respected.\n",20,__func__,file.string().c_str());
      return false;
    }
    if (params_["verbosity"]>1)
      print_info("%*s]\tSaved current pose estimation results in %s\n",20,__func__,file.string().c_str());
    return true;
  }

  bool PoseEstimation::elaborateTests(path file, path result)
  {
    int k = params_["kNeighbors"];
    unsigned int tot_v(0),tot_e(0),tot_c(0),tot_o(0),tot(0);
    vector<int> vfh_r(k+1,0), esf_r(k+1,0), cvfh_r(k+1,0), ourcvfh_r(k+1,0), comp_r(k+1,0), final_c(4,0);
    if (exists(file) && is_regular_file(file))
    {
      ifstream f (file.string().c_str());
      string line;
      if (f.is_open())
      {
        while (getline (f, line))
        {
          try
          {
            vector<string> vst;
            split (vst, line, boost::is_any_of(" "), boost::token_compress_on);
            int vfh,esf,cvfh,ourcvfh,comp,fin_c;
            vfh = stoi(vst.at(0));
            esf = stoi(vst.at(1));
            cvfh = stoi(vst.at(2));
            ourcvfh = stoi(vst.at(3));
            comp = stoi(vst.at(4));
            fin_c = stoi(vst.at(5));
            if (vfh != -1)
            {
              ++vfh_r[vfh];
              ++tot_v;
            }
            if (esf != -1)
            {
              ++esf_r[esf];
              ++tot_e;
            }
            if (cvfh != -1)
            {
              ++cvfh_r[cvfh];
              ++tot_c;
            }
            if (ourcvfh != -1)
            {
              ++ourcvfh_r[ourcvfh];
              ++tot_o;
            }
            ++comp_r[comp];
            ++final_c[fin_c];
            ++tot;
          }
          catch (...)
          {
            print_error ("%*s]\tError reading from test file. Try recreating it.\n",20,__func__);
            return false;
          }
        }//end of file
      }
      else
      {
        print_error ("%*s]\tError opening test file for reading. Try recreating it.\n",20,__func__);
        return false;
      }
      f.close();
    }
    else
    {
      if (params_["verbosity"] >0)
        print_warn("%*s]\tTest file %s does not exists. Nothing to elaborate...\n",20,__func__, file.string().c_str());
      return false;
    }
    if (exists(result) && is_regular_file(result))
    {
      if (params_["verbosity"] >0)
        print_warn("%*s]\tResult file %s already exists. Truncating its contents...\n",20,__func__, result.string().c_str());
    }
    fstream res;
    res.open ( result.string().c_str(), fstream::out | fstream::trunc );
    if (res.is_open())
    {
      timestamp t(TIME_NOW);
      res << "Tests elaborates on "<<to_simple_string(t).c_str()<<endl;
      res <<"Total of "<< tot_v <<" VFH tests: ";
      for (int i=0; i< vfh_r.size(); ++i)
      {
        if (i == 0)
          res << vfh_r[i]/tot_v*100 <<"%% Failed\t";
        else
          res << vfh_r[i]/tot_v*100 <<"%% Rank "<<i<<"\t";
      }
      res<<endl;
      res <<"Total of "<< tot_e <<" ESF tests: ";
      for (int i=0; i< esf_r.size(); ++i)
      {
        if (i == 0)
          res << esf_r[i] <<" failed,\t";
        else
          res << esf_r[i] <<" rank "<<i<<",\t";
      }
      res<<endl;
      res <<"Total of "<< tot_c <<" CVFH tests: ";
      for (int i=0; i< cvfh_r.size(); ++i)
      {
        if (i == 0)
          res << cvfh_r[i]<<" failed,\t";
        else
          res << cvfh_r[i]<<" rank "<<i<<",\t";
      }
      res<<endl;
      res <<"Total of "<< tot_o <<" OURCVFH tests: ";
      for (int i=0; i< ourcvfh_r.size(); ++i)
      {
        if (i == 0)
          res << ourcvfh_r[i]<<" failed,\t";
        else
          res << ourcvfh_r[i]<<" rank "<<i<<",\t";
      }
      res<<endl;
      res <<"Total of "<< tot <<" Composite list tests: ";
      for (int i=0; i< comp_r.size(); ++i)
      {
        if (i == 0)
          res << comp_r[i]<<" failed,\t";
        else
          res << comp_r[i]<<" rank "<<i<<",\t";
      }
      res<<endl<<endl;
      res<<"On a total of "<<tot<<" tests, the final candidate chosen was:"<<endl;
      res<<"the correct pose "<<final_c[0]<<" times"<<endl;
      res<<"a direct neighbor of the correct pose "<<final_c[1]<<" times"<<endl;
      res<<"the same object with a wrong orientation "<<final_c[2]<<" times"<<endl;
      res<<"another object "<<final_c[3]<<" times"<<endl;
    }
    else
    {
      print_error ("%*s]\tError opening result file for writing. Check if %s is a valid path.\n",20,__func__,result.string().c_str());
      return false;
    }
    res.close();
    return true;
  }
  */
}

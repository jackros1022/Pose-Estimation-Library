/* This file implementes the Pose Estimation interface, thus contains definitions,
 * for the interface declarations look in PoseEstimation_interface.h
 */
#ifndef __INTERFACE_HPP_INCLUDED__
#define __INTERFACE_HPP_INCLUDED__

#include <pcl/io/pcd_io.h>
#include <pcl/common/norms.h>
#include <pcl/common/time.h>
#include <pcl/console/print.h>
#include <pcl/surface/mls.h>
#include <pcl/search/kdtree.h>
#include <pcl/features/normal_3d_omp.h>
#include <pcl/filters/statistical_outlier_removal.h>
#include <pcl/features/vfh.h>
#include <pcl/features/esf.h>
#include <pcl/features/cvfh.h>
#include <pcl/features/our_cvfh.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/registration/icp.h>
#include <pcl/visualization/pcl_visualizer.h>
#include <flann/flann.h>
#include <flann/io/hdf5.h>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <algorithm>
#include <fstream>
#include <cmath>
#include <stdexcept>
#include <pcl/common/centroid.h>
//Definition header
#include "PoseEstimation_interface.h"

#define D2R 0.017453293 //degrees to radians conversion

using namespace pcl::console;
using namespace boost;
using namespace boost::filesystem;

/* Class Candidate Implementation */

Candidate::Candidate ()
{
  name_ = "UNSET";
  rank_ = -1;
  distance_ = -1;
  rmse_ = -1;
  transformation_.setIdentity();
}
///////////////////////////////////////////////////////////////////
Candidate::Candidate (string& str, PointCloud<PointXYZRGBA>& cl)
{
  name_ = str;
  cloud_ = cl.makeShared();
  rank_ = -1;
  distance_ = -1;
  rmse_ = -1;
  transformation_.setIdentity();
}
/////////////////////////////////////////////////////////////////////
Candidate::Candidate (string& str, PointCloud<PointXYZRGBA>::Ptr clp)
{
  name_ = str;
  cloud_ = clp;
  rank_ = -1;
  distance_ = -1;
  rmse_ = -1;
  transformation_.setIdentity();
}
///////////////////////////////////////////////////////////////////////
void Candidate::setRank (float r) 
{ 
  rank_ = r; 
}
void Candidate::getRank (float& r) 
{ 
  r = rank_; 
}
void Candidate::setDistance (float d) 
{ 
  distance_ = d; 
}
void Candidate::getDistance (float& d) 
{ 
  d = distance_; 
}
void Candidate::setRMSE (float r) 
{ 
  rmse_ = r; 
}
void Candidate::getRMSE (float& r) 
{ 
  r = rmse_; 
}
void Candidate::getTransformation (Eigen::Matrix4f& t) 
{ 
  t = transformation_; 
}
void Candidate::setTransformation (Eigen::Matrix4f t) 
{ 
  transformation_ = t; 
}

/* Class PoseEstimation Implementation */

PoseEstimation::PoseEstimation ()
{
  vp_supplied_ = query_set_ = candidates_found_ = refinement_done_ = false;
  feature_count_ = 4;
  params_["verbosity"]=1;
  params_["useVFH"]=params_["useESF"]=params_["useCVFH"]=params_["useOURCVFH"]=1;
  params_["progBisection"]=params_["downsampling"]=1;
  params_["vgridLeafSize"]=0.003f;
  params_["upsampling"]=params_["filtering"]=0;
  params_["kNeighbors"]=20;
  params_["maxIterations"]=200;
  params_["progItera"]=5;
  params_["progFraction"]=0.5f;
  params_["rmseThreshold"]=0.003f;
  params_["mlsPolyOrder"]=2;
  params_["mlsPointDensity"]=250;
  params_["mlsPolyFit"]=1;
  params_["mlsSearchRadius"]=0.03f;
  params_["filterMeanK"]=50;
  params_["filterStdDevMulThresh"]=3;
  params_["neRadiusSearch"]=0.015;
  params_["useSOasViewpoint"]=1;
  params_["computeViewpointFromName"]=0;
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
void PoseEstimation::setParam(string& key, float value)
{
  int size = params_.size();
  if (value < 0)
  {
    if (params_["verbosity"]>0)
      print_warn("[setParam]\tParameter '%s' has a negative value (%f), ignoring...\n", key.c_str(), value);
    exit;
  }
  params_[key]=value;
  //Check if key was a valid one, since the class has fixed number of parameters, 
  //if one was mispelled, now we have one more 
  if (params_.size() != size)
  {
    if (params_["verbosity"]>0)
      print_warn("[setParam]\tInvalid key parameter '%s', ignoring...\n", key.c_str());
    params_.erase(key);
  }
  else if (params_["verbosity"]>1)
    print_info("[setParam]\tSetting parameter: %s=%f\n",key.c_str(),value);
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
    print_warn("[setParam]\tYou disabled all features, pose estimation will not initialize query...\n");
}
/////////////////////////////////////////////////////////////////////////////////////
void PoseEstimation::setParam_ (string& key, string& value)
{
  float f;
  try
  {
    f = stof(value);
  }
  catch (const std::invalid_argument& ia)
  {
    if (params_["verbosity"] > 0)
      print_warn("[setParam_]\tInvalid %s=%s : %s \n", key.c_str(), value.c_str(), ia.what());
  }
  catch (const std::out_of_range& oor)
  {
    if (params_["verbosity"] > 0)
      print_warn("[setParam]\tInvalid %s=%s : %s \n", key.c_str(), value.c_str(), oor.what());
  }
  setParam(key, f); 
}
//////////////////////////////////////////////////////////////////////////////////////
void PoseEstimation::initParams(path config_file)
{ 
  if ( exists(config_file) && is_regular_file(config_file))   
  {
    if (extension(config_file)==".conf") 
    {
      ifstream file (config_file.string().c_str());
      string line;
      if (file.is_open())
      {
        while (getline (file, line))
        {
          trim(line); //remove white spaces from line
          if (line.empty())
          {
            //do nothing empty line ...
          }
          else if (line.compare(0,1,"%") == 0 )
          {
            //do nothing comment line ...
          }
          else
          {
            vector<string> vst;
            //split the line to get a key and a token
            split(vst, line, boost::is_any_of("="), boost::token_compress_on);
            if (vst.size()!=2)
            {
              if (params_["verbosity"]>0)
                print_warn("[initParams]\tInvalid configuration line (%s), ignoring... Must be [Token]=[Value]\n", line.c_str());
              continue;
            }
            setParam_(vst.at(0), vst.at(1));
          }
        }//end of config file
      }
      else
        print_error("[initParams]\tCannot open config file! (%s)\n", config_file.string().c_str());
    }  
    else
      print_error("[initParams]\tConfig file provided (%s) has no valid extension! (must be .conf)\n", config_file.string().c_str());
  }
  else
    print_error("[initParams]\tPath to Config File is not valid ! (%s)\n", config_file.string().c_str());
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void PoseEstimation::filtering_()
{
  StopWatch timer;
  if (params_["verbosity"] >1)
  {
    print_info("[filtering]\tSetting Statistical Outlier Filter to preprocess query cloud...\n");
    print_info("[filtering]\tSetting mean K to %f\n", params_["filterMeanK"]);
    print_info("[filtering]\tSetting Standard Deviation multiplier to %f\n", params_["filterStdDevMulThresh"]);
    timer.reset();
  }
  PointCloud<PointXYZRGBA>::Ptr filtered (new PointCloud<PointXYZRGBA>);
  StatisticalOutlierRemoval<PointXYZRGBA> fil;
  fil.setMeanK (params_["filterMeanK"]);
  fil.setStddevMulThresh (params_["filterStdDevMulThresh"]);
  fil.setInputCloud(query_cloud_);
  fil.filter(*filtered);
  if (query_cloud_processed_)
    copyPointCloud(*filtered, *query_cloud_processed_);
  else
    query_cloud_processed_ = filtered;
  if (params_["verbosity"]>1)
  {
    print_info("[filtering]\tTotal time elapsed during filter: ");
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
    print_info("[upsampling]\tSetting MLS with Random Uniform Density to preprocess query cloud...\n");
    print_info("[upsampling]\tSetting polynomial order to %f\n", params_["mlsPolyOrder"]);
    string t = params_["mlsPolyFit"] ? "true" : "false";
    print_info("[upsampling]\tSetting polynomial fit to %s\n", t.c_str());
    print_info("[upsampling]\tSetting desired point density to %f\n", params_["mlsPointDensity"]);
    print_info("[upsampling]\tSetting search radius to %f\n", params_["mlsSearchRadius"]);
    timer.reset();
  }
  PointCloud<PointXYZRGBA>::Ptr upsampled (new PointCloud<PointXYZRGBA>);
  search::KdTree<PointXYZRGBA>::Ptr tree (new search::KdTree<PointXYZRGBA>);
  MovingLeastSquares<PointXYZRGBA, PointXYZRGBA> mls;
  if (query_cloud_processed_)
    mls.setInputCloud(query_cloud_processed_);
  else
    mls.setInputCloud(query_cloud_);
  mls.setSearchMethod(tree);
  mls.setUpsamplingMethod (MovingLeastSquares<PointXYZRGBA, PointXYZRGBA>::RANDOM_UNIFORM_DENSITY);
  mls.setComputeNormals (false);
  mls.setPolynomialOrder(params_["mlsPolyOrder"]);
  mls.setPolynomialFit(params_["mlsPolyFit"]);
  mls.setSearchRadius(params_["mlsSearchRadius"]);
  mls.setPointDensity(params_["mlsPointDensity"]);
  mls.process(*upsampled);
  if (query_cloud_processed_)
    copyPointCloud(*upsampled, *query_cloud_processed_);
  else
    query_cloud_processed_ = upsampled;
  if (params_["verbosity"]>1)
  {
    print_info("[upsampling]\tTotal time elapsed during upsampling: ");
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
    print_info("[downsampling]\tSetting Voxel Grid to preprocess query cloud...\n");
    print_info("[downsampling]\tSetting Leaf Size to %f\n", params_["vgridLeafSize"]);
    timer.reset();
  }
  PointCloud<PointXYZRGBA>::Ptr downsampled (new PointCloud<PointXYZRGBA>);
  VoxelGrid<PointXYZRGBA> vg;
  if (query_cloud_processed_)
    vg.setInputCloud(query_cloud_processed_);
  else
    vg.setInputCloud(query_cloud_);
  vg.setLeafSize (params_["vgridLeafSize"], params_["vgridLeafSize"], params_["vgridLeafSize"]);
  vg.setDownsampleAllData (true);
  vg.filter(*downsampled);
  if (query_cloud_processed_)
    copyPointCloud(*downsampled, *query_cloud_processed_);
  else
    query_cloud_processed_ = downsampled;
  if (params_["verbosity"]>1)
  {
    print_info("[downsampling]\tTotal time elapsed during downsampling: ");
    print_value("%g", timer.getTime());
    print_info(" ms\n");
  }
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
void PoseEstimation::setQueryViewpoint(float x, float y, float z)
{
  vpx_ = x;
  vpy_ = y;
  vpz_ = z;
  vp_supplied_ = true;
}
/////////////////////////////////////////////////////////////////////////////////////////////////
bool PoseEstimation::computeNormals_()
{
  StopWatch timer;
  if (params_["verbosity"]>1)
  {
    print_info("[normalEstimation]\tSetting normal estimation to calculate query normals...\n");
    print_info("[normalEstimation]\tSetting a neighborhood radius of %f\n", params_["neRadiusSearch"]);
    timer.reset();
  }
  NormalEstimationOMP<PointXYZRGBA, Normal> ne;
  search::KdTree<PointXYZRGBA>::Ptr tree (new search::KdTree<PointXYZRGBA>);
  ne.setSearchMethod(tree);
  ne.setRadiusSearch(params_["neRadiusSearch"]);
  ne.setNumberOfThreads(0); //use pcl autoallocation
  ne.setInputCloud(query_cloud_processed_);
  if (vp_supplied_)
  {
    //A Viewpoint was already supplied by setQueryViewpoint, so we use it
    ne.setViewPoint (vpx_, vpy_, vpz_);
    if (params_["verbosity"] >1)
      print_info("[normalEstimation]\tUsing supplied viewpoint: %f, %f, %f\n", vpx_, vpy_, vpz_);
  }
  else if (params_["computeViewpointFromName"])
  {
    //Try to compute viewpoint from query name
    try
    {
      //assume correct naming convection (name_lat_long)
      //If something goes wrong an exception is catched
      vector<string> vst;
      split(vst, query_name_, boost::is_any_of("_"), boost::token_compress_on);
      float vx,vy,vz;
      int lat,lon;
      lat = stoi(vst.at(1));
      lon = stoi(vst.at(2));
      //assume radius of one meter and reference frame in object center
      vx = cos(lat*D2R)*sin(lon*D2R);
      vy = sin(lat*D2R);
      vz = cos(lat*D2R)*cos(lon*D2R);
      setQueryViewpoint(vx,vy,vz);
      if (vp_supplied_)
      {
        if (params_["verbosity"]>1)
          print_info("[normalEstimation]\tUsing calculated viewpoint: %f, %f, %f\n", vpx_, vpy_, vpz_);
        ne.setViewPoint(vpx_, vpy_, vpz_);
      }
    }
    catch (...)
    {
      //something went wrong
      print_error("[normalEstimation]\tCannot compute Viewpoint from query name... check naming convection and object reference frame!\n");
      return false;
    }
  }
  else if (params_["useSOasViewpoint"])
  {
    //Use Viewpoint stored in sensor_origin_ of query cloud
    //However we want to save it in class designated spots so it can be used again by 
    //other features
    float vx,vy,vz;
    vx = query_cloud_->sensor_origin_[0];
    vy = query_cloud_->sensor_origin_[1];
    vz = query_cloud_->sensor_origin_[2];
    setQueryViewpoint(vx,vy,vz);
    if (vp_supplied_)
    {
      ne.setViewPoint (vpx_, vpy_, vpz_);
      if (params_["verbosity"]>1)
        print_info("[normalEstimation]\tUsing viewpoint from sensor_origin_: %f, %f, %f\n", vpx_, vpy_, vpz_);
    }
    else
    {
      print_error("[normalEstimation]\tCannot set viewpoint from sensor_origin_...\n");
      return false;
    }
  }
  else
  {
    print_error("[normalEstimation]\tNo Viewpoint supplied!! Cannot continue!!\n");
    print_error("[normalEstimation]\tEither use setQueryViewpoint or set 'useSOasViewpoint'/'computeViewpointFromName'\n");
    return false;
  }
  ne.compute(normals_);
  if (params_["verbosity"]>1)
  {
    print_info("[normalEstimation]\tTotal time elapsed during normal estimation: ");
    print_value("%g", timer.getTime());
    print_info(" ms\n");
  }
  return true;
}
//////////////////////////////////////////////////////////////////////////////////////////////
void PoseEstimation::computeVFH_()
{
  if(!vp_supplied_)
  {
    //Should never happen, viewpoint was set by normal estimation
    print_error("[VFH]\tCannot estimate VFH of query, viewpoint was not set...\n");
    exit;
  }
  else
  {
    StopWatch timer;
    if (params_["verbosity"]>1)
    {
      print_info("[VFH]\tEstimating VFH feature of query...\n");
      timer.reset();
    }
    VFHEstimation<PointXYZRGBA, Normal, VFHSignature308> vfhE;
    search::KdTree<PointXYZRGBA>::Ptr tree (new search::KdTree<PointXYZRGBA>);
    vfhE.setSearchMethod(tree);
    vfhE.setInputCloud (query_cloud_processed_);
    vfhE.setViewPoint (vpx_, vpy_, vpz_);
    vfhE.setInputNormals (normals_.makeShared());
    vfhE.compute (vfh_);
    if (params_["verbosity"]>1)
    {
      print_info("[VFH]\tTotal time elapsed during VFH estimation: ");
      print_value("%g", timer.getTime());
      print_info(" ms\n");
    }
  }
}
////////////////////////////////////////////////////////////////////////////////////
void PoseEstimation::computeESF_()
{
  StopWatch timer;
  if (params_["verbosity"]>1)
  {
    print_info("[ESF]\tEstimating ESF feature of query...\n");
    timer.reset();
  }
  ESFEstimation<PointXYZRGBA, ESFSignature640> esfE;
  search::KdTree<PointXYZRGBA>::Ptr tree (new search::KdTree<PointXYZRGBA>);
  esfE.setSearchMethod(tree);
  esfE.setInputCloud (query_cloud_processed_);
  esfE.compute (esf_);
  if (params_["verbosity"]>1)
  {
    print_info("[ESF]\tTotal time elapsed during ESF estimation: ");
    print_value("%g", timer.getTime());
    print_info(" ms\n");
  }
}
//////////////////////////////////////////////////////////////////////////////////////
void PoseEstimation::computeCVFH_()
{
  if(!vp_supplied_)
  {
    //Should never happen, viewpoint was set by normal estimation
    print_error("[CVFH]\tCannot estimate CVFH of query, viewpoint was not set...\n");
    exit;
  }
  else
  {
    StopWatch timer;
    if (params_["verbosity"]>1)
    {
      print_info("[CVFH]\tEstimating CVFH feature of query...\n");
      print_info("[CVFH]\tUsing Angle Threshold of %f degress for normal deviation\n",params_["cvfhEPSAngThresh"]); 
      print_info("[CVFH]\tUsing Curvature Threshold of %f\n",params_["cvfhCurvThresh"]); 
      print_info("[CVFH]\tUsing Cluster Tolerance of %f\n",params_["cvfhClustTol"]); 
      print_info("[CVFH]\tConsidering a minimum of %f points for a cluster\n",params_["cvfhMinPoints"]); 
      timer.reset();
    }
    CVFHEstimation<PointXYZRGBA, Normal, VFHSignature308> cvfhE;
    search::KdTree<PointXYZRGBA>::Ptr tree (new search::KdTree<PointXYZRGBA>);
    cvfhE.setSearchMethod(tree);
    cvfhE.setInputCloud (query_cloud_processed_);
    cvfhE.setViewPoint (vpx_, vpy_, vpz_);
    cvfhE.setInputNormals (normals_.makeShared());
    cvfhE.setEPSAngleThreshold(params_["cvfhEPSAngThresh"]*D2R); //angle needs to be supplied in radians
    cvfhE.setCurvatureThreshold(params_["cvfhCurvThresh"]);
    cvfhE.setClusterTolerance(params_["cvfhClustTol"]);
    cvfhE.setMinPoints(params_["cvfhMinPoints"]);
    cvfhE.setNormalizeBins(false);
    cvfhE.compute (cvfh_);
    if (params_["verbosity"]>1)
    {
      print_info("[CVFH]\tTotal of %d clusters were found on query\n", cvfh_.points.size());
      print_info("[CVFH]\tTotal time elapsed during CVFH estimation: ");
      print_value("%g", timer.getTime());
      print_info(" ms\n");
    }
  }
}
//////////////////////////////////////////////////////////////////////////////////////////////////
void PoseEstimation::computeOURCVFH_()
{
  if(!vp_supplied_)
  {
    //Should never happen, viewpoint was set by normal estimation
    print_error("[OURCVFH]\tCannot estimate OURCVFH of query, viewpoint was not set...\n");
    exit;
  }
  else
  {
    StopWatch timer;
    if (params_["verbosity"]>1)
    {
      print_info("[OURCVFH]\tEstimating OURCVFH feature of query...\n");
      print_info("[OURCVFH]\tUsing Angle Threshold of %f degress for normal deviation\n",params_["ourcvfhEPSAngThresh"]); 
      print_info("[OURCVFH]\tUsing Curvature Threshold of %f\n",params_["ourcvfhCurvThresh"]); 
      print_info("[OURCVFH]\tUsing Cluster Tolerance of %f\n",params_["ourcvfhClustTol"]); 
      print_info("[OURCVFH]\tConsidering a minimum of %f points for a cluster\n",params_["ourcvfhMinPoints"]); 
      print_info("[OURCVFH]\tUsing Axis Ratio of %f and Min Axis Value of %f during SGURF disambiguation\n",params_["ourcvfhAxisRatio"],params_["ourcvfhMinAxisValue"]); 
      print_info("[OURCVFH]\tUsing Refinement Factor of %f for clusters\n",params_["ourcvfhRefineClusters"]); 
      timer.reset();
    }
    OURCVFHEstimation<PointXYZ, Normal, VFHSignature308> ourcvfhE;
    search::KdTree<PointXYZ>::Ptr tree (new search::KdTree<PointXYZ>);
    PointCloud<PointXYZ>::Ptr cloud (new PointCloud<PointXYZ>);
    copyPointCloud(*query_cloud_processed_, *cloud);
    ourcvfhE.setSearchMethod(tree);
    ourcvfhE.setInputCloud (cloud);
    ourcvfhE.setViewPoint (vpx_, vpy_, vpz_);
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
      print_info("[OURCVFH]\tTotal of %d clusters were found on query\n", ourcvfh_.points.size());
      print_info("[OURCVFH]\tTotal time elapsed during OURCVFH estimation: ");
      print_value("%g", timer.getTime());
      print_info(" ms\n");
    }
  }
}
/////////////////////////////////////////////////////////////////////////////////////////////////
bool PoseEstimation::initQuery_()
{
  if (feature_count_ <=0)
  {
    if (params_["verbosity"]>0)
      print_warn("[initQuery]\tCannot initialize query, zero features chosen to estimate from query\n");
    return false;
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
  if (! query_cloud_processed_ )
    query_cloud_processed_ = query_cloud_;

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
    print_info("[setQuery]\tQuery succesfully set and initialized:\n");
    print_info("[setQuery]\t");
    print_value("%s", query_name_.c_str());
    print_info(" with ");
    print_value("%d", query_cloud_processed_->points.size());
    print_info(" points\n");
  }
  return true;
}
/////////////////////////////////////////////////////////////////////////////////////
void PoseEstimation::setQuery(string& str, PointCloud<PointXYZRGBA>& cl)
{
  query_name_ = str;
  if (query_cloud_)
    copyPointCloud(cl, *query_cloud_);
  else
    query_cloud_ = cl.makeShared();
  if (initQuery_())
    query_set_ = true;
}
/////////////////////////////////////////////////////////////////////////////////////
void PoseEstimation::setQuery(string& str, PointCloud<PointXYZRGBA>::Ptr clp)
{
  query_name_ = str;
  if (query_cloud_)
    copyPointCloud(*clp, *query_cloud_);
  else
    query_cloud_ = clp;
  if (initQuery_())
    query_set_ = true;
}
/////////////////////////////////////////////////////////////////////////////////////////
void PoseEstimation::printParams()
{
  for (auto &x : params_)
    cout<< x.first.c_str() <<"="<< x.second<<endl;
}
#endif

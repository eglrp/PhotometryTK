//__BEGIN_LICENSE__
//  Copyright (c) 2009-2012, United States Government as represented by the
//  Administrator of the National Aeronautics and Space Administration. All
//  rights reserved.
//
//  The NGT platform is licensed under the Apache License, Version 2.0 (the
//  "License"); you may not use this file except in compliance with the
//  License. You may obtain a copy of the License at
//  http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
// __END_LICENSE__


#ifdef _MSC_VER
#pragma warning(disable:4244)
#pragma warning(disable:4267)
#pragma warning(disable:4996)
#endif

#include <iostream>
#include <string>
#include <vector>
#include <limits>
#include <sys/types.h>
#include <sys/stat.h>

#include <vw/Core.h>
#include <vw/Image.h>
#include <vw/FileIO.h>
#include <vw/Cartography.h>
#include <vw/Math.h>
using namespace vw;
using namespace vw::cartography;

#include <photk/ShapeFromShading.h>
#include <photk/Shape.h>
#include <photk/Albedo.h>
#include <photk/Exposure.h>
#include <photk/Reflectance.h>
#include <photk/Reconstruct.h>
#include <photk/Shadow.h>
#include <photk/Index.h>
#include <photk/Weights.h>
using namespace photometry;


/*
Vector2 ComputeImageCenter2D(std::string input_img_file, float *maxDist)
{

    //compute the center of the image
    DiskImageView<PixelMask<PixelGray<uint8> > >  input_img(input_img_file);
    GeoReference input_geo;
    read_georeference(input_geo, input_img_file);

    int k, l;
    int numSamples = 0;

    Vector2 C;
    C(0) = 0;
    C(1) = 0;

    //initialize  output_img, and numSamples
    for (k = 0 ; k < input_img.rows(); ++k) {
        for (l = 0; l < input_img.cols(); ++l) {

           Vector2 input_image_pix(l,k);

           if ( is_valid(input_img(l,k)) ) {

              C(0) = C(0) + l;
              C(1) = C(1) + k;

              numSamples++;
          }
      }
   }
   C(0) = C(0)/numSamples;
   C(1) = C(1)/numSamples;


   float l_maxDist = 0;
    for (k = 0 ; k < input_img.rows(); ++k) {
        for (l = 0; l < input_img.cols(); ++l) {

           Vector2 input_image_pix(l,k);

           if ( is_valid(input_img(l,k)) ) {

              Vector2 dist_t;
              dist_t[0] = l-C(0);
              dist_t[1] = k-C(1);


              float dist = (dist_t[0]*dist_t[0]) + (dist_t[0]*dist_t[0]) ;
              if (dist > l_maxDist){
                 l_maxDist = dist;
              }
          }
      }
   }

    l_maxDist = sqrt(l_maxDist);

    printf("file=%s, i = %f, j = %f, maxDist = %f\n",input_img_file.c_str(), C(0), C(1), l_maxDist);

    *maxDist = l_maxDist;
    return C;
}
float ComputeWeights(Vector2 pix, Vector2 C, float maxDistance)
{

  float a = -1.0/maxDistance;
  float b = 1;

  float weight;
  Vector2 dist;

  dist[0]= pix[0]-C[0];
  dist[1]= pix[1]-C[1];

  float t_dist = sqrt(dist[0]*dist[0] + dist[1]*dist[1]);
  weight = a*t_dist + b;

  return weight;
}
*/

template<class ImageT>
void  ComputeCenterLines(struct ModelParams & modelParams,
                         ImageT const& input_img){

  // Compute the centerlines of the image. Their product at each pixel
  // will give a function which is continuous, positive where the
  // image has data, and zero where it does not.
  
  // A re-implementation of ComputeImageHCenterLine and ComputeImageVCenterLine
  // which is more efficient and uses less memory since it does only one
  // pass through the image.

  int numRows = input_img.rows();
  int numCols = input_img.cols();

  // Arrays to be returned out of this function
  std::vector<int> hCenterLine  (numRows, 0);
  std::vector<int> hMaxDistArray(numRows, 0);
  std::vector<int> vCenterLine  (numCols, 0);
  std::vector<int> vMaxDistArray(numCols, 0);

  // Temporary arrays
  std::vector<int> minValInRow(numRows, 0);
  std::vector<int> maxValInRow(numRows, 0);
  std::vector<int> minValInCol(numCols, 0);
  std::vector<int> maxValInCol(numCols, 0);

  for (int k = 0; k < numRows; k++){
    minValInRow[k] = numCols;
    maxValInRow[k] = 0;
  }
  for (int l = 0; l < numCols; l++){
    minValInCol[l] = numRows;
    maxValInCol[l] = 0;
  }
    
  for (int k = 0 ; k < numRows; ++k) {
    for (int l = 0; l < numCols; ++l) {

      if ( !is_valid(input_img(l,k)) ) continue;
            
      if (l < minValInRow[k]) minValInRow[k] = l;
      if (l > maxValInRow[k]) maxValInRow[k] = l;
      
      if (k < minValInCol[l]) minValInCol[l] = k;
      if (k > maxValInCol[l]) maxValInCol[l] = k;
      
    }
  }
    
  for (int k = 0 ; k < numRows; ++k) {
    hCenterLine   [k] = (minValInRow[k] + maxValInRow[k])/2;
    hMaxDistArray [k] =  maxValInRow[k] - minValInRow[k];
    if (hMaxDistArray[k] < 0){
      hMaxDistArray[k]=0;
    }
    //printf("cl[%d] = %d, maxDist[%d] = %d\n", k, hCenterLine[k], k, hMaxDistArray[k]);
  }

  for (int l = 0 ; l < numCols; ++l) {
    vCenterLine   [l] = (minValInCol[l] + maxValInCol[l])/2;
    vMaxDistArray [l] =  maxValInCol[l] - minValInCol[l];
    if (vMaxDistArray[l] < 0){
      vMaxDistArray[l]=0;
    }
    //printf("cl[%d] = %d, maxDist[%d] = %d\n", l, vCenterLine[l], l, vMaxDistArray[l]);
  }

    
  modelParams.hMaxDistArray = hMaxDistArray;
  modelParams.hCenterLine   = hCenterLine;
  modelParams.vMaxDistArray = vMaxDistArray;
  modelParams.vCenterLine   = vCenterLine;

  //system("echo Weight top is $(top -u $(whoami) -b -n 1|grep lt-reconstruct)");
  
#if 0 // Debugging code
  ImageView< PixelGray<double> >  weights(input_img.cols(), input_img.rows());
  for (int row = 0; row < weights.rows(); row++){
    for (int col = 0; col < weights.cols(); col++){
      weights(col, row) = ComputeLineWeightsHV(Vector2(col, row), modelParams);
      //std::cout << "weight is " << weights(col, row) << std::endl;
    }
  }
#endif
  
  return;
}

void photometry::ComputeImageCenterLines(struct ModelParams & modelParams){

  // Here we assume the image has two bands, with the second one being
  // the mask.
  std::string input_img_file = modelParams.inputFilename;
  
  printf("file=%s\n", input_img_file.c_str());
  
  DiskImageView<PixelMask<PixelGray<uint8> > >  input_img(input_img_file);
  ComputeCenterLines(modelParams, input_img);
}

void photometry::ComputeDEMCenterLines(struct ModelParams & modelParams){

  // Here we assume that the image has just one band, but may
  // have a no-data field which we use to mask invalid pixels.
  
  std::string dem_file = modelParams.inputFilename;
  vw_out() << "Computing centerlines for: " << dem_file << std::endl;
  
  // If we have a nodata value, create a mask.
  double nodata_value = std::numeric_limits<double>::quiet_NaN();
  {
    DiskImageResourceGDAL in_rsrc(dem_file);
    if ( in_rsrc.has_nodata_read() ) nodata_value = in_rsrc.nodata_read();
  }
  
  DiskImageView<float> dem_disk_image(dem_file);
  ImageViewRef< PixelMask<float> > dem = create_mask(dem_disk_image, nodata_value);
  
  ComputeCenterLines(modelParams, dem);
}


int*
photometry::ComputeImageHCenterLine(std::string input_img_file,
                                       int **r_hMaxDistArray) {

    //compute the center of the image
    DiskImageView<PixelMask<PixelGray<uint8> > >  input_img(input_img_file);

    int k, l;

    int *hCenterLine = new int [input_img.rows()];
    int* hMaxDistArray = new int [input_img.rows()];

    int minVal, maxVal;

    printf("file=%s\n",input_img_file.c_str());

    //initialize  output_img, and numSamples
    for (k = 0 ; k < (int)input_img.rows(); ++k) {

        minVal = input_img.cols();
        maxVal = 0;
        for (l = 0; l < (int)input_img.cols(); ++l) {


           if ( is_valid(input_img(l,k)) ) {

             if (l < minVal){
                 minVal = l;
             }
             if (l > maxVal){
                 maxVal = l;
             }
          }
      }
      hCenterLine[k] = (minVal + maxVal)/2;
      hMaxDistArray[k] = maxVal - minVal;
      if (hMaxDistArray[k] < 0){
          hMaxDistArray[k]=0;
      }
      //printf("cl[%d] = %d, maxDist[%d] = %d\n", k, hCenterLine[k], k, hMaxDistArray[k]);
   }

    *r_hMaxDistArray = hMaxDistArray;

    //system("echo hWeight top is $(top -u $(whoami) -b -n 1|grep lt-reconstruct)");
    
   return hCenterLine;
}


int*
photometry::ComputeImageVCenterLine(std::string input_img_file,
                                       int **r_vMaxDistArray) {

    //compute the center of the image
    DiskImageView<PixelMask<PixelGray<uint8> > >  input_img(input_img_file);

    int k, l;

    int *vCenterLine = new int [input_img.cols()];
    int* vMaxDistArray = new int [input_img.cols()];

    int minVal, maxVal;

    printf("file=%s\n",input_img_file.c_str());

    //initialize  output_img, and numSamples
    for (k = 0 ; k < (int)input_img.cols(); ++k) {
      
        minVal = input_img.rows();
        maxVal = 0;
        for (l = 0; l < (int)input_img.rows(); ++l) {

          if ( is_valid(input_img(k, l)) ) {

             if (l < minVal){
                 minVal = l;
             }
             if (l > maxVal){
                 maxVal = l;
             }
          }
      }
      vCenterLine[k] = (minVal + maxVal)/2;
      vMaxDistArray[k] = maxVal - minVal;
      if (vMaxDistArray[k] < 0){
          vMaxDistArray[k]=0;
      }
      //printf("cl[%d] = %d, maxDist[%d] = %d\n", k, vCenterLine[k], k, vMaxDistArray[k]);
   }

    *r_vMaxDistArray = vMaxDistArray;

    //system("echo vWeight top is $(top -u $(whoami) -b -n 1|grep lt-reconstruct)");
    
   return vCenterLine;
}


int*
photometry::ComputeDEMHCenterLine(std::string input_DEM_file,int noDataDEMVal,
                                     int **r_hMaxDistArray) {

    //compute the center of the image
    DiskImageView<PixelGray<float> >   input_DEM(input_DEM_file);

    int k, l;

    int *hCenterLine = new int[input_DEM.rows()];
    int *hMaxDistArray = new int[input_DEM.rows()];

    int minVal, maxVal;

    printf("file=%s\n",input_DEM_file.c_str());
    //initialize  output_img, and numSamples
    for (k = 0 ; k < input_DEM.rows(); ++k) {

        minVal = input_DEM.cols();
        maxVal = 0;
        for (l = 0; l < input_DEM.cols(); ++l) {

	  if ( input_DEM(l,k) != noDataDEMVal ) {
             if (l < minVal){
                 minVal = l;
             }
             if (l > maxVal){
                 maxVal = l;
             }
          }
      }
      hCenterLine[k] = (minVal + maxVal)/2;
      hMaxDistArray[k] = maxVal - minVal;
      if (hMaxDistArray[k] < 0){
        hMaxDistArray[k]=0;
      }
      //printf("cl[%d] = %d, maxDist[%d] = %d\n", k, hCenterLine[k], k, hMaxDistArray[k]);
   }



   *r_hMaxDistArray = hMaxDistArray;
   return hCenterLine;
}

int*
photometry::ComputeDEMVCenterLine(std::string input_DEM_file,int noDataDEMVal,
                                     int **r_vMaxDistArray) {

    //compute the center of the image
    DiskImageView<PixelGray<float> >   input_DEM(input_DEM_file);

    int k, l;

    int *vCenterLine = new int[input_DEM.cols()];
    int *vMaxDistArray = new int[input_DEM.cols()];

    int minVal, maxVal;

    printf("file=%s\n",input_DEM_file.c_str());
    //initialize  output_img, and numSamples
    for (k = 0 ; k < input_DEM.cols(); ++k) {

        minVal = input_DEM.rows();
        maxVal = 0;
        for (l = 0; l < input_DEM.rows(); ++l) {

	  if ( input_DEM(k,l) != noDataDEMVal ) {
             if (l < minVal){
                 minVal = l;
             }
             if (l > maxVal){
                 maxVal = l;
             }
          }
      }
      vCenterLine[k] = (minVal + maxVal)/2;
      vMaxDistArray[k] = maxVal - minVal;
      if (vMaxDistArray[k] < 0){
        vMaxDistArray[k]=0;
      }
      //printf("cl[%d] = %d, maxDist[%d] = %d\n", k, vCenterLine[k], k, vMaxDistArray[k]);
   }



   *r_vMaxDistArray = vMaxDistArray;
   return vCenterLine;
}

float
photometry::ComputeLineWeightsH(Vector2 const& pix,
                                    std::vector<int> const& hCenterLine, std::vector<int> const& hMaxDistArray){

  // We round below, to avoid issues when we are within numerical value
  // to an integer value for pix[1].
  // To do: Need to do interpolation here.

  int pos = (int)round(pix[1]);
  if (pos < 0 || pos >= hMaxDistArray.size() || pos >= hCenterLine.size() ) return 0;
  
  float maxDist = hMaxDistArray[pos]/2.0;
  float center  = hCenterLine[pos];
  float dist    = fabs(pix[0]-center);

  if (maxDist == 0) return 0;

  float a      = -1.0/maxDist;
  float b      = 1.0;
  float weight = std::max(float(0.0), a*dist + b);

  //printf("i = %d, j = %d, weight = %f\n", (int)pix[1], (int)pix[0], weight);
  return weight;
}

float
photometry::ComputeLineWeightsV(Vector2 const& pix,
                                    std::vector<int> const& vCenterLine, std::vector<int> const& vMaxDistArray){

  // See the notes at ComputeLineWeightsH().

  int pos = (int)round(pix[0]);
  if (pos < 0 || pos >= vMaxDistArray.size() || pos >= vCenterLine.size() ) return 0;

  float maxDist = vMaxDistArray[pos]/2.0;
  float center  = vCenterLine[pos];
  float dist    = fabs(pix[1]-center);

  if (maxDist == 0) return 0;

  float a      = -1.0/(float)maxDist;
  float b      = 1.0;
  float weight = std::max(float(0.0), a*dist + b);
  
  //printf("i = %d, j = %d, weight = %f\n", (int)pix[1], (int)pix[0], weight);
  return weight;
}

float
photometry::ComputeLineWeightsHV(Vector2 const& pix, struct ModelParams const& modelParams)
{
  float weightH = ComputeLineWeightsH(pix, modelParams.hCenterLine, modelParams.hMaxDistArray);
  float weightV = ComputeLineWeightsV(pix, modelParams.vCenterLine, modelParams.vMaxDistArray);
  float weight = weightH*weightV;
  return weight;
}


// Saves weights to file.
void 
photometry::SaveWeightsParamsToFile(struct ModelParams const& modelParams)
{

  FILE *fp;
  fp = fopen((char*)(modelParams.weightFilename).c_str(), "w");
  std::cout << "Writing " << modelParams.weightFilename << std::endl;

  // Horizonal DRG
  DiskImageView<PixelMask<PixelGray<uint8> > > drg(modelParams.inputFilename);
  for (int i = 0; i < (int)drg.rows(); i++){
      fprintf(fp, "%d ", modelParams.hCenterLine[i]);
  }
  fprintf(fp, "\n");
  for (int i = 0; i < (int)drg.rows(); i++){
      fprintf(fp, "%d ", modelParams.hMaxDistArray[i]);
  }
  fprintf(fp,"\n");
  
  // Vertical DRG
  for (int i = 0; i < (int)drg.cols(); i++){
      fprintf(fp, "%d ", modelParams.vCenterLine[i]);
  }
  fprintf(fp, "\n");
  for (int i = 0; i < (int)drg.cols(); i++){
      fprintf(fp, "%d ", modelParams.vMaxDistArray[i]);
  }
  fprintf(fp,"\n");

  fclose(fp);
}

void 
photometry::ReadWeightsParamsFromFile(struct ModelParams & modelParams){
  FILE *fp;

  std::cout << "Reading: " << modelParams.weightFilename << std::endl;
  fp = fopen((char*)(modelParams.weightFilename).c_str(), "r");
  if (fp==NULL){
    return;
  }

  // Horizontal DRG
  DiskImageView<PixelMask<PixelGray<uint8> > > drg(modelParams.inputFilename);
  modelParams.hCenterLine.resize(drg.rows());
  modelParams.hMaxDistArray.resize(drg.rows());
  for (int i = 0; i < (int)drg.rows(); i++){
    fscanf(fp, "%d ", &(modelParams.hCenterLine[i]));
  }
  fscanf(fp, "\n");
  for (int i = 0; i < (int)drg.rows(); i++){
    fscanf(fp, "%d ", &(modelParams.hMaxDistArray[i]));
  }
  fscanf(fp,"\n");

  // Vertical DRG
  modelParams.vCenterLine.resize(drg.cols());
  modelParams.vMaxDistArray.resize(drg.cols());
  for (int i = 0; i < (int)drg.cols(); i++){
    fscanf(fp, "%d ", &(modelParams.vCenterLine[i]));
  }
  fscanf(fp, "\n");
  for (int i = 0; i < (int)drg.cols(); i++){
    fscanf(fp, "%d ", &(modelParams.vMaxDistArray[i]));
  }
  fscanf(fp,"\n");

  fclose(fp);
  
}

/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */

#include "itkImageRegionConstIterator.h"
#include "imageHelpers.h"
#include "map"

namespace CU = cascade::util;

int main(int argc, char *argv[])
{
  if (argc < 4)
  {
    std::cerr << "Missing Parameters " << std::endl;
    std::cerr << "Usage: " << argv[0];
    std::cerr << " segmentation atlas [thresh=0] [labelnames]";
    std::cerr << std::endl;
    return EXIT_FAILURE;
  }

  std::string segmentation(argv[1]);
  std::string atlas(argv[2]);
  std::string labelNamesFile;
  float thresh = 0;
  if (argc > 3)
  {
    thresh = atof(argv[3]);
  }
  if (argc > 4)
  {
    labelNamesFile = argv[4];
  }


  const unsigned int ImageDimension = 3;
  typedef unsigned int PixelType;
  typedef double CoordinateRepType;
  const unsigned int SpaceDimension = ImageDimension;

  typedef std::map< PixelType, std::size_t > AtlasMapType;
  typedef std::map< PixelType, std::string > AtlasNameMapType;

  typedef itk::Image< PixelType, ImageDimension > ImageType;
  typedef itk::ImageRegionConstIterator< ImageType > ImageIteratorType;

  ImageType::Pointer segImg = CU::LoadImage< ImageType >(segmentation);
  ImageType::Pointer atlasImg = CU::LoadImage< ImageType >(atlas);

  float voxelSize = 1;
  for(unsigned int i=0;i<ImageDimension;i++)
  {
    voxelSize*= segImg->GetSpacing()[i];
  }

  AtlasNameMapType atlasNameMap;
  if (!labelNamesFile.empty())
  {
    std::ifstream infile(labelNamesFile.c_str());
    std::string line;
    while (std::getline(infile, line))
    {
      /*
       * If the line starts with a digit
       */
      if (line.find_first_of("0123456789") == 0)
      {
        std::istringstream iss(line);
        PixelType atl;
        std::string atlLabel;
        if (!(iss >> atl >> atlLabel)) { continue; } // error
        atlasNameMap[atl] = atlLabel;
      }
    }
  }

  ImageIteratorType segIt(segImg, segImg->GetLargestPossibleRegion());
  ImageIteratorType atlasIt(atlasImg, atlasImg->GetLargestPossibleRegion());

  AtlasMapType atlasMap;
  while (!segIt.IsAtEnd())
  {
    PixelType seg = segIt.Get();
    PixelType atl = atlasIt.Get();
    if (seg > thresh)
    {
      atlasMap[atl] += 1;
    }
    else
    {
      atlasMap[atl] += 0;
    }
    ++segIt;
    ++atlasIt;
  }
  std::string delim=";";
  float total = 0;
  std::cout << "Total";
  for (AtlasMapType::const_iterator it = atlasMap.begin(); it != atlasMap.end(); ++it)
  {
    std::string key = atlasNameMap[it->first];
    if(key.empty())
    {
      std::cout << delim << it->first ;
    }else{
      std::cout << delim << key;
    }
    total += it->second;
  }
  std::cout << std::endl << total*voxelSize;
  for (AtlasMapType::const_iterator it = atlasMap.begin(); it != atlasMap.end(); ++it)
  {
      std::cout << delim << it->second * voxelSize;
  }
  std::cout << std::endl;

  return EXIT_SUCCESS;
}

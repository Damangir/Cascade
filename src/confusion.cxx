/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */

#include "itkImageRegionConstIterator.h"

#include "imageHelpers.h"

namespace CU = cascade::util;

int main(int argc, char *argv[])
{
  if (argc < 3)
  {
    std::cerr << "Missing Parameters " << std::endl;
    std::cerr << "Usage: " << argv[0];
    std::cerr << " img1 img2";
    std::cerr << std::endl;
    return EXIT_FAILURE;
  }

  const unsigned int ImageDimension = 3;
  const unsigned int SpaceDimension = ImageDimension;
  typedef float PixelType;

  typedef itk::Image< PixelType, ImageDimension > ImageType;

  ImageType::Pointer image1 = CU::LoadImage< ImageType >(argv[1]);
  ImageType::Pointer image2 = CU::LoadImage< ImageType >(argv[2]);


  ImageType::RegionType region = image1->GetLargestPossibleRegion();

  itk::ImageRegionConstIterator< ImageType > it1(image1,region);
  itk::ImageRegionConstIterator< ImageType > it2(image2,region);

  size_t tp = 0;
  size_t fp = 0;
  size_t tn = 0;
  size_t fn = 0;

  /*
   * img1 actual
   * img2 predicted
   */
  while (!it1.IsAtEnd())
  {
    if(it1.Get() > 0)
    {
      if(it2.Get() > 0)
      {
        tp++;
      }else{
        fn++;
      }
    }else
    {
      if(it2.Get() > 0)
      {
        fp++;
      }else{
        tn++;
      }
    }
    ++it1;
    ++it2;
  }

  std::cout << tp << " " << fn << " " << fp << " " << tn << std::endl;

  return EXIT_SUCCESS;
}

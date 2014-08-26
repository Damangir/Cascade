/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */

#include "itkChangeLabelImageFilter.h"
#include "imageHelpers.h"

namespace CU = cascade::util;

int main(int argc, char *argv[])
{
  if (argc < 4)
  {
    std::cerr << "Missing Parameters " << std::endl;
    std::cerr << "Usage: " << argv[0];
    std::cerr << " labelImage labelMap relabeledImage";
    std::cerr << std::endl;
    return EXIT_FAILURE;
  }

  std::string labelImage(argv[1]);
  std::string labelMap(argv[2]);
  std::string relabeledImage(argv[3]);

  const unsigned int ImageDimension = 3;
  typedef unsigned int LabelPixelType;
  typedef double CoordinateRepType;
  const unsigned int SpaceDimension = ImageDimension;

  typedef itk::Image< LabelPixelType, ImageDimension > LabelImageType;

  typedef itk::ChangeLabelImageFilter< LabelImageType, LabelImageType > RelabelFilterType;

  RelabelFilterType::Pointer relabeler = RelabelFilterType::New();

  {
    LabelPixelType origLab;
    LabelPixelType newLab;
    std::ifstream file(labelMap.c_str());
    while (file >> origLab >> newLab)
    {
      std::cout << "Convert " << origLab << " to " << newLab << std::endl;
      relabeler->SetChange(origLab, newLab);
    }
  }
  relabeler->SetInput(CU::LoadImage< LabelImageType >(labelImage));
  CU::WriteImage< LabelImageType >(relabeledImage, relabeler->GetOutput());

  return EXIT_SUCCESS;
}

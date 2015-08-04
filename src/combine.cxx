/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */
#include "itkImageUtil.h"

#include "itkBinaryFunctorImageFilter.h"
#include "itkMapSelectorImageFilter.h"
#include "itkNaryFunctorImageFilter.h"

int main(int argc, char *argv[])
{
  if (argc < 4)
  {
    std::cerr << "Missing Parameters " << std::endl;
    std::cerr << "Usage: " << argv[0];
    std::cerr << " map output input1 input2 ...";
    std::cerr << std::endl;
    std::cerr << "Mix inputs to output file according to the map. For pixel ";
    std::cerr << "whose value in map is N, value of inputN will be assigned";
    std::cerr << std::endl;
    return EXIT_FAILURE;
  }

  std::string mapImage(argv[1]);
  std::string output(argv[2]);

  const unsigned int ImageDimension = 3;
  const unsigned int SpaceDimension = ImageDimension;
  typedef float PixelType;
  typedef unsigned char LabelType;

  typedef itk::Image< LabelType, ImageDimension > MapImageType;
  typedef itk::VectorImage< PixelType, ImageDimension > ModelImageType;

  typedef itk::ImageUtil< MapImageType > MapImageUtil;
  MapImageType::Pointer mapImg = MapImageUtil::ReadImage(mapImage);

  typedef itk::MapSelectorImageFilter< MapImageType, ModelImageType > MapSelector;

  MapSelector::Pointer selector = MapSelector::New();
  selector->SetMap(mapImg);
  for (unsigned int i = 3; i < argc; i++)
  {
    selector->SetInput(i - 3, MapImageUtil::ReadImage(argv[i]));
  }

  MapImageUtil::WriteImage(output, selector->GetOutput());

  return EXIT_SUCCESS;
}

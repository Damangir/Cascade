/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */

#include "itkImageUtil.h"

#include "itkBinaryThresholdImageFilter.h"
#include "itkBinaryImageToShapeLabelMapFilter.h"
#include "itkShapeOpeningLabelMapFilter.h"
#include "itkLabelMapToLabelImageFilter.h"

#include "itkStatisticsLabelMapFilter.h"
#include "itkLabelStatisticsImageFilter.h"
#include "itkBinaryImageToStatisticsLabelMapFilter.h"

int main(int argc, char *argv[])
{
  if (argc < 2)
  {
    std::cerr << "Missing Parameters " << std::endl;
    std::cerr << "Usage: " << argv[0];
    std::cerr << " map [threshold=0] [minsize=0] [bridgeradius=0] [binarySeg]";
    std::cerr << std::endl;
    return EXIT_FAILURE;
  }

  std::string map(argv[1]);
  std::string seg = "";
  float thresh = 0;
  float minSize = 0;
  float bridgeRad = 0;
  if (argc > 2) thresh = atof(argv[2]);
  if (argc > 3) minSize = atof(argv[3]);
  if (argc > 4) bridgeRad = atof(argv[4]);
  if (argc > 5) seg = argv[5];

  const unsigned int ImageDimension = 3;
  typedef float PixelType;
  typedef double CoordinateRepType;
  typedef unsigned char SegPixelType;
  const unsigned int SpaceDimension = ImageDimension;

  typedef itk::Image< PixelType, ImageDimension > ImageType;
  typedef itk::Image< unsigned char, ImageDimension > LabelImageType;

  typedef itk::ImageUtil< ImageType > ImageUtil;
  typedef itk::ImageUtil< LabelImageType > LabelImageUtil;

  ImageType::Pointer mapImg = ImageUtil::ReadImage(map);

  float voxelSize = ImageUtil::GetPhysicalPixelSize(mapImg);

  typedef itk::BinaryThresholdImageFilter< ImageType, ImageType > ThresholdType;
  ThresholdType::Pointer threshFilter = ThresholdType::New();
  threshFilter->SetInput(mapImg);
  threshFilter->SetLowerThreshold(thresh);
  threshFilter->SetOutsideValue(0);
  threshFilter->SetInsideValue(1);

  ImageType::Pointer openedMap = ImageUtil::GraftOutput(threshFilter, 0);
  if (bridgeRad > 0)
  {
    openedMap = ImageUtil::Opening(openedMap, bridgeRad, 1);
  }

  // Create a ShapeLabelMap from the image
  typedef itk::BinaryImageToShapeLabelMapFilter< ImageType > BinaryImageToShapeLabelMapFilterType;
  BinaryImageToShapeLabelMapFilterType::Pointer binaryImageToShapeLabelMapFilter =
      BinaryImageToShapeLabelMapFilterType::New();
  binaryImageToShapeLabelMapFilter->SetInput(openedMap);
  binaryImageToShapeLabelMapFilter->SetInputForegroundValue(1);
  binaryImageToShapeLabelMapFilter->Update();

  typedef itk::ShapeOpeningLabelMapFilter<
      BinaryImageToShapeLabelMapFilterType::OutputImageType > ShapeOpeningLabelMapFilterType;
  ShapeOpeningLabelMapFilterType::Pointer shapeOpeningLabelMapFilter =
      ShapeOpeningLabelMapFilterType::New();
  shapeOpeningLabelMapFilter->SetInput(
      binaryImageToShapeLabelMapFilter->GetOutput());
  shapeOpeningLabelMapFilter->SetLambda(minSize);
  shapeOpeningLabelMapFilter->ReverseOrderingOff();
  shapeOpeningLabelMapFilter->SetAttribute(
      ShapeOpeningLabelMapFilterType::LabelObjectType::PHYSICAL_SIZE);
  shapeOpeningLabelMapFilter->Update();

  float totalPhysicalSize = 0;
  for (size_t i = 0;
      i < shapeOpeningLabelMapFilter->GetOutput()->GetNumberOfLabelObjects();
      i++)
  {
    ShapeOpeningLabelMapFilterType::OutputImageType::LabelObjectType* labelObject =
        shapeOpeningLabelMapFilter->GetOutput()->GetNthLabelObject(i);
    totalPhysicalSize += labelObject->GetPhysicalSize();
  }

  if (seg != "")
  {
    // Create a label image
    typedef itk::LabelMapToLabelImageFilter<
        BinaryImageToShapeLabelMapFilterType::OutputImageType, LabelImageType > LabelMapToLabelImageFilterType;
    LabelMapToLabelImageFilterType::Pointer labelMapToLabelImageFilter =
        LabelMapToLabelImageFilterType::New();
    labelMapToLabelImageFilter->SetInput(
        shapeOpeningLabelMapFilter->GetOutput());
    labelMapToLabelImageFilter->Update();

    LabelImageUtil::WriteImage(seg, labelMapToLabelImageFilter->GetOutput());
  }
  std::cout << totalPhysicalSize << ","
  << shapeOpeningLabelMapFilter->GetOutput()->GetNumberOfLabelObjects()
  << std::endl;

  return EXIT_SUCCESS;
}

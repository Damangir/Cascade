/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */
#include "itkImageUtil.h"

#include "itkStatisticsLabelObject.h"
#include "itkStatisticsLabelMapFilter.h"

#include "itkBinaryImageToLabelMapFilter.h"
#include "itkLabelImageToShapeLabelMapFilter.h"
#include "itkLabelMapMaskImageFilter.h"
#include "itkBinaryThresholdImageFilter.h"
#include "itkConnectedComponentImageFilter.h"

#include "itkLabelContourImageFilter.h"

#include "itkLabelStatisticsOpeningImageFilter.h"
#include "itkLabelMapToBinaryImageFilter.h"

#include "itkConfidenceConnectedImageFilter.h"

int main(int argc, char *argv[])
{
  if (argc < 3)
  {
    std::cerr << "Missing Parameters " << std::endl;
    std::cerr << "Usage: " << argv[0];
    std::cerr << " swi microbleeds minRad maxRad minRoundness" << std::endl;
    std::cerr << "swi, microbleeds: Input SWI image; microbleed mask"
              << std::endl;
    std::cerr << "minDiam, maxDiam: Diameter range of the detection"
              << std::endl;
    std::cerr
        << "minRoundness: minimum roundness required for a micro-bleed. "
        << "roundness = SphericalPerimeter / Perimeter 0-1 with 1 roundest"
        << std::endl;
    std::cerr << std::endl;
    return EXIT_FAILURE;
  }

  std::string swiFilename(argv[1]);
  std::string microbleedFilename(argv[2]);
  float minRad = 1.5;
  float maxRad = 6;
  float minRoundness = 0.7;
  if (argc > 3) minRad = atof(argv[3]) / 2;
  if (argc > 4) maxRad = atof(argv[4]) / 2;
  if (argc > 5) minRoundness = atof(argv[5]);

  const unsigned int ImageDimension = 3;
  const unsigned int SpaceDimension = ImageDimension;
  typedef float PixelType;
  typedef unsigned int LabelType;

  const LabelType MicroBleedLabel = static_cast< LabelType >(1);
  const LabelType OthersLabel = static_cast< LabelType >(0);

  typedef itk::Image< PixelType, ImageDimension > ImageType;
  typedef itk::Image< LabelType, ImageDimension > LabelImageType;

  typedef itk::StatisticsLabelObject< LabelType, ImageDimension > StatisticsLabelObjectType;
  typedef StatisticsLabelObjectType::LabelMapType StatisticsLabelMapType;

  typedef itk::ImageUtil< ImageType > ImageUtil;
  typedef itk::ImageUtil< LabelImageType > LabelImageUtil;

  /*
   * Load images
   */
  ImageType::Pointer swiImg = ImageUtil::ReadImage(swiFilename);

  /*
   * Microbleed looks dark on SWI image
   */
  PixelType minP;
  PixelType maxP;

  ImageUtil::ImageExtent(swiImg, minP, maxP, 0.02);

  std::cerr << "Image extent: [" << minP << "," << maxP << "]" << std::endl;

  typedef itk::BinaryThresholdImageFilter< ImageType, LabelImageType > BinaryThresholdImageFilterT;
  BinaryThresholdImageFilterT::Pointer thresholdImageFilter =
      BinaryThresholdImageFilterT::New();
  thresholdImageFilter->SetInput(swiImg);
  thresholdImageFilter->SetUpperThreshold(minP);

  typedef itk::ConfidenceConnectedImageFilter< ImageType, LabelImageType > ConfidenceConnectedFilterType;
  ConfidenceConnectedFilterType::Pointer confidenceConnectedFilter =
      ConfidenceConnectedFilterType::New();
  confidenceConnectedFilter->SetInitialNeighborhoodRadius(0);
  confidenceConnectedFilter->SetMultiplier(3);
  confidenceConnectedFilter->SetNumberOfIterations(3);

  confidenceConnectedFilter->SetInput(swiImg);

  itk::ImageRegionConstIteratorWithIndex< ImageType > imageIterator(
      swiImg, swiImg->GetLargestPossibleRegion());
  while (!imageIterator.IsAtEnd())
  {
    if (imageIterator.Get() < minP)
    {
      confidenceConnectedFilter->AddSeed(imageIterator.GetIndex());
    }
    ++imageIterator;
  }

  std::cerr << " Start region growing." << std::endl;
  LabelImageUtil::WriteImage("possible_mask.nii.gz",
                             confidenceConnectedFilter->GetOutput());

  typedef itk::ConnectedComponentImageFilter< LabelImageType, LabelImageType > ConnectedComponentImageFilterType;

  ConnectedComponentImageFilterType::Pointer connected =
      ConnectedComponentImageFilterType::New();
  connected->SetInput(confidenceConnectedFilter->GetOutput());

  typedef itk::LabelImageToShapeLabelMapFilter< LabelImageType,
      StatisticsLabelMapType > I2LType;
  I2LType::Pointer i2l = I2LType::New();
  i2l->SetInput(connected->GetOutput());
  i2l->Update();

  typedef itk::StatisticsLabelMapFilter< StatisticsLabelMapType, ImageType > ImageStatisticsLabelMapFilterT;

  ImageStatisticsLabelMapFilterT::Pointer imageStatisticsFilter =
      ImageStatisticsLabelMapFilterT::New();

  imageStatisticsFilter->ComputeFeretDiameterOff();
  imageStatisticsFilter->ComputePerimeterOff();
  imageStatisticsFilter->ComputeHistogramOff();
  imageStatisticsFilter->SetInput(i2l->GetOutput());
  imageStatisticsFilter->SetFeatureImage(swiImg);

  imageStatisticsFilter->Update();
  StatisticsLabelMapType::Pointer statLabelMap =
      imageStatisticsFilter->GetOutput();

  for (size_t i = 1; i < statLabelMap->GetNumberOfLabelObjects(); i++)
  {

    StatisticsLabelObjectType::Pointer objMB = statLabelMap->GetLabelObject(i);
    bool toRemove = false;
    std::cerr << "Diameter: " << objMB->GetEquivalentSphericalRadius() * 2;
    std::cerr << " N: " << objMB->GetNumberOfPixels();
    std::cerr << " Roundness: " << objMB->GetRoundness();
    if (objMB->GetEquivalentSphericalRadius() > maxRad)
    {
      toRemove = true;
    }
    if (objMB->GetEquivalentSphericalRadius() < minRad)
    {
      toRemove = true;
    }
    if (objMB->GetRoundness() < minRoundness)
    {
      toRemove = true;
    }

    if (toRemove)
    {
      std::cerr << " -->Removed";
      statLabelMap->RemoveLabel(objMB->GetLabel());
    }
    std::cerr << std::endl;
  }

  typedef itk::LabelMapToBinaryImageFilter< StatisticsLabelMapType,
      LabelImageType > LabelMapToBinaryImageFilterT;
  LabelMapToBinaryImageFilterT::Pointer labelMapToBinaryImage =
      LabelMapToBinaryImageFilterT::New();

  labelMapToBinaryImage->SetInput(statLabelMap);
  labelMapToBinaryImage->SetBackgroundValue(OthersLabel);
  labelMapToBinaryImage->SetForegroundValue(MicroBleedLabel);
  labelMapToBinaryImage->Update();

  LabelImageUtil::WriteImage(microbleedFilename,
                             labelMapToBinaryImage->GetOutput());

  return EXIT_SUCCESS;
}

/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */
#include "itkImageToWeightedHistogramFilter.h"
#include "itkVotingBinaryIterativeHoleFillingImageFilter.h"
#include "itkBinaryFunctorImageFilter.h"
#include "itkLogicOpsFunctors.h"
#include "itkDiscreteGaussianImageFilter.h"

#include "itkBinaryImageToShapeLabelMapFilter.h"
#include "itkShapeOpeningLabelMapFilter.h"
#include "itkLabelMapToLabelImageFilter.h"
#include "itkBinaryThresholdImageFilter.h"

#include "imageHelpers.h"

namespace CU = cascade::util;

int main(int argc, char *argv[])
{
  if (argc < 5)
  {
    std::cerr << "Missing Parameters " << std::endl;
    std::cerr << "Usage: " << argv[0] << std::endl;
    std::cerr << " image brainTissues output percentile [brainTissueLevels = 3]"
              << std::endl;
    std::cerr << "Threshold the image and percentile of brain tissue."
              << std::endl;
    std::cerr << std::endl;
    return EXIT_FAILURE;
  }

  std::string subjectImage(argv[1]);
  std::string brainTissue(argv[2]);
  std::string output(argv[3]);
  float percentile = 0.5;
  int brainTissueLevel = 2;
  if (argc > 4) percentile = atof(argv[4]);
  if (argc > 5) brainTissueLevel = atoi(argv[5]);

  const unsigned int ImageDimension = 3;
  const unsigned int SpaceDimension = ImageDimension;
  typedef float PixelType;
  typedef unsigned char LabelType;
  typedef float Probability;

  typedef itk::Image< PixelType, ImageDimension > ImageType;
  typedef itk::Image< LabelType, ImageDimension > ClassifidImageType;

  typedef itk::Statistics::ImageToWeightedHistogramFilter< ImageType,
      ClassifidImageType > WeightedHistogramType;

  ClassifidImageType::Pointer brainTissueImg =
      CU::LoadImage< ClassifidImageType >(brainTissue);

  ImageType::Pointer subjectImg = CU::LoadImage< ImageType >(subjectImage);

  ClassifidImageType::Pointer BrainTissueMask;

  const unsigned int nComp = subjectImg->GetNumberOfComponentsPerPixel();
  const unsigned int hSize = 100;
  WeightedHistogramType::HistogramSizeType size(nComp);
  size.Fill(hSize);

  typedef itk::BinaryFunctorImageFilter< ClassifidImageType, ClassifidImageType,
      ClassifidImageType,
      itk::Functor::Equal< ClassifidImageType::PixelType,
          ClassifidImageType::PixelType > > EqualClassificationFilterType;

  /*
   * Create total WM and possible Lesion Masks
   */
  {
    EqualClassificationFilterType::Pointer eqFilter =
        EqualClassificationFilterType::New();
    eqFilter->SetInput1(brainTissueImg);
    eqFilter->SetConstant2(brainTissueLevel);
    BrainTissueMask = CU::GraftOutput< EqualClassificationFilterType >(
        eqFilter);
  }
  /*
   * All lesions should be on one side of the WM mode.
   */
  float wmMode;
  float wmlMode;
  float spread;
  float wmlThresh;
  {
    WeightedHistogramType::HistogramType::Pointer lesionDist;

    WeightedHistogramType::Pointer hist = WeightedHistogramType::New();
    hist->SetAutoMinimumMaximum(true);
    hist->SetHistogramSize(size);
    hist->SetInput(subjectImg);
    hist->SetWeightImage(
        CU::MultiplyConstant< ClassifidImageType >(BrainTissueMask, 100).GetPointer());
    hist->Update();

    wmlThresh = hist->GetOutput()->Quantile(
        0, percentile > 0 ? percentile : -percentile);
    std::cout << percentile * 100 << "% percentile of tissue type "
              << brainTissueLevel << " is " << wmlThresh << std::endl;
  }

  typedef itk::BinaryThresholdImageFilter< ImageType, ClassifidImageType > ThresholdType;
  ThresholdType::Pointer thresh = ThresholdType::New();
  thresh->SetInput(subjectImg);
  if (percentile > 0)
  {
    thresh->SetUpperThreshold(wmlThresh);
  }
  else
  {
    thresh->SetLowerThreshold(wmlThresh);
  }

  CU::WriteImage< ClassifidImageType >(output, thresh->GetOutput());

  return EXIT_SUCCESS;
}

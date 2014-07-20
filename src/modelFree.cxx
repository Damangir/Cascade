/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */
#include "itkImageToWeightedHistogramFilter.h"
#include "itkVotingBinaryIterativeHoleFillingImageFilter.h"
#include "itkBinaryFunctorImageFilter.h"
#include "itkLogicOpsFunctors.h"
#include "itkDiscreteGaussianImageFilter.h"

#include "itkBinaryImageToShapeLabelMapFilter.h"
#include "itkShapeOpeningLabelMapFilter.h"
#include "itkLabelMapToLabelImageFilter.h"

#include "imageHelpers.h"

namespace CU = cascade::util;

int main(int argc, char *argv[])
{
  if (argc < 4)
  {
    std::cerr << "Missing Parameters " << std::endl;
    std::cerr << "Usage: " << argv[0];
    std::cerr << " image brainTissues output [variance] [alpha]";
    std::cerr << std::endl;
    return EXIT_FAILURE;
  }

  std::string subjectImage(argv[1]);
  std::string brainTissue(argv[2]);
  std::string output(argv[3]);
  float variance = 2;
  float alpha = 1;
  if (argc > 4) variance = atof(argv[4]);
  if (argc > 5) alpha = atof(argv[5]);

  const unsigned int ImageDimension = 3;
  const unsigned int SpaceDimension = ImageDimension;
  typedef float PixelType;
  typedef unsigned char LabelType;
  typedef float Probability;

  typedef itk::Image< PixelType, ImageDimension > ImageType;
  typedef itk::Image< LabelType, ImageDimension > ClassifidImageType;

  typedef itk::Statistics::ImageToWeightedHistogramFilter< ImageType,
      ClassifidImageType > WeightedHistogramType;

  typedef itk::DiscreteGaussianImageFilter< ImageType, ImageType > PriorSmoothingType;

  ClassifidImageType::Pointer brainTissueImg =
      CU::LoadImage< ClassifidImageType >(brainTissue);

  ImageType::Pointer subjectImg = CU::LoadImage< ImageType >(subjectImage);

  ClassifidImageType::Pointer GMMask;
  ClassifidImageType::Pointer WMMask;
  ClassifidImageType::Pointer LesionMask;
  ClassifidImageType::Pointer TotalWMMask;

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
    eqFilter->SetConstant2(3);
    WMMask = CU::GraftOutput< EqualClassificationFilterType >(eqFilter);
    eqFilter->SetConstant2(4);
    LesionMask = CU::GraftOutput< EqualClassificationFilterType >(eqFilter);
    TotalWMMask = CU::Add< ClassifidImageType, ClassifidImageType >(WMMask,
                                                                    LesionMask);
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
        CU::MultiplyConstant< ClassifidImageType >(WMMask, 100).GetPointer());
    hist->Update();
    wmMode = CU::histogramMode(hist->GetOutput()->Begin(),
                               hist->GetOutput()->End());

    hist->SetWeightImage(
        CU::MultiplyConstant< ClassifidImageType >(LesionMask, 100).GetPointer());
    hist->Update();
    wmlMode = CU::histogramMode(hist->GetOutput()->Begin(),
                                hist->GetOutput()->End());
    hist->SetWeightImage(
        CU::MultiplyConstant< ClassifidImageType >(TotalWMMask, 100).GetPointer());
    hist->Update();

    spread = hist->GetOutput()->Quantile(0, 0.75)
        - hist->GetOutput()->Quantile(0, 0.25);

    wmlThresh = CU::histogramMode(hist->GetOutput()->Begin(),
                                  hist->GetOutput()->End())
                + spread * alpha;
    std::cout << CU::histogramMode(hist->GetOutput()->Begin(),
                                   hist->GetOutput()->End())
    << "+" << spread << "x" << alpha << "=" << wmlThresh << std::endl;
  }
  ClassifidImageType::Pointer LesionProb = CU::MultiplyConstant(
      LesionMask.GetPointer(), 0);

  const unsigned int MaximumLevels = 5;
  for (unsigned int level = 0; level < MaximumLevels; level++)
  {
    typedef itk::DiscreteGaussianImageFilter< ImageType, ImageType > SmoothFilterType;
    SmoothFilterType::Pointer smoother = SmoothFilterType::New();
    smoother->SetInput(
        CU::Mask< ImageType, ClassifidImageType >(subjectImg, TotalWMMask));
    smoother->SetVariance(level * variance);
    ImageType::Pointer levelImg = CU::GraftOutput< SmoothFilterType >(smoother);

    typedef itk::BinaryThresholdImageFilter< ImageType, ClassifidImageType > ThresholdType;
    ThresholdType::Pointer thresh = ThresholdType::New();
    thresh->SetInput(levelImg);
    if (alpha > 0)
    {
      thresh->SetLowerThreshold(wmlThresh);
    }
    else
    {
      thresh->SetUpperThreshold(wmlThresh);
    }
    ClassifidImageType::Pointer LevelLesionMask = CU::Mask< ClassifidImageType,
        ClassifidImageType >(TotalWMMask, thresh->GetOutput());

    LesionProb = CU::Add< ClassifidImageType, ClassifidImageType >(
        LesionProb, LevelLesionMask);
  }

  CU::WriteImage< ClassifidImageType >(output, LesionProb);

  return EXIT_SUCCESS;
}

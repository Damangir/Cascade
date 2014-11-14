/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */
#include "itkFloodFilledImageFunctionConditionalIterator.h"
#include "itkBinaryThresholdImageFunction.h"
#include "itkBinaryFillholeImageFilter.h"
#include "itkDiscreteGaussianImageFilter.h"
#include "itkComposeImageFilter.h"

#include "itkMembershipImageFilter.h"
#include "itkMAPMarkovImageFilter.h"

#include "itkLinearCombinationModelEstimator.h"
#include "itkGaussianMixtureModelComponent.h"
#include "itkImageToHistogramFilter.h"

#include "itkHistogram.h"
#include "imageHelpers.h"

#include <vector>
#include <algorithm>

#include "imageHelpers.h"

namespace CU = cascade::util;

int
main(int argc, char *argv[])
{
  if (argc < 6)
    {
    std::cerr << "Missing Parameters " << std::endl;
    std::cerr << "Usage: " << argv[0];
    std::cerr
        << " image brainMask btOutput Prior1 Prior2 [Prior3 [Prior4 ...]] ";
    std::cerr << std::endl;
    return EXIT_FAILURE;
    }

  ::itk::Object::GetGlobalWarningDisplay();

  const size_t priorArgNumber = 4;
  std::string subjectImage(argv[1]);
  std::string mask(argv[2]);
  std::string btOutput(argv[3]);
  const size_t numberOfClasses = argc - priorArgNumber;

  std::cerr << "Number of classes are " << numberOfClasses << std::endl;

  const int histogramSmoothing = 3;
  const unsigned int ImageDimension = 3;
  const unsigned int SpaceDimension = ImageDimension;
  typedef float PixelType;
  typedef unsigned char LabelType;
  typedef float Probability;

  typedef itk::Image< PixelType, ImageDimension > ImageType;
  typedef itk::VectorImage< PixelType, ImageDimension > VectorImageType;
  typedef itk::Image< Probability, ImageDimension > ProbabilityImageType;
  typedef itk::VectorImage< Probability, ImageDimension > ProbabilityVectorImageType;
  typedef itk::Image< LabelType, ImageDimension > LabelImageType;

  LabelImageType::Pointer maskImg = CU::LoadImage< LabelImageType >(mask);
  ImageType::Pointer originalImage = CU::LoadImage< ImageType >(subjectImage);
  ImageType::Pointer image = CU::Mask< ImageType, LabelImageType >(
      originalImage, maskImg);

  ProbabilityVectorImageType::Pointer priorImage;
    {
    typedef itk::ComposeImageFilter< ProbabilityImageType,
        ProbabilityVectorImageType > ProbabilityComposerType;
    ProbabilityComposerType::Pointer probabilityComposer =
        ProbabilityComposerType::New();
    for (size_t i = 0; i < numberOfClasses; i++)
      {
      probabilityComposer->SetInput(
          i, CU::LoadImage< ProbabilityImageType >(argv[i + priorArgNumber]));
      }
    priorImage = CU::GraftOutput< ProbabilityComposerType >(probabilityComposer,
                                                            0);
    }

  PixelType minValue;
  PixelType maxValue;
  double quantile = 0.0001;
  CU::ImageExtent< ImageType >(image, minValue, maxValue, quantile);

  std::cerr << "(" << minValue << ", " << maxValue << ")" << std::endl;

  typedef itk::Statistics::ImageToHistogramFilter< ImageType > HistogramFilterType;
  HistogramFilterType::Pointer histogramFilter = HistogramFilterType::New();
  HistogramFilterType::HistogramSizeType histSize(1);
  HistogramFilterType::HistogramMeasurementVectorType minBin(1);
  HistogramFilterType::HistogramMeasurementVectorType maxBin(1);
  histSize[0] = 256;
  minBin[0] = minValue;
  maxBin[0] = maxValue;

  histogramFilter->SetHistogramSize(histSize);
  histogramFilter->SetAutoMinimumMaximum(false);
  histogramFilter->SetHistogramBinMinimum(minBin);
  histogramFilter->SetHistogramBinMaximum(maxBin);
  histogramFilter->SetInput(image);
  histogramFilter->Update();
  HistogramFilterType::HistogramPointer originalHist =
      histogramFilter->GetOutput();
  originalHist->SetFrequency(0, 0);

  typedef HistogramFilterType::HistogramType HistogramType;
  typedef itk::Statistics::GaussianMixtureModelComponent< HistogramType > GaussianComponentType;

  typedef itk::Statistics::LinearCombinationModelEstimator< HistogramType,
      GaussianComponentType > LCMEstimator;

  HistogramType::Pointer hist = HistogramType::New();

    {
    hist->Graft(originalHist);

    const double totalFrequency =
        static_cast< double >(hist->GetTotalFrequency());
    const size_t sampleSize = hist->Size();
    typedef std::vector< double > HistType;
    HistType histVector(sampleSize);
    for (size_t i = 0; i < sampleSize; i++)
      {
      histVector[i] = hist->GetFrequency(i);
      }
    for (size_t i = histogramSmoothing; i < sampleSize - histogramSmoothing;
        i++)
      {
      std::vector< double > freqs;
      for (int j = -histogramSmoothing; j <= histogramSmoothing; j++)
        {
        freqs.push_back(histVector[i + j]);
        }
      std::sort(freqs.begin(), freqs.end());
      hist->SetFrequency(i, *(freqs.begin() + histogramSmoothing));
      }
    const double scaleUp = totalFrequency / hist->GetTotalFrequency();
    for (size_t i = histogramSmoothing; i < sampleSize - histogramSmoothing;
        i++)
      {
      hist->SetFrequency(i, hist->GetFrequency(i) * scaleUp);
      }
    }

  LCMEstimator::Pointer estimator = LCMEstimator::New();
  estimator->SetSample(hist);
  estimator->SetNumberOfClasses(numberOfClasses);
  estimator->DebugOff();
  estimator->Update();

  typedef itk::MembershipImageFilter< ImageType,
      LCMEstimator::MembershipFunctionType, Probability,
      ProbabilityVectorImageType > MembershipFilterType;
  typedef itk::MAPMarkovImageFilter< ImageType, LabelImageType, Probability > MAPMarkovFilterType;

  MembershipFilterType::Pointer membershipFilter = MembershipFilterType::New();

  MAPMarkovFilterType::Pointer MAPMarkovFilter = MAPMarkovFilterType::New();

  membershipFilter->SetInput(image);
  for (size_t j = 0; j < numberOfClasses; j++)
    {
    membershipFilter->AddMembershipFunction(estimator->GetNthComponent(j));
    }

  MAPMarkovFilter->SetPriorVectorImage(priorImage);
  MAPMarkovFilter->SetPriorBias(0.1);
  MAPMarkovFilter->SetNumberOfIterations(1);
  MAPMarkovFilter->SetMembershipVectorImage(membershipFilter->GetOutput());
  MAPMarkovFilter->Update();

  LabelImageType::Pointer tts = CU::GraftOutput< MAPMarkovFilterType >(
      MAPMarkovFilter, 0);

  tts = CU::AddConstant< LabelImageType >(tts, 1);
  tts = CU::Mask< LabelImageType, LabelImageType >(tts, maskImg);

  CU::WriteImage< LabelImageType >(btOutput, tts);
  return EXIT_SUCCESS;
}

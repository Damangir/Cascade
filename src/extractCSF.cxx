/*=========================================================================
 *
 *  Copyright Insight Software Consortium
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/
#include "itkImageToWeightedHistogramFilter.h"
#include "itkEmpiricalDensityMembershipFunction.h"
#include "itkIterativeBayesianImageFilter.h"
#include "itkBinaryFillholeImageFilter.h"
#include "itkFloodFilledImageFunctionConditionalIterator.h"
#include "itkBinaryThresholdImageFunction.h"

#include "itkDiscreteGaussianImageFilter.h"
#include "itkComposeImageFilter.h"

#include "imageHelpers.h"

namespace CU = cascade::util;

int main(int argc, char *argv[])
{
  if (argc < 5)
  {
    std::cerr << "Missing Parameters " << std::endl;
    std::cerr << "Usage: " << argv[0];
    std::cerr << " image brainMask csfPrior csfOutput [bias] [nIteration]";
    std::cerr << std::endl;
    return EXIT_FAILURE;
  }

  std::string subjectImage(argv[1]);
  std::string brainMask(argv[2]);
  std::string csfPrior(argv[3]);
  std::string csfOutput(argv[4]);
  float priorBias = 0.3;
  unsigned int numberOfIteration = 1;
  if (argc > 5) priorBias = atof(argv[5]);
  if (argc > 6) numberOfIteration = atof(argv[6]);

  const unsigned int ImageDimension = 3;
  const unsigned int SpaceDimension = ImageDimension;
  typedef float PixelType;
  typedef unsigned char LabelType;
  typedef float Probability;

  typedef itk::Image< PixelType, ImageDimension > ImageType;
  typedef itk::VectorImage< PixelType, ImageDimension > VectorImageType;
  typedef itk::Image< LabelType, ImageDimension > LabelImageType;

  typedef itk::IterativeBayesianImageFilter< VectorImageType, LabelImageType,
      Probability > IBFilterType;

  typedef IBFilterType::PriorImageType PriorImageType;
  typedef IBFilterType::PriorsVectorImageType PriorsVectorImageType;
  typedef IBFilterType::MembershipsVectorImageType MembershipsVectorImageType;
  typedef IBFilterType::ClassifierOutputImageType ClassifidImageType;

  typedef IBFilterType::EmpiricalDistributionMembershipType EmpiricalDistributionMembershipType;
  typedef IBFilterType::WeightedHistogramType WeightedHistogramType;
  typedef IBFilterType::MembershipFilterType MembershipFilterType;

  typedef itk::SubtractImageFilter< PriorImageType > PriorSubFilterType;
  typedef itk::DiscreteGaussianImageFilter< PriorImageType, PriorImageType > PriorSmoothingType;
  typedef itk::ComposeImageFilter< PriorImageType, PriorsVectorImageType > ComposePriorsFilterType;
  typedef itk::ComposeImageFilter< ImageType, VectorImageType > ComposeInputFilterType;

  LabelImageType::Pointer brainMaskImg = CU::LoadImage< LabelImageType >(
      brainMask);

  ComposeInputFilterType::Pointer inputComposer = ComposeInputFilterType::New();
  inputComposer->SetInput(0, CU::LoadImage< ImageType >(subjectImage));
  inputComposer->Update();
  VectorImageType::Pointer subjectImg = inputComposer->GetOutput();

  PriorImageType::Pointer csfPriorImg;
  PriorImageType::Pointer wgPriorImg;

  csfPriorImg = CU::LoadImage< PriorImageType >(csfPrior);

  {
    PriorSubFilterType::Pointer priorSubFilter = PriorSubFilterType::New();
    priorSubFilter->SetConstant1(
        CU::ImageMinimumMaximum< PriorImageType >(csfPriorImg)[1]);
    priorSubFilter->SetInput2(csfPriorImg);
    wgPriorImg = priorSubFilter->GetOutput();
    wgPriorImg->Update();
    wgPriorImg->DisconnectPipeline();
  }

  /*
   * Mask All inputs:
   */
  {
    subjectImg = CU::Mask< VectorImageType, LabelImageType >(subjectImg,
                                                             brainMaskImg);
    csfPriorImg = CU::Mask< PriorImageType, LabelImageType >(csfPriorImg,
                                                             brainMaskImg);
    wgPriorImg = CU::Mask< PriorImageType, LabelImageType >(wgPriorImg,
                                                            brainMaskImg);
  }

  const unsigned int nComp = subjectImg->GetNumberOfComponentsPerPixel();
  const unsigned int hSize = 100;
  WeightedHistogramType::HistogramSizeType size(nComp);
  size.Fill(hSize);

  ClassifidImageType::Pointer CSFMask;

  {
    ComposePriorsFilterType::Pointer composePriorFilter =
        ComposePriorsFilterType::New();
    {
      PriorSmoothingType::Pointer csfPriorSmoothing = PriorSmoothingType::New();
      PriorSmoothingType::Pointer wgPriorSmoothing = PriorSmoothingType::New();
      wgPriorSmoothing->SetInput(wgPriorImg);
      wgPriorSmoothing->SetVariance(10);
      csfPriorSmoothing->SetInput(csfPriorImg);
      csfPriorSmoothing->SetVariance(10);

      composePriorFilter->SetInput(
          0,
          CU::Mask< PriorImageType, LabelImageType >(
              wgPriorSmoothing->GetOutput(), brainMaskImg));
      composePriorFilter->SetInput(
          1,
          CU::Mask< PriorImageType, LabelImageType >(
              csfPriorSmoothing->GetOutput(), brainMaskImg));
      composePriorFilter->Update();
    }

    IBFilterType::Pointer ibFilter = IBFilterType::New();

    // Create WM+GM membership
    {
      WeightedHistogramType::Pointer hist = WeightedHistogramType::New();
      hist->SetWeightImage(wgPriorImg);
      hist->SetAutoMinimumMaximum(true);
      hist->SetHistogramSize(size);
      hist->SetInput(subjectImg);
      hist->Update();
      EmpiricalDistributionMembershipType::Pointer membership =
          EmpiricalDistributionMembershipType::New();
      membership->SetDistribution(hist->GetOutput());
      ibFilter->AddMembershipFunction(membership);
    }
    //Create CSF membership
    {
      WeightedHistogramType::Pointer hist = WeightedHistogramType::New();
      EmpiricalDistributionMembershipType::Pointer membership =
          EmpiricalDistributionMembershipType::New();
      hist->SetWeightImage(csfPriorImg);
      hist->SetAutoMinimumMaximum(true);
      hist->SetHistogramSize(size);
      hist->SetInput(subjectImg);
      hist->Update();
      membership->SetDistribution(hist->GetOutput());
      ibFilter->AddMembershipFunction(membership);
    }
    ibFilter->SetPriorVectorImage(composePriorFilter->GetOutput());
    ibFilter->SetInput(subjectImg);
    ibFilter->SetNumberOfIterations(numberOfIteration);
    ibFilter->SetPriorBias(priorBias);
    ibFilter->Update();
    CSFMask = CU::Mask< ClassifidImageType, LabelImageType >(
        ibFilter->GetOutput(), brainMaskImg);
  }

  CSFMask = CU::Closing< ClassifidImageType >(CSFMask, 1);
  typedef itk::BinaryFillholeImageFilter< ClassifidImageType > FillHoleType;
  FillHoleType::Pointer fillHole = FillHoleType::New();
  fillHole->SetForegroundValue(1);
  fillHole->SetInput(CSFMask);
  CSFMask = CU::GraftOutput< FillHoleType >(fillHole);
  CU::WriteImage(csfOutput, CSFMask.GetPointer());

  return EXIT_SUCCESS;
}

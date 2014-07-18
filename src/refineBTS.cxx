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
#include "itkVotingRelabelImageFilter.h"
#include "itkVotingBinaryIterativeHoleFillingImageFilter.h"
#include "itkVotingBinaryImageFilter.h"
#include "itkBinaryFunctorImageFilter.h"
#include "itkLogicOpsFunctors.h"
#include "itkDiscreteGaussianImageFilter.h"

#include "imageHelpers.h"

namespace CU = cascade::util;

int main(int argc, char *argv[])
{
  if (argc < 4)
  {
    std::cerr << "Missing Parameters " << std::endl;
    std::cerr << "Usage: " << argv[0];
    std::cerr << " image brainTissues newBrainTissue [alpha] [beta]";
    std::cerr << std::endl;
    return EXIT_FAILURE;
  }

  std::string subjectImage(argv[1]);
  std::string brainTissue(argv[2]);
  std::string output(argv[3]);
  float alpha = 1;
  float beta = 0.4;
  if (argc > 4) alpha = atof(argv[4]);
  if (argc > 5) beta = atof(argv[5]);

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

  ClassifidImageType::Pointer CSFMask;
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
  typedef itk::BinaryFunctorImageFilter< ClassifidImageType, ClassifidImageType,
      ClassifidImageType,
      itk::Functor::NotEqual< ClassifidImageType::PixelType,
          ClassifidImageType::PixelType > > NotEqualClassificationFilterType;

  /*
   * Create Masks
   */
  {
    EqualClassificationFilterType::Pointer eqFilter =
        EqualClassificationFilterType::New();
    eqFilter->SetInput1(brainTissueImg);
    eqFilter->SetConstant2(1);
    CSFMask = CU::GraftOutput< EqualClassificationFilterType >(eqFilter);
    eqFilter->SetConstant2(2);
    GMMask = CU::GraftOutput< EqualClassificationFilterType >(eqFilter);
    eqFilter->SetConstant2(3);
    WMMask = CU::GraftOutput< EqualClassificationFilterType >(eqFilter);
    eqFilter->SetConstant2(4);
    LesionMask = CU::GraftOutput< EqualClassificationFilterType >(eqFilter);
    TotalWMMask = CU::Add< ClassifidImageType, ClassifidImageType >(WMMask,
                                                                    LesionMask);
  }

  /*
   * If any part of the GM is similar to lesion relabel it as WM if it is
   * surrounded by WM.
   */
  {
    typedef itk::VotingBinaryIterativeHoleFillingImageFilter< ClassifidImageType > HoleFillingType;
    HoleFillingType::Pointer holeFilling = HoleFillingType::New();
    holeFilling->SetInput(TotalWMMask);
    holeFilling->SetBackgroundValue(0);
    holeFilling->SetForegroundValue(1);
    holeFilling->SetMaximumNumberOfIterations(100);
    holeFilling->SetRadius(
        CU::GetPhysicalRadius< ClassifidImageType >(TotalWMMask, 1));

    holeFilling->Update();
    ClassifidImageType::Pointer GMLMask = CU::Mask< ClassifidImageType,
        ClassifidImageType >(GMMask, holeFilling->GetOutput());

    /*
     * Calculate WML threshold for GM
     */
    WeightedHistogramType::Pointer hist = WeightedHistogramType::New();
    hist->SetAutoMinimumMaximum(true);
    hist->SetHistogramSize(size);
    hist->SetInput(subjectImg);
    hist->SetWeightImage(
        CU::MultiplyConstant< ClassifidImageType >(GMMask, 100).GetPointer());
    hist->Update();
    const float wmMode = CU::histogramMode(hist->GetOutput()->Begin(),
                                           hist->GetOutput()->End());
    const float spread = hist->GetOutput()->Quantile(0, 0.75)
        - hist->GetOutput()->Quantile(0, 0.25);

    const float wmlThresh = wmMode + spread * alpha;

    typedef itk::BinaryThresholdImageFilter< ImageType, ClassifidImageType > ThresholdType;
    ThresholdType::Pointer thresh = ThresholdType::New();
    thresh->SetInput(subjectImg);
    thresh->SetLowerThreshold(wmlThresh);
    ClassifidImageType::Pointer lesionInGM = thresh->GetOutput();
    lesionInGM = CU::Opening< ClassifidImageType >(lesionInGM, 0.5, 1);
    lesionInGM = CU::Mask< ClassifidImageType, ClassifidImageType >(GMLMask,
                                                                    lesionInGM);

    CU::WriteImage< ClassifidImageType >("gmlesion_mask.nii.gz", lesionInGM);

    /*
     * Create a temporary tissue segmentation so that:
     *  # Possible WML in GM is 4
     *  # WM and WML is 3
     *  # GM is 2
     *  # CSF is 1
     */
    ClassifidImageType::Pointer newBTS = CU::MultiplyConstant<
        ClassifidImageType >(TotalWMMask, 3);
    newBTS = CU::Add< ClassifidImageType, ClassifidImageType >(newBTS, CSFMask);
    newBTS = CU::Add< ClassifidImageType, ClassifidImageType >(
        newBTS, CU::MultiplyConstant< ClassifidImageType >(GMMask, 2));
    newBTS = CU::Add< ClassifidImageType, ClassifidImageType >(
        newBTS, CU::MultiplyConstant< ClassifidImageType >(lesionInGM, 2));

    typedef itk::VotingRelabelImageFilter< ClassifidImageType,
        ClassifidImageType > RelabelerFilter;
    RelabelerFilter::Pointer relabler = RelabelerFilter::New();
    relabler->SetRadius(
        CU::GetPhysicalRadius< ClassifidImageType >(LesionMask, 0.5));

    relabler->SetBackgroundValue(4); // Background are doubtful
    relabler->SetForegroundValue(3); // WM vote for doubtful

    relabler->SetBirthThreshold(beta * 100); // Percentage of WM to the rest voxels in order for a voxel to be considered WM
    relabler->SetSurvivalThreshold((1 - beta) * 100); // Percentage of GM to the rest voxels in order for a voxel to be considered GM

    relabler->SetBirthValue(3); // We convert to WM
    relabler->SetUnsurvivedValue(2); // unsurvived set to GM

    unsigned int totalNumberOfChanges = 0;
    unsigned int currentNumberOfIterations = 0;
    unsigned int maximumNumberOfIterations = 200;

    while (true)
    {
      relabler->SetInput(newBTS);
      relabler->Update();
      currentNumberOfIterations++;
      newBTS = CU::GraftOutput< RelabelerFilter >(relabler, 0);
      totalNumberOfChanges += relabler->GetNumberOfPixelsChanged();
      if (relabler->GetNumberOfPixelsChanged() == 0)
      {
        std::cout << "No more pixel to change" << std::endl;
        break;
      }
      else if (currentNumberOfIterations > maximumNumberOfIterations)
      {
        std::cout << "Maximum iteration reached" << std::endl;
        break;
      }
      else
      {
        std::cout << "." << std::flush;
      }
    }
    std::cerr << totalNumberOfChanges << " pixels changed in "
              << currentNumberOfIterations << " iterations." << std::endl;

    EqualClassificationFilterType::Pointer eqFilter =
        EqualClassificationFilterType::New();
    eqFilter->SetInput1(newBTS);
    eqFilter->SetConstant2(4);
    newBTS = CU::Subtract< ClassifidImageType, ClassifidImageType >(
        newBTS, CU::GraftOutput< EqualClassificationFilterType >(eqFilter));

    NotEqualClassificationFilterType::Pointer neqFilter =
        NotEqualClassificationFilterType::New();
    neqFilter->SetInput1(newBTS);
    neqFilter->SetInput2(brainTissueImg);
    LesionMask = CU::GraftOutput< NotEqualClassificationFilterType >(neqFilter);

    brainTissueImg = CU::Add< ClassifidImageType, ClassifidImageType >(
        newBTS, LesionMask);

    typedef itk::VotingBinaryImageFilter< ClassifidImageType, ClassifidImageType > VotingFilterType;
    VotingFilterType::Pointer filter = VotingFilterType::New();
    filter->SetRadius(
        CU::GetPhysicalRadius< ClassifidImageType >(brainTissueImg, 0.5));
    filter->SetInput(brainTissueImg);
    filter->SetForegroundValue(3);
    filter->SetBackgroundValue(2);
    filter->SetSurvivalThreshold(0);
    filter->SetBirthThreshold(
        0.75 * (CU::GetNumberOfPixels(filter->GetRadius()) - 1));
    brainTissueImg = CU::GraftOutput< VotingFilterType >(filter);
  }

  CU::WriteImage< ClassifidImageType >(output, brainTissueImg);

  return EXIT_SUCCESS;
}

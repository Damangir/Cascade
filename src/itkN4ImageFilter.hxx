/*
 * Copyright (C) 2013 Soheil Damangir - All Rights Reserved
 * You may use and distribute, but not modify this code under the terms of the
 * Creative Commons Attribution-NonCommercial-NoDerivs 3.0 Unported License
 * under the following conditions:
 *
 * Attribution: You must attribute the work in the manner specified by the
 * author or licensor (but not in any way that suggests that they endorse you
 * or your use of the work).
 * Noncommercial: You may not use this work for commercial purposes.
 * No Derivative Works: You may not alter, transform, or build upon this
 * work
 *
 * To view a copy of the license, visit
 * http://creativecommons.org/licenses/by-nc-nd/3.0/
 */
#ifndef __itkN4ImageFilter_hxx
#define __itkN4ImageFilter_hxx
#include "itkN4ImageFilter.h"

#include "itkMacro.h"

#include "itkBSplineControlPointImageFilter.h"
#include "itkConstantPadImageFilter.h"
#include "itkExpImageFilter.h"
#include "itkExtractImageFilter.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkImageRegionIterator.h"
#include "itkImageRegionIteratorWithIndex.h"
#include "itkN4BiasFieldCorrectionImageFilter.h"
#include "itkOtsuThresholdImageFilter.h"
#include "itkShrinkImageFilter.h"

#include "itkTimeProbe.h"

namespace itk
{

template< class TInputImage, class TOutputImage >
N4ImageFilter< TInputImage, TOutputImage >::N4ImageFilter()
  {
  m_MinSampleDistance = 5;
  }

template< class TInputImage, class TOutputImage >
void N4ImageFilter< TInputImage, TOutputImage >::PrintSelf(std::ostream & os,
                                                        Indent indent) const
  {
  Superclass::PrintSelf(os, indent);
  os << indent << "Minimum distance for shrinkage: " << m_MinSampleDistance << std::endl;
  }

template< class TInputImage, class TOutputImage >
void N4ImageFilter< TInputImage, TOutputImage >::GenerateData()
  {
  typedef itk::Image< unsigned char, InputImageDimension > MaskImageType;

  const InputImageType* inputImage = this->GetInput();
  typename MaskImageType::Pointer maskImage = NULL;
  typename InputImageType::Pointer weightImage = NULL;

  typedef itk::N4BiasFieldCorrectionImageFilter< InputImageType, MaskImageType,
      OutputImageType > CorrecterType;
  typename CorrecterType::Pointer correcter = CorrecterType::New();

  /**
   * handle the mask image
   */

  if (!maskImage)
    {
    itkDebugMacro("Mask not read.  Creating Otsu mask.");
    typedef itk::OtsuThresholdImageFilter< InputImageType, MaskImageType > ThresholderType;
    typename ThresholderType::Pointer otsu = ThresholderType::New();
    otsu->SetInput(inputImage);
    otsu->SetNumberOfHistogramBins(200);
    otsu->SetInsideValue(0);
    otsu->SetOutsideValue(1);
    maskImage = otsu->GetOutput();
    maskImage->Update();
    maskImage->DisconnectPipeline();
    }

  /**
   * TODO: convergence options
   */

  /**
   * B-spline options -- we place this here to take care of the case where
   * the user wants to specify things in terms of the spline distance.
   */

  bool useSplineDistance = false;
  typename InputImageType::IndexType inputImageIndex =
      inputImage->GetLargestPossibleRegion().GetIndex();
  typename InputImageType::SizeType inputImageSize =
      inputImage->GetLargestPossibleRegion().GetSize();
  typename InputImageType::IndexType maskImageIndex =
      maskImage->GetLargestPossibleRegion().GetIndex();
  typename InputImageType::SizeType maskImageSize =
      maskImage->GetLargestPossibleRegion().GetSize();

  typename InputImageType::PointType newOrigin = inputImage->GetOrigin();

  /*
   * Shrinking the images
   */
  typedef itk::ShrinkImageFilter< InputImageType, InputImageType > ShrinkerType;
  typedef typename ShrinkerType::ShrinkFactorsType ShrinkFactorsType;
  ShrinkFactorsType shrinkage;
  for (unsigned int i=0;i<InputImageDimension;i++){
    shrinkage[i] = m_MinSampleDistance / inputImage->GetSpacing()[i];
    if (shrinkage[i] < 1){
      shrinkage[i] = 1;
    }
  }

  itkDebugMacro(<< shrinkage);

  typename ShrinkerType::Pointer shrinker = ShrinkerType::New();
  shrinker->SetInput(inputImage);
  shrinker->SetShrinkFactors(shrinkage);
  shrinker->Update();

  typedef itk::ShrinkImageFilter< MaskImageType, MaskImageType > MaskShrinkerType;
  typename MaskShrinkerType::Pointer maskshrinker = MaskShrinkerType::New();
  maskshrinker->SetInput(maskImage);
  maskshrinker->SetShrinkFactors(shrinkage);
  maskshrinker->Update();

  correcter->SetInput(shrinker->GetOutput());
  correcter->SetMaskImage(maskshrinker->GetOutput());

  typedef itk::ShrinkImageFilter< InputImageType, InputImageType > WeightShrinkerType;
  typename WeightShrinkerType::Pointer weightshrinker =
      WeightShrinkerType::New();
  if (weightImage)
    {
    weightshrinker->SetInput(weightImage);
    weightshrinker->SetShrinkFactors(shrinkage);
    weightshrinker->Update();

    correcter->SetConfidenceImage(weightshrinker->GetOutput());
    }

  /**
   * TODO: histogram sharpening options
   * correcter->SetBiasFieldFullWidthAtHalfMaximum();
   * correcter->SetWeinerFilterNoise();
   * correcter->SetNumberOfHistogramBins();
   */

  correcter->Update();

  /**
   * Reconstruct the bias field at full image resolution.  Divide
   * the original input image by the bias field to get the final
   * corrected image.
   */
  typedef itk::BSplineControlPointImageFilter<
      typename CorrecterType::BiasFieldControlPointLatticeType,
      typename CorrecterType::ScalarImageType > BSplinerType;
  typename BSplinerType::Pointer bspliner = BSplinerType::New();
  bspliner->SetInput(correcter->GetLogBiasFieldControlPointLattice());
  bspliner->SetSplineOrder(correcter->GetSplineOrder());
  bspliner->SetSize(inputImage->GetLargestPossibleRegion().GetSize());
  bspliner->SetOrigin(newOrigin);
  bspliner->SetDirection(inputImage->GetDirection());
  bspliner->SetSpacing(inputImage->GetSpacing());
  bspliner->Update();

  typename InputImageType::Pointer logField = InputImageType::New();
  logField->SetOrigin(inputImage->GetOrigin());
  logField->SetSpacing(inputImage->GetSpacing());
  logField->SetRegions(inputImage->GetLargestPossibleRegion());
  logField->SetDirection(inputImage->GetDirection());
  logField->Allocate();

  itk::ImageRegionIterator< typename CorrecterType::ScalarImageType > ItB(
      bspliner->GetOutput(), bspliner->GetOutput()->GetLargestPossibleRegion());
  itk::ImageRegionIterator< InputImageType > ItF(
      logField, logField->GetLargestPossibleRegion());
  for (ItB.GoToBegin(), ItF.GoToBegin(); !ItB.IsAtEnd(); ++ItB, ++ItF)
    {
    ItF.Set(ItB.Get()[0]);
    }

  typedef itk::ExpImageFilter< InputImageType, InputImageType > ExpFilterType;
  typename ExpFilterType::Pointer expFilter = ExpFilterType::New();
  expFilter->SetInput(logField);
  expFilter->Update();

  typedef itk::DivideImageFilter< InputImageType, InputImageType, InputImageType > DividerType;
  typename DividerType::Pointer divider = DividerType::New();
  divider->SetInput1(inputImage);
  divider->SetInput2(expFilter->GetOutput());
  divider->Update();

  typename InputImageType::RegionType inputRegion;
  inputRegion.SetIndex(inputImageIndex);
  inputRegion.SetSize(inputImageSize);

  typedef itk::ExtractImageFilter< InputImageType, InputImageType > CropperType;
  typename CropperType::Pointer cropper = CropperType::New();
  cropper->SetInput(divider->GetOutput());
  cropper->SetExtractionRegion(inputRegion);
  cropper->Update();

  typename CropperType::Pointer biasFieldCropper = CropperType::New();
  biasFieldCropper->SetInput(expFilter->GetOutput());
  biasFieldCropper->SetExtractionRegion(inputRegion);
  biasFieldCropper->Update();

  this->GraftNthOutput(0, cropper->GetOutput());
  //this->GraftNthOutput(1, biasFieldCropper->GetOutput());
  }

} // end namespace itk

#endif

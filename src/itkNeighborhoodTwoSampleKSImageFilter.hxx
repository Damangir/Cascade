/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */
#ifndef __itkNeighborhoodTwoSampleKSImageFilter_hxx
#define __itkNeighborhoodTwoSampleKSImageFilter_hxx
#include "itkNeighborhoodTwoSampleKSImageFilter.h"

#include "itkConstNeighborhoodIterator.h"
#include "itkNeighborhoodInnerProduct.h"
#include "itkImageRegionIterator.h"
#include "itkNeighborhoodAlgorithm.h"
#include "itkOffset.h"
#include "itkProgressReporter.h"

#include <vector>


namespace itk
{
template< typename TInputImage, typename TReferenceImage, typename ProbabilityPrecision,typename LabelType>
NeighborhoodTwoSampleKSImageFilter< TInputImage, TReferenceImage, ProbabilityPrecision, LabelType>::NeighborhoodTwoSampleKSImageFilter()
{
  m_KS = KSType::New();
  m_KS->SortedReferenceOff();
}

template< typename TInputImage, typename TReferenceImage, typename ProbabilityPrecision,typename LabelType>
void NeighborhoodTwoSampleKSImageFilter< TInputImage, TReferenceImage, ProbabilityPrecision, LabelType>::ThreadedGenerateData(
    const OutputImageRegionType & outputRegionForThread, ThreadIdType threadId)
{
  typename OutputImageType::Pointer output = this->GetOutput();
  typename InputImageType::ConstPointer input = this->GetInput();
  typename ReferenceImageType::ConstPointer referece = this->GetReference();
  typename LabelImageType::ConstPointer mask = this->GetMask();

  NeighborhoodAlgorithm::ImageBoundaryFacesCalculator< InputImageType > bC;
  typename NeighborhoodAlgorithm::ImageBoundaryFacesCalculator< InputImageType >::FaceListType faceList =
      bC(input, outputRegionForThread, this->GetRadius());

  ProgressReporter progress(this, threadId,
                            outputRegionForThread.GetNumberOfPixels());

  ZeroFluxNeumannBoundaryCondition< InputImageType > nbcInput;
  ZeroFluxNeumannBoundaryCondition< LabelImageType > nbcLabel;
  ZeroFluxNeumannBoundaryCondition< ReferenceImageType > nbcRef;

  DistributionType pixels;
  DistributionType refDist;
  size_t len = referece->GetNumberOfComponentsPerPixel();
  for (typename NeighborhoodAlgorithm::ImageBoundaryFacesCalculator<
      InputImageType >::FaceListType::iterator fit = faceList.begin();
      fit != faceList.end(); ++fit)
  {
    ImageRegionIterator< OutputImageType > it = ImageRegionIterator<
        OutputImageType >(output, *fit);

    ConstNeighborhoodIterator< InputImageType > bit = ConstNeighborhoodIterator<
        InputImageType >(this->GetRadius(), input, *fit);
    bit.OverrideBoundaryCondition(&nbcInput);
    bit.GoToBegin();

    ConstNeighborhoodIterator< ReferenceImageType > rit = ConstNeighborhoodIterator<
        ReferenceImageType >(this->GetRadius(), referece, *fit);
    rit.OverrideBoundaryCondition(&nbcRef);
    rit.GoToBegin();

    ConstNeighborhoodIterator< LabelImageType > maskIt = ConstNeighborhoodIterator<
        LabelImageType >(this->GetRadius(), mask, *fit);
    maskIt.OverrideBoundaryCondition(&nbcLabel);
    maskIt.GoToBegin();

    const unsigned int neighborhoodSize = bit.Size();
    while (!bit.IsAtEnd())
    {
      if (maskIt.GetCenterPixel() > itk::NumericTraits< LabelType >::Zero)
      {
        pixels.clear();
        refDist.clear();
        for (size_t i = 0; i < neighborhoodSize; i++)
        {
          if (maskIt.GetPixel(i) > itk::NumericTraits< LabelType >::Zero)
          {
            pixels.push_back(bit.GetPixel(i));
            for(size_t j=0;j<len;j++)
            {
              ReferenceImageInternalPixelType r = rit.GetPixel(i)[j];
              if (r != itk::NumericTraits< ReferenceImageInternalPixelType >::Zero)
              {
                refDist.push_back(r);
              }
            }
          }
        }
        const double st = m_KS->Evaluate(PairDistribution(refDist, pixels));
        it.Set(static_cast< OutputPixelType >(st));
      }
      else
      {
        it.Set(itk::NumericTraits< OutputPixelType >::ZeroValue());
      }

      ++maskIt;
      ++bit;
      ++it;
      ++rit;
      progress.CompletedPixel();
    }
  }
}
} // end namespace itk

#endif

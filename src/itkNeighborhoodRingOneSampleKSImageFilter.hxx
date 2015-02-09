/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */
#ifndef __itkNeighborhoodRingOneSampleKSImageFilter_hxx
#define __itkNeighborhoodRingOneSampleKSImageFilter_hxx
#include "itkNeighborhoodRingOneSampleKSImageFilter.h"

#include "itkConstNeighborhoodIterator.h"
#include "itkNeighborhoodInnerProduct.h"
#include "itkImageRegionIterator.h"
#include "itkNeighborhoodAlgorithm.h"
#include "itkOffset.h"
#include "itkProgressReporter.h"
#include <vector>
#include <algorithm>

namespace itk
{
template< typename TInputImage, typename ProbabilityPrecision,
    typename LabelType >
NeighborhoodRingOneSampleKSImageFilter< TInputImage, ProbabilityPrecision,
    LabelType >::NeighborhoodRingOneSampleKSImageFilter()
{
  m_KS = KSType::New();
  m_KS->SortedReferenceOn();
}

template< typename TInputImage, typename ProbabilityPrecision,
    typename LabelType >
void NeighborhoodRingOneSampleKSImageFilter< TInputImage, ProbabilityPrecision,
    LabelType >::ThreadedGenerateData(
    const OutputImageRegionType & outputRegionForThread, ThreadIdType threadId)
{
  typename OutputImageType::Pointer output = this->GetOutput();
  typename InputImageType::ConstPointer input = this->GetInput();
  typename LabelImageType::ConstPointer mask = this->GetMask();

  NeighborhoodAlgorithm::ImageBoundaryFacesCalculator< InputImageType > bC;
  typename NeighborhoodAlgorithm::ImageBoundaryFacesCalculator< InputImageType >::FaceListType faceList =
      bC(input, outputRegionForThread, this->GetRadius());

  ProgressReporter progress(this, threadId,
                            outputRegionForThread.GetNumberOfPixels());

  ZeroFluxNeumannBoundaryCondition< InputImageType > nbcInput;
  ZeroFluxNeumannBoundaryCondition< LabelImageType > nbcLabel;
  DistributionType outerPixels;
  DistributionType innerPixels;

  typedef typename ConstNeighborhoodIterator< InputImageType >::IndexType IndexType;

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

    ConstNeighborhoodIterator< LabelImageType > maskIt =
        ConstNeighborhoodIterator< LabelImageType >(this->GetRadius(), mask,
                                                    *fit);
    maskIt.OverrideBoundaryCondition(&nbcLabel);
    maskIt.GoToBegin();

    const unsigned int neighborhoodSize = bit.Size();
    while (!bit.IsAtEnd())
    {
      if (maskIt.GetCenterPixel() > itk::NumericTraits< LabelType >::Zero)
      {
        innerPixels.clear();
        outerPixels.clear();
        IndexType centerIndex = bit.GetIndex(neighborhoodSize / 2);
        for (size_t i = 0; i < neighborhoodSize; i++)
        {
          if (maskIt.GetPixel(i) > itk::NumericTraits< LabelType >::Zero)
          {
            typename IndexType::OffsetType currentOffset = bit.GetIndex(i)
                - centerIndex;
            bool outside = false;
            for (size_t j = 0; j < InputImageDimension; j++)
            {
              if (vnl_math_abs(currentOffset[j]) > m_InnerRadius[j])
              {
                outside = true;
                break;
              }
            }
            if (outside)
            {
              outerPixels.push_back(bit.GetPixel(i));
            }
            else
            {
              innerPixels.push_back(bit.GetPixel(i));
            }
          }
        }
        const double st = m_KS->Evaluate(
            PairDistribution(outerPixels, innerPixels));
        it.Set(static_cast< OutputPixelType >(st));
      }
      else
      {
        it.Set(itk::NumericTraits< OutputPixelType >::ZeroValue());
      }

      ++maskIt;
      ++bit;
      ++it;
      progress.CompletedPixel();
    }
  }
}
} // end namespace itk

#endif

/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */
#ifndef __itkNeighborhoodOneSampleKSImageFilter_hxx
#define __itkNeighborhoodOneSampleKSImageFilter_hxx
#include "itkNeighborhoodOneSampleKSImageFilter.h"

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
template< typename TInputImage, typename ProbabilityPrecision,typename LabelType>
NeighborhoodOneSampleKSImageFilter< TInputImage, ProbabilityPrecision, LabelType>::NeighborhoodOneSampleKSImageFilter()
{
  m_KS = KSType::New();
  m_KS->SortedReferenceOn();
}

template< typename TInputImage, typename ProbabilityPrecision,typename LabelType>
void NeighborhoodOneSampleKSImageFilter< TInputImage, ProbabilityPrecision, LabelType>::ThreadedGenerateData(
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
  DistributionType pixels;

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
        for (size_t i = 0; i < neighborhoodSize; i++)
        {
          if (maskIt.GetPixel(i) > itk::NumericTraits< LabelType >::Zero)
          {
            pixels.push_back(bit.GetPixel(i));
          }
        }
        const double st = m_KS->Evaluate(PairDistribution(m_RefrenceDistribution, pixels));
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

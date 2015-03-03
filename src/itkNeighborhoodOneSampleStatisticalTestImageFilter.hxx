/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */
#ifndef __itkNeighborhoodOneSampleStatisticalTestImageFilter_hxx
#define __itkNeighborhoodOneSampleStatisticalTestImageFilter_hxx
#include "itkNeighborhoodOneSampleStatisticalTestImageFilter.h"

#include "itkConstNeighborhoodIterator.h"
#include "itkNeighborhoodInnerProduct.h"
#include "itkImageRegionIterator.h"
#include "itkNeighborhoodAlgorithm.h"
#include "itkOffset.h"
#include "itkProgressReporter.h"

namespace itk
{
template< typename TInputImage, typename TOutputImage, typename TReferenceSample >
NeighborhoodOneSampleStatisticalTestImageFilter< TInputImage, TOutputImage,
    TReferenceSample >::NeighborhoodOneSampleStatisticalTestImageFilter()
{
  m_Statistics = ITK_NULLPTR;
  m_BackgroundPixel = NumericTraits< InputPixelType >::ZeroValue();
}

template< typename TInputImage, typename TOutputImage, typename TReferenceSample >
void NeighborhoodOneSampleStatisticalTestImageFilter< TInputImage, TOutputImage,
    TReferenceSample >::ThreadedGenerateData(
    const OutputImageRegionType & outputRegionForThread, ThreadIdType threadId)
{
  typename InputImageType::ConstPointer input = this->GetInput();
  typename OutputImageType::Pointer output = this->GetOutput();

  NeighborhoodAlgorithm::ImageBoundaryFacesCalculator< InputImageType > bC;
  typename NeighborhoodAlgorithm::ImageBoundaryFacesCalculator< InputImageType >::FaceListType faceList =
      bC(input, outputRegionForThread, this->GetRadius());

  ProgressReporter progress(this, threadId,
                            outputRegionForThread.GetNumberOfPixels());

  ZeroFluxNeumannBoundaryCondition< InputImageType > nbcInput;
  typename InternalSampleType::Pointer pixels = InternalSampleType::New();
  pixels->SetMeasurementVectorSize(input->GetNumberOfComponentsPerPixel());
  MeasurementVectorType mv(input->GetNumberOfComponentsPerPixel());

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

    const unsigned int neighborhoodSize = bit.Size();
    while (!bit.IsAtEnd())
    {
      if (bit.GetCenterPixel() != m_BackgroundPixel)
      {
        pixels->Clear();
        for (size_t i = 0; i < neighborhoodSize; i++)
        {
          const InputPixelType & p = bit.GetPixel(i);
          if (p != m_BackgroundPixel)
          {
            NumericTraits< InputPixelType >::AssignToArray(p, mv);
            pixels->PushBack(mv);
          }
        }
        const OutputPixelType st = m_Statistics->Evaluate(m_RefrenceSample.GetPointer(), pixels.GetPointer());
        it.Set(st);
      }
      else
      {
        it.Set(itk::NumericTraits< OutputPixelType >::ZeroValue());
      }

      ++bit;
      ++it;
      progress.CompletedPixel();
    }
  }

}
} // end namespace itk

#endif

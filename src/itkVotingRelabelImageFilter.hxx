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
#ifndef __itkVotingRelabelImageFilter_hxx
#define __itkVotingRelabelImageFilter_hxx
#include "itkVotingRelabelImageFilter.h"

#include "itkConstNeighborhoodIterator.h"
#include "itkNeighborhoodInnerProduct.h"
#include "itkImageRegionIterator.h"
#include "itkNeighborhoodAlgorithm.h"
#include "itkProgressReporter.h"

#include <vector>
#include <algorithm>

namespace itk
{
template< typename TInputImage, typename TOutputImage >
VotingRelabelImageFilter< TInputImage, TOutputImage >::VotingRelabelImageFilter()
{
  this->m_NumberOfPixelsChanged = 0;
}

template< typename TInputImage, typename TOutputImage >
void VotingRelabelImageFilter< TInputImage, TOutputImage >::BeforeThreadedGenerateData()
{
  this->m_NumberOfPixelsChanged = 0;

  unsigned int numberOfThreads = this->GetNumberOfThreads();
  this->m_Count.SetSize(numberOfThreads);
  for (unsigned int i = 0; i < numberOfThreads; i++)
  {
    this->m_Count[i] = 0;
  }
}

template< typename TInputImage, typename TOutputImage >
void VotingRelabelImageFilter< TInputImage, TOutputImage >::ThreadedGenerateData(
    const OutputImageRegionType & outputRegionForThread, ThreadIdType threadId)
{
  ZeroFluxNeumannBoundaryCondition< InputImageType > nbc;

  ConstNeighborhoodIterator< InputImageType > bit;
  ImageRegionIterator< OutputImageType > it;

  // Allocate output
  typename OutputImageType::Pointer output = this->GetOutput();
  typename InputImageType::ConstPointer input = this->GetInput();

  // Find the data-set boundary "faces"
  typename NeighborhoodAlgorithm::ImageBoundaryFacesCalculator< InputImageType >::FaceListType faceList;
  NeighborhoodAlgorithm::ImageBoundaryFacesCalculator< InputImageType > bC;
  faceList = bC(input, outputRegionForThread, this->GetRadius());

  typename NeighborhoodAlgorithm::ImageBoundaryFacesCalculator< InputImageType >::FaceListType::iterator fit;

  unsigned int numberOfPixelsChanged = 0;

  ProgressReporter progress(this, threadId,
                            outputRegionForThread.GetNumberOfPixels());

  // Process each of the boundary faces.  These are N-d regions which border
  // the edge of the buffer.
  for (fit = faceList.begin(); fit != faceList.end(); ++fit)
  {
    bit = ConstNeighborhoodIterator< InputImageType >(this->GetRadius(), input,
                                                      *fit);
    it = ImageRegionIterator< OutputImageType >(output, *fit);
    bit.OverrideBoundaryCondition(&nbc);
    bit.GoToBegin();

    unsigned int neighborhoodSize = bit.Size();

    while (!bit.IsAtEnd())
    {
      const InputPixelType inpixel = bit.GetCenterPixel();
      // Unless the birth or survival rate is meet the pixel will be
      // the same value
      it.Set(static_cast< OutputPixelType >(inpixel));

      if (inpixel == this->GetBackgroundValue())
      {
        // count the pixels ON in the neighborhood
        unsigned int countO = 0;
        unsigned int countF = 0;
        unsigned int countB = 0;
        for (unsigned int i = 0; i < neighborhoodSize; ++i)
        {
          InputPixelType value = bit.GetPixel(i);
          if (value == this->GetForegroundValue())
          {
            countF++;
          }
          else if (value == this->GetBackgroundValue())
          {
            countB++;
          }
          else
          {
            countO++;
          }
        }

        if (countF*100 >= this->GetBirthThreshold()*(countO+countB))
        {
          it.Set(static_cast< OutputPixelType >(m_BirthValue));
          numberOfPixelsChanged++;
        }
        if (countO*100 >= this->GetSurvivalThreshold()*(countF+countB))
        {
          it.Set(static_cast< OutputPixelType >(m_UnsurvivedValue));
          numberOfPixelsChanged++;
        }
      }

      ++bit;
      ++it;
      progress.CompletedPixel();
    }
  }
  this->m_Count[threadId] = numberOfPixelsChanged;
}

template< typename TInputImage, typename TOutputImage >
void VotingRelabelImageFilter< TInputImage, TOutputImage >::AfterThreadedGenerateData()
{
  this->m_NumberOfPixelsChanged = NumericTraits< SizeValueType >::Zero;

  unsigned int numberOfThreads = this->GetNumberOfThreads();
  this->m_Count.SetSize(numberOfThreads);
  for (unsigned int t = 0; t < numberOfThreads; t++)
  {
    this->m_NumberOfPixelsChanged += this->m_Count[t];
  }
}

/**
 * Standard "PrintSelf" method
 */
template< typename TInputImage, typename TOutput >
void VotingRelabelImageFilter< TInputImage, TOutput >::PrintSelf(
    std::ostream & os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);
}
} // end namespace itk

#endif

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
#ifndef __itkGibbsMarkovEnergyImageFilter_hxx
#define __itkGibbsMarkovEnergyImageFilter_hxx
#include "itkGibbsMarkovEnergyImageFilter.h"

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
template< typename TInputImage, typename TProbabilityPrecision,
    typename TOutputImage >
GibbsMarkovEnergyImageFilter< TInputImage, TProbabilityPrecision, TOutputImage >::GibbsMarkovEnergyImageFilter()
{
  m_ClassIDs = InputClassContainerType::New();
  m_ClassIDs->Initialize();
}

template< typename TInputImage, typename TProbabilityPrecision,
    typename TOutputImage >
void GibbsMarkovEnergyImageFilter< TInputImage, TProbabilityPrecision,
    TOutputImage >::AddClass(const InputPixelType& p)
{
  m_ClassIDs->InsertElement(GetNumberOfClasses(), p);
}

template< typename TInputImage, typename TProbabilityPrecision,
    typename TOutputImage >
void GibbsMarkovEnergyImageFilter< TInputImage, TProbabilityPrecision,
    TOutputImage >::GenerateOutputInformation()
{
  this->Superclass::GenerateOutputInformation();
  TOutputImage *output = this->GetOutput();
  output->SetNumberOfComponentsPerPixel(this->GetNumberOfClasses());
}

template< typename TInputImage, typename TProbabilityPrecision,
    typename TOutputImage >
void GibbsMarkovEnergyImageFilter< TInputImage, TProbabilityPrecision,
    TOutputImage >::ClearClasses()
{
  m_ClassIDs->Initialize();
}

template< typename TInputImage, typename TProbabilityPrecision,
    typename TOutputImage >
void GibbsMarkovEnergyImageFilter< TInputImage, TProbabilityPrecision,
    TOutputImage >::ThreadedGenerateData(
    const OutputImageRegionType & outputRegionForThread, ThreadIdType threadId)
{
  // Allocate output
  typename OutputImageType::Pointer output = this->GetOutput();
  typename InputImageType::ConstPointer input = this->GetInput();

  // Find the data-set boundary "faces"
  NeighborhoodAlgorithm::ImageBoundaryFacesCalculator< InputImageType > bC;
  typename NeighborhoodAlgorithm::ImageBoundaryFacesCalculator< InputImageType >::FaceListType faceList =
      bC(input, outputRegionForThread, this->GetRadius());

  // support progress methods/callbacks
  ProgressReporter progress(this, threadId,
                            outputRegionForThread.GetNumberOfPixels());

  ZeroFluxNeumannBoundaryCondition< InputImageType > nbc;

  OutputPixelType outpix;
  NumericTraits< OutputPixelType >::SetLength(outpix,
                                              this->GetNumberOfClasses());

  ClassIteratorType clsIt = m_ClassIDs->Begin();
  ClassIteratorType clsEnd = m_ClassIDs->End();

  // Process each of the boundary faces.  These are N-d regions which border
  // the edge of the buffer.
  for (typename NeighborhoodAlgorithm::ImageBoundaryFacesCalculator<
      InputImageType >::FaceListType::iterator fit = faceList.begin();
      fit != faceList.end(); ++fit)
  {
    ImageRegionIterator< OutputImageType > it = ImageRegionIterator<
        OutputImageType >(output, *fit);

    ConstNeighborhoodIterator< InputImageType > bit = ConstNeighborhoodIterator<
        InputImageType >(this->GetRadius(), input, *fit);
    bit.OverrideBoundaryCondition(&nbc);
    bit.GoToBegin();
    const unsigned int neighborhoodSize = bit.Size();
    TProbabilityPrecision bitInc = 1.0
        / static_cast< TProbabilityPrecision >(neighborhoodSize-1);
    while (!bit.IsAtEnd())
    {
      // collect all the pixels in the neighborhood, note that we use
      // GetPixel on the NeighborhoodIterator to honor the boundary conditions

      for (clsIt = m_ClassIDs->Begin(); clsIt != clsEnd; ++clsIt)
      {
        outpix[clsIt.Index()] = 0;
        if (bit.GetCenterPixel() == clsIt.Value())
        {
          outpix[clsIt.Index()] = -bitInc;
        }
      }

      for (unsigned int i = 0; i < neighborhoodSize; ++i)
      {
        for (clsIt = m_ClassIDs->Begin(); clsIt != clsEnd; ++clsIt)
        {
          if (bit.GetPixel(i) == clsIt.Value())
          {
            outpix[clsIt.Index()] += bitInc;
          }
        }
      }

      /*
      for (clsIt = m_ClassIDs->Begin(); clsIt != clsEnd; ++clsIt)
      {
        outpix[clsIt.Index()] = exp(outpix[clsIt.Index()]);
      }
      */

      bit.GetCenterPixel();
      it.Set(outpix);

      ++bit;
      ++it;
      progress.CompletedPixel();
    }
  }
}
} // end namespace itk

#endif

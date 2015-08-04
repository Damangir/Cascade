/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */
#ifndef __itkMapSelectorImageFilter_hxx
#define __itkMapSelectorImageFilter_hxx

#include "itkMapSelectorImageFilter.h"
#include "itkImageRegionConstIterator.h"
#include "itkImageRegionIterator.h"
#include "itkProgressReporter.h"

namespace itk
{
/**
 * Initialize new instance
 */
template< typename TMap, typename TImage >
MapSelectorImageFilter< TMap, TImage >::MapSelectorImageFilter()
{
}

/**
 * Print out a description of self
 *
 */
template< typename TMap, typename TImage >
void MapSelectorImageFilter< TMap, TImage >::PrintSelf(std::ostream & os,
                                                       Indent indent) const
{
  Superclass::PrintSelf(os, indent);
}

template< typename TMap, typename TImage >
void MapSelectorImageFilter< TMap, TImage >::SetMap(
    const MapImageType *mapImage)
{
  ProcessObject::SetInput("MapImage", const_cast< MapImageType * >(mapImage));
}
template< typename TMap, typename TImage >
const typename MapSelectorImageFilter< TMap, TImage >::MapImageType *
MapSelectorImageFilter< TMap, TImage >::GetMap()
{
  return static_cast< const MapImageType * >(ProcessObject::GetInput("MapImage"));
}

/**
 * ThreadedGenerateData
 */
template< typename TMap, typename TImage >
void MapSelectorImageFilter< TMap, TImage >::ThreadedGenerateData(
    const ImageRegionType & outputRegionForThread, ThreadIdType threadId)
{
  const unsigned int numberOfInputImages =
      static_cast< unsigned int >(this->GetNumberOfIndexedInputs());

  typedef ImageRegionConstIterator< InputImageType > ImageRegionConstIteratorType;
  typedef ImageRegionConstIterator< MapImageType >   MapRegionConstIteratorType;
  std::vector< ImageRegionConstIteratorType * > inputItrVector;
  inputItrVector.reserve(numberOfInputImages);

  // support progress methods/callbacks.
  // count the number of inputs that are non-null
  for (unsigned int i = 0; i < numberOfInputImages; ++i)
  {
    if (!ProcessObject::GetInput(i))
    {
      return;
    }
  }

  for (unsigned int i = 0; i < numberOfInputImages; ++i)
  {
    InputImageConstPointer inputPtr =
        dynamic_cast< InputImageType * >(ProcessObject::GetInput(i));
    inputItrVector.push_back(
        new ImageRegionConstIteratorType(inputPtr, outputRegionForThread));
  }

  ProgressReporter progress(this, threadId,
                            outputRegionForThread.GetNumberOfPixels());

  OutputImagePointer outputPtr = this->GetOutput(0);
  ImageRegionIterator< OutputImageType > outputIt(outputPtr,
                                               outputRegionForThread);
  MapRegionConstIteratorType mapIt(this->GetMap(), outputRegionForThread);

  while (!outputIt.IsAtEnd())
  {
    int index = mapIt.Get() - 1;
    if(index >= numberOfInputImages)
      index = numberOfInputImages-1;
    if(index < 0)
      index = 0;
    outputIt.Set(inputItrVector[index]->Get());

    ++mapIt;
    for (unsigned int i = 0; i < numberOfInputImages; ++i)
    {
      ++inputItrVector[i];
    }
    ++outputIt;
    progress.CompletedPixel(); // potential exception thrown here
  }

  // Free memory
  for (unsigned int i = 0; i < numberOfInputImages; ++i)
  {
    delete inputItrVector[i];
  }
}
} // end namespace itk

#endif

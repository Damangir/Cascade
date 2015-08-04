/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */

#ifndef __itkImageToWeightedHistogramFilter_hxx
#define __itkImageToWeightedHistogramFilter_hxx

#include "itkImageToWeightedHistogramFilter.h"
#include "itkImageRegionConstIterator.h"
#include "itkMacro.h"
namespace itk
{
namespace Statistics
{
template< typename TImage, typename TWImage >
ImageToWeightedHistogramFilter< TImage, TWImage >
::ImageToWeightedHistogramFilter()
{
  this->SetNumberOfRequiredInputs(1);
  this->SetNumberOfRequiredOutputs(1);

  this->ProcessObject::SetNthOutput( 0, this->MakeOutput(0) );

  // same default values as in the HistogramGenerator

  typename SimpleDataObjectDecorator<HistogramMeasurementType>::Pointer marginalScale =
    SimpleDataObjectDecorator<HistogramMeasurementType>::New();
  marginalScale->Set(100);
  this->ProcessObject::SetInput( "MarginalScale", marginalScale );

  SimpleDataObjectDecorator<bool>::Pointer autoMinMax =
    SimpleDataObjectDecorator<bool>::New();
  if( typeid(ValueType) == typeid(signed char) || typeid(ValueType) == typeid(unsigned char) )
    {
    autoMinMax->Set(false);
    }
  else
    {
    autoMinMax->Set(true);
    }
   this->ProcessObject::SetInput( "AutoMinimumMaximum", autoMinMax );
   m_WeightImage = 0;
}
/*
 * TODO: Check mask and Input occupy the same space and have the same dimension
 */
template< typename TImage, typename TWImage >
void
ImageToWeightedHistogramFilter< TImage, TWImage >
::ThreadedComputeHistogram(const RegionType & inputRegionForThread, ThreadIdType threadId, ProgressReporter & progress )
{
  unsigned int nbOfComponents = this->GetInput()->GetNumberOfComponentsPerPixel();
  ImageRegionConstIterator< TImage > inputIt( this->GetInput(), inputRegionForThread );
  inputIt.GoToBegin();
  ImageRegionConstIterator< TWImage > weightIt( this->GetWeightImage(), inputRegionForThread );
  weightIt.GoToBegin();
  HistogramMeasurementVectorType m( nbOfComponents );

  while ( !inputIt.IsAtEnd() )
    {
    const PixelType & p = inputIt.Get();
    NumericTraits<PixelType>::AssignToArray( p, m );
    this->m_Histograms[threadId]->IncreaseFrequencyOfMeasurement( m, weightIt.Get() );
    ++inputIt;
    ++weightIt;
    progress.CompletedPixel();  // potential exception thrown here
    }
}


template< typename TImage, typename TWImage >
void
ImageToWeightedHistogramFilter< TImage, TWImage >
::PrintSelf(std::ostream & os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);
}
} // end of namespace Statistics
} // end of namespace itk

#endif

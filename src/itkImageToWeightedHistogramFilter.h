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
#ifndef __itkImageToWeightedHistogramFilter_h
#define __itkImageToWeightedHistogramFilter_h

#include "itkImageToHistogramFilter.h"
#include "itkHistogram.h"

namespace itk
{
namespace Statistics
{
/** \class ImageToWeightedHistogramFilter
 *  \brief This class generates an histogram from an image.
 *
 *  The concept of Histogram in ITK is quite generic. It has been designed to
 *  manage multiple components data. This class facilitates the computation of
 *  an histogram from an image. Internally it creates a List that is feed into
 *  the SampleToHistogramFilter.
 *
 * \ingroup ITKStatistics
 */

template< typename TImage, typename TWImage = TImage >
class ImageToWeightedHistogramFilter:public ImageToHistogramFilter<TImage>
{
public:
  /** Standard typedefs */
  typedef ImageToWeightedHistogramFilter   Self;
  typedef ImageToHistogramFilter<TImage>   Superclass;
  typedef SmartPointer< Self >             Pointer;
  typedef SmartPointer< const Self >       ConstPointer;

  /** Run-time type information (and related methods). */
  itkTypeMacro(ImageToWeightedHistogramFilter, ImageToHistogramFilter);

  /** standard New() method support */
  itkNewMacro(Self);

  typedef TImage                                               ImageType;
  typedef typename ImageType::PixelType                        PixelType;
  typedef typename ImageType::RegionType                       RegionType;
  typedef typename NumericTraits< PixelType >::ValueType       ValueType;
  typedef typename NumericTraits< ValueType >::RealType        ValueRealType;

  typedef TWImage                                              WeightImageType;
  typedef typename WeightImageType::PixelType                  WeightPixelType;
  typedef typename WeightImageType::RegionType                 WeightRegionType;
  typedef typename NumericTraits< WeightPixelType >::ValueType WeightValueType;
  typedef typename NumericTraits< WeightValueType >::RealType  WeightValueRealType;

  typedef typename Superclass::HistogramType                   HistogramType;
  typedef typename HistogramType::Pointer                      HistogramPointer;
  typedef typename HistogramType::ConstPointer                 HistogramConstPointer;
  typedef typename HistogramType::SizeType                     HistogramSizeType;
  typedef typename HistogramType::MeasurementType              HistogramMeasurementType;
  typedef typename HistogramType::MeasurementVectorType        HistogramMeasurementVectorType;

public:

  itkGetConstObjectMacro(WeightImage, WeightImageType);
  itkSetObjectMacro(WeightImage, WeightImageType);

protected:
  ImageToWeightedHistogramFilter();
  virtual ~ImageToWeightedHistogramFilter() {}
  void PrintSelf(std::ostream & os, Indent indent) const;

  virtual void ThreadedComputeHistogram( const RegionType & inputRegionForThread, ThreadIdType threadId, ProgressReporter & progress );

private:
  ImageToWeightedHistogramFilter(const Self &); //purposely not implemented
  void operator=(const Self &);         //purposely not implemented
  typename WeightImageType::Pointer m_WeightImage;
};
} // end of namespace Statistics
} // end of namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkImageToWeightedHistogramFilter.hxx"
#endif

#endif

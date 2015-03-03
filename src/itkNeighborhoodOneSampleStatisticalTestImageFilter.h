/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */
#ifndef __itkNeighborhoodOneSampleStatisticalTestImageFilter_h
#define __itkNeighborhoodOneSampleStatisticalTestImageFilter_h

#include "itkStatisticalTestBase.h"
#include "itkListSample.h"
#include "itkBoxImageFilter.h"
#include "itkImage.h"
#include "itkArray.h"

namespace itk
{
/** \class NeighborhoodOneSampleStatisticalTestImageFilter
 */
template< typename TInputImage, typename TOutputImage, typename TReferenceSample >
class NeighborhoodOneSampleStatisticalTestImageFilter: public BoxImageFilter<TInputImage,TOutputImage >
{
public:

  /** Standard class typedefs. */
  typedef NeighborhoodOneSampleStatisticalTestImageFilter Self;
  typedef BoxImageFilter< TInputImage, TOutputImage > Superclass;
  typedef SmartPointer< Self > Pointer;
  typedef SmartPointer< const Self > ConstPointer;

  /** Method for creation through the object factory. */
  itkNewMacro (Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(NeighborhoodOneSampleStatisticalTestImageFilter, BoxImageFilter);

  /** Image typedef support. */
  typedef TInputImage InputImageType;
  typedef TOutputImage OutputImageType;

  typedef typename InputImageType::PixelType InputPixelType;
  typedef typename OutputImageType::PixelType OutputPixelType;

  typedef typename InputImageType::RegionType InputImageRegionType;
  typedef typename OutputImageType::RegionType OutputImageRegionType;

  typedef typename InputImageType::SizeType InputSizeType;

  typedef TReferenceSample ReferenceSampleType;
  typedef Array< InputPixelType > MeasurementVectorType;
  typedef Statistics::ListSample<MeasurementVectorType> InternalSampleType;
  typedef Statistics::StatisticalTestBase<TReferenceSample, InternalSampleType> StatisticsTestType;

  itkGetConstMacro(BackgroundPixel, InputPixelType);
  itkSetMacro(BackgroundPixel, InputPixelType);

  itkGetObjectMacro(Statistics,StatisticsTestType);
  itkSetObjectMacro(Statistics, StatisticsTestType);

  itkGetObjectMacro(RefrenceSample,ReferenceSampleType);
  itkSetObjectMacro(RefrenceSample, ReferenceSampleType);

#ifdef ITK_USE_CONCEPT_CHECKING
  // Begin concept checking
  itkConceptMacro( InputLessThanComparableCheck,
      ( Concept::LessThanComparable< InputPixelType > ) );
  // End concept checking
#endif

protected:
  NeighborhoodOneSampleStatisticalTestImageFilter();
  virtual ~NeighborhoodOneSampleStatisticalTestImageFilter()
  {
  }

  void ThreadedGenerateData(const OutputImageRegionType & outputRegionForThread,
                            ThreadIdType threadId);

private:
  NeighborhoodOneSampleStatisticalTestImageFilter(const Self &); //purposely not implemented
  void operator=(const Self &); //purposely not implemented

  typename StatisticsTestType::Pointer m_Statistics;
  typename ReferenceSampleType::Pointer m_RefrenceSample;

  InputPixelType m_BackgroundPixel;

};
}
// end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkNeighborhoodOneSampleStatisticalTestImageFilter.hxx"
#endif

#endif

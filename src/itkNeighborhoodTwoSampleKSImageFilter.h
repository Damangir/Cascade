/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */
#ifndef __itkNeighborhoodTwoSampleKSImageFilter_h
#define __itkNeighborhoodTwoSampleKSImageFilter_h

#include "itkBoxImageFilter.h"
#include "itkImage.h"

#include "itkKolmogorovSmirnovTest.h"

namespace itk
{
/** \class NeighborhoodTwoSampleKSImageFilter
 */
template< typename TInputImage, typename TReferenceImage, typename ProbabilityPrecision = double,
    typename LabelType = unsigned char >
class NeighborhoodTwoSampleKSImageFilter: public BoxImageFilter< TInputImage,
    Image< ProbabilityPrecision, TInputImage::ImageDimension > >
{
public:
  /** Extract dimension from input and output image. */
    itkStaticConstMacro(InputImageDimension, unsigned int,
        TInputImage::ImageDimension);
      itkStaticConstMacro(OutputImageDimension, unsigned int,
          InputImageDimension);

      /** Convenient typedefs for simplifying declarations. */
      typedef TInputImage InputImageType;
      typedef TReferenceImage ReferenceImageType;
      typedef Image<ProbabilityPrecision, TInputImage::ImageDimension> OutputImageType;
      typedef Image< LabelType, InputImageDimension > LabelImageType;

      /** Standard class typedefs. */
      typedef NeighborhoodTwoSampleKSImageFilter Self;
      typedef ImageToImageFilter< InputImageType, OutputImageType > Superclass;
      typedef SmartPointer< Self > Pointer;
      typedef SmartPointer< const Self > ConstPointer;

      /** Method for creation through the object factory. */
      itkNewMacro(Self);

      /** Run-time type information (and related methods). */
      itkTypeMacro(NeighborhoodTwoSampleKSImageFilter, BoxImageFilter);

      /** Image typedef support. */
      typedef typename InputImageType::PixelType InputPixelType;
      typedef typename OutputImageType::PixelType OutputPixelType;
      typedef typename ReferenceImageType::PixelType ReferenceImagePixelType;
      typedef typename ReferenceImageType::InternalPixelType ReferenceImageInternalPixelType;

      typedef typename InputImageType::RegionType InputImageRegionType;
      typedef typename OutputImageType::RegionType OutputImageRegionType;
      typedef typename ReferenceImageType::RegionType ReferenceImageRegionType;

      typedef typename InputImageType::SizeType InputSizeType;

      typedef Statistics::KolmogorovSmirnovTest< InputPixelType > KSType;
      typedef typename KSType::DistributionType DistributionType;

#ifdef ITK_USE_CONCEPT_CHECKING
    // Begin concept checking
    itkConceptMacro( InputLessThanComparableCheck,
        ( Concept::LessThanComparable< InputPixelType > ) );
    // End concept checking
#endif

    void SetReference( const ReferenceImageType *image)
    {
      this->ProcessObject::SetNthInput( 1, const_cast< ReferenceImageType * >( image ) );
    }

    const ReferenceImageType * GetReference()
    {
      return static_cast<const ReferenceImageType*>(this->ProcessObject::GetInput(1));
    }

    void SetMask( const LabelImageType *image)
    {
      this->ProcessObject::SetNthInput( 2, const_cast< LabelImageType * >( image ) );
    }

    const LabelImageType * GetMask()
    {
      return static_cast<const LabelImageType*>(this->ProcessObject::GetInput(2));
    }

    virtual bool GetPositive () const
    {
      return m_KS->GetPositive();
    }
    virtual void SetPositive (const bool _arg)
    {
      m_KS->SetPositive(_arg);
      this->Modified();
    }
    itkBooleanMacro(Positive);

  protected:
    NeighborhoodTwoSampleKSImageFilter();
    virtual ~NeighborhoodTwoSampleKSImageFilter()
    {}

    void ThreadedGenerateData(const OutputImageRegionType & outputRegionForThread,
        ThreadIdType threadId);

  private:
    NeighborhoodTwoSampleKSImageFilter(const Self &); //purposely not implemented
    void operator=(const Self &);//purposely not implemented

    typedef typename KSType::PairDistribution PairDistribution;
    typename KSType::Pointer m_KS;
  };
}
// end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkNeighborhoodTwoSampleKSImageFilter.hxx"
#endif

#endif

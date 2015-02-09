/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */
#ifndef __itkNeighborhoodRingOneSampleKSImageFilter_h
#define __itkNeighborhoodRingOneSampleKSImageFilter_h

#include "itkBoxImageFilter.h"
#include "itkImage.h"

#include "itkKolmogorovSmirnovTest.h"

namespace itk
{
/** \class NeighborhoodRingOneSampleKSImageFilter
 */
template< typename TInputImage, typename ProbabilityPrecision = double,
    typename LabelType = unsigned char >
class NeighborhoodRingOneSampleKSImageFilter: public BoxImageFilter< TInputImage,
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
      typedef Image<ProbabilityPrecision, TInputImage::ImageDimension> OutputImageType;
      typedef Image< LabelType, InputImageDimension > LabelImageType;

      /** Standard class typedefs. */
      typedef NeighborhoodRingOneSampleKSImageFilter Self;
      typedef BoxImageFilter< InputImageType, OutputImageType > Superclass;
      typedef SmartPointer< Self > Pointer;
      typedef SmartPointer< const Self > ConstPointer;

      /** Method for creation through the object factory. */
      itkNewMacro(Self);

      /** Run-time type information (and related methods). */
      itkTypeMacro(NeighborhoodRingOneSampleKSImageFilter, BoxImageFilter);

      /** Image typedef support. */
      typedef typename InputImageType::PixelType InputPixelType;
      typedef typename OutputImageType::PixelType OutputPixelType;

      typedef typename InputImageType::RegionType InputImageRegionType;
      typedef typename OutputImageType::RegionType OutputImageRegionType;

      typedef typename InputImageType::SizeType InputSizeType;

      typedef Statistics::KolmogorovSmirnovTest< InputPixelType > KSType;
      typedef typename KSType::DistributionType DistributionType;

      typedef typename Superclass::RadiusType RadiusType;

#ifdef ITK_USE_CONCEPT_CHECKING
    // Begin concept checking
    itkConceptMacro( InputLessThanComparableCheck,
        ( Concept::LessThanComparable< InputPixelType > ) );
    // End concept checking
#endif

    void SetMask( const LabelImageType *image)
    {
      this->ProcessObject::SetNthInput( 1, const_cast< LabelImageType * >( image ) );
    }

    const LabelImageType * GetMask()
    {
      return static_cast<const LabelImageType*>(this->ProcessObject::GetInput(1));
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

    itkGetConstMacro(InnerRadius, RadiusType);
    itkSetMacro(InnerRadius, RadiusType);

  protected:
    NeighborhoodRingOneSampleKSImageFilter();
    virtual ~NeighborhoodRingOneSampleKSImageFilter()
    {}

    void ThreadedGenerateData(const OutputImageRegionType & outputRegionForThread,
        ThreadIdType threadId);

  private:
    NeighborhoodRingOneSampleKSImageFilter(const Self &); //purposely not implemented
    void operator=(const Self &);//purposely not implemented

    typedef typename KSType::PairDistribution PairDistribution;
    typename KSType::Pointer m_KS;

    RadiusType m_InnerRadius;

  };
}
// end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkNeighborhoodRingOneSampleKSImageFilter.hxx"
#endif

#endif

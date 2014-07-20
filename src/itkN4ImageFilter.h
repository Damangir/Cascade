/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */
#ifndef __itkN4ImageFilter_h
#define __itkN4ImageFilter_h

#include "itkImageToImageFilter.h"
#include "itkNumericTraits.h"

namespace itk
{
template< class TInputImage, class TOutputImage=TInputImage >
class ITK_EXPORT N4ImageFilter:
  public ImageToImageFilter< TInputImage, TOutputImage >
{
public:
  /** Standard "Self" & Superclass typedef.   */
  typedef N4ImageFilter                            Self;
  typedef ImageToImageFilter< TInputImage, TOutputImage > Superclass;

  /** Extract some information from the image types.  Dimensionality
   * of the two images is assumed to be the same. */
  typedef typename TOutputImage::PixelType         OutputImagePixelType;
  typedef typename TOutputImage::InternalPixelType OutputInternalPixelType;
  typedef typename TInputImage::PixelType          InputImagePixelType;
  typedef typename TInputImage::InternalPixelType  InputInternalPixelType;
  itkStaticConstMacro(InputImageDimension, unsigned int,
                      TInputImage::ImageDimension);
  itkStaticConstMacro(OutputImageDimension, unsigned int,
                      TOutputImage::ImageDimension);

  /** Image typedef support. */
  typedef TInputImage                      InputImageType;
  typedef TOutputImage                     OutputImageType;
  typedef typename InputImageType::Pointer InputImagePointer;
  typedef typename NumericTraits< InputImagePixelType >::RealType RealType;

  /** Smart pointer typedef support.   */
  typedef SmartPointer< Self >       Pointer;
  typedef SmartPointer< const Self > ConstPointer;

  /** Run-time type information (and related methods)  */
  itkTypeMacro(N4ImageFilter, ImageToImageFilter);

  /** Method for creation through the object factory.  */
  itkNewMacro(Self);

  itkSetMacro(MinSampleDistance, RealType);
  itkGetConstMacro(MinSampleDistance, RealType);

#ifdef ITK_USE_CONCEPT_CHECKING
  /** Begin concept checking */
  itkConceptMacro( SameDimensionCheck,
                   ( Concept::SameDimension< InputImageDimension, OutputImageDimension > ) );
  /** End concept checking */
#endif

protected:
  N4ImageFilter();

  virtual ~N4ImageFilter()  {}

  void GenerateData();

  void PrintSelf(std::ostream &, Indent) const;
private:
  N4ImageFilter(const Self &); //purposely not implemented
  void operator=(const Self &);       //purposely not implemented
  RealType m_MinSampleDistance;

};
} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkN4ImageFilter.hxx"
#endif

#endif

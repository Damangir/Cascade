/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */

#ifndef __itkMAPMarkovImageFilter_h
#define __itkMAPMarkovImageFilter_h


#include "itkImageToImageFilter.h"
#include "itkMaskImageFilter.h"

#include "itkNormalizeVectorImageFilter.h"

#include "itkAddImageFilter.h"
#include "itkSubtractImageFilter.h"

#include "itkMembershipImageFilter.h"
#include "itkMultiplyVectorImageFilter.h"

#include "itkGibbsMarkovEnergyImageFilter.h"

#include "itkMaximumIndexVectorImageFilter.h"

#include "itkVectorIndexSelectionCastImageFilter.h"

#include "itkRGBGibbsPriorFilter.h"

namespace itk
{

/** \class MAPMarkovImageFilter
 * \brief composite ITK filter for MAP classification with Markov field
 */

template <class TImage, class TClassificationImage=TImage,class TProbabilityPrecision=float>
class ITK_EXPORT
MAPMarkovImageFilter : public ImageToImageFilter<TImage, TClassificationImage>
{
public:
  itkStaticConstMacro(SpaceDimension, unsigned int,
                      TImage::ImageDimension);
  itkStaticConstMacro(ImageDimension, unsigned int,
                      TImage::ImageDimension);

  // standard class typedefs
  typedef MAPMarkovImageFilter                              Self;
  typedef ImageToImageFilter<TImage,TClassificationImage>  Superclass;
  typedef SmartPointer<Self>                              Pointer;
  typedef SmartPointer<const Self>                        ConstPointer;

  // method for creation through the object factory
  itkNewMacro(Self);

  // run-time type information (and related methods)
  itkTypeMacro(MAPMarkovImageFilter, ImageToImageFilter);

  // image and label templates
  typedef TImage                            ImageType;
  typedef typename ImageType::Pointer       ImagePointer;
  typedef typename ImageType::ConstPointer  ImageConstPointer;

  typedef TProbabilityPrecision PriorPixelType;
  typedef TProbabilityPrecision MembershipPixelType;

  typedef TClassificationImage ClassifierOutputImageType;
  typedef typename ClassifierOutputImageType::PixelType ClassificationPixelType;

  typedef itk::VectorImage< PriorPixelType, ImageDimension > PriorsVectorImageType;
  typedef itk::VectorImage< MembershipPixelType, ImageDimension > MembershipsVectorImageType;

  void SetPriorVectorImage( const PriorsVectorImageType *image)
    {
    this->ProcessObject::SetNthInput( 0, const_cast< PriorsVectorImageType * >( image ) );
    }

  void SetMembershipVectorImage( const MembershipsVectorImageType *image)
    {
    this->ProcessObject::SetNthInput( 1, const_cast< MembershipsVectorImageType * >( image ) );
    }

  const PriorsVectorImageType * GetPriorVectorImage()
    {
    return static_cast<const PriorsVectorImageType*>(this->ProcessObject::GetInput(0));
    }

  const MembershipsVectorImageType * GetMembershipVectorImage()
    {
    return static_cast<const MembershipsVectorImageType*>(this->ProcessObject::GetInput(1));
    }

  itkSetMacro(NumberOfIterations, unsigned int);
  itkGetMacro(NumberOfIterations, unsigned int);

  itkSetMacro(PriorBias, float);
  itkGetMacro(PriorBias, float);

protected:

  MAPMarkovImageFilter();
  ~MAPMarkovImageFilter(){}

  // does the real work
  virtual void GenerateData();
  // display
  void PrintSelf( std::ostream& os, Indent indent ) const;

private:

  MAPMarkovImageFilter(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented

  typedef itk::NormalizeVectorImageFilter< PriorsVectorImageType,
      PriorsVectorImageType > NormalizeProbabilitiesFilterType;
  typedef itk::MultiplyVectorImageFilter< PriorsVectorImageType,
      MembershipsVectorImageType, PriorsVectorImageType > FreqBayesFilterType;
  typedef itk::MaximumIndexVectorImageFilter< PriorsVectorImageType,
      ClassifierOutputImageType > MaxIndexFilterType;
  typedef itk::GibbsMarkovEnergyImageFilter< ClassifierOutputImageType > GibbsMarkovEnergyFilterType;

  unsigned int m_NumberOfIterations;
  float m_PriorBias;

}; // end of class

} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkMAPMarkovImageFilter.hxx"
#endif

#endif

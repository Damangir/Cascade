/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */

#ifndef __itkIterativeBayesianImageFilter_h
#define __itkIterativeBayesianImageFilter_h

#include "itkImageToImageFilter.h"
#include "itkMaskImageFilter.h"
#include "itkVectorContainer.h"
#include "itkLogicOpsFunctors.h"
#include "itkBinaryFunctorImageFilter.h"
#include "itkVectorImageToImageAdaptor.h"

#include "itkMAPMarkovImageFilter.h"
#include "itkImageToWeightedHistogramFilter.h"
#include "itkEmpiricalDensityMembershipFunction.h"

namespace itk
{

/** \class IterativeBayesianImageFilter
 * \brief composite ITK filter for MAP classification with Markov field
 */

template <class TImage, class TClassificationImage=TImage,class TProbabilityPrecision=float>
class ITK_EXPORT
IterativeBayesianImageFilter : public ImageToImageFilter<TImage, TClassificationImage>
{
public:
  itkStaticConstMacro(SpaceDimension, unsigned int,
                      TImage::ImageDimension);
  itkStaticConstMacro(ImageDimension, unsigned int,
                      TImage::ImageDimension);

  // standard class typedefs
  typedef IterativeBayesianImageFilter                              Self;
  typedef ImageToImageFilter<TImage,TClassificationImage>  Superclass;
  typedef SmartPointer<Self>                              Pointer;
  typedef SmartPointer<const Self>                        ConstPointer;

  // method for creation through the object factory
  itkNewMacro(Self);

  // run-time type information (and related methods)
  itkTypeMacro(IterativeBayesianImageFilter, ImageToImageFilter);

  // image and label templates
  typedef TImage                            ImageType;
  typedef typename ImageType::Pointer       ImagePointer;
  typedef typename ImageType::ConstPointer  ImageConstPointer;

  typedef itk::MAPMarkovImageFilter< ImageType, TClassificationImage, TProbabilityPrecision > MAPMarkovFilterType;

  typedef typename MAPMarkovFilterType::PriorsVectorImageType PriorsVectorImageType;
  typedef typename MAPMarkovFilterType::MembershipsVectorImageType MembershipsVectorImageType;
  typedef typename MAPMarkovFilterType::ClassifierOutputImageType ClassifierOutputImageType;

  typedef typename PriorsVectorImageType::InternalPixelType PriorPixelType;
  typedef typename MembershipsVectorImageType::InternalPixelType MembershipPixelType;
  typedef typename ClassifierOutputImageType::PixelType ClassificationPixelType;

  typedef Image< MembershipPixelType, ImageDimension > MembershipsImageType;
  typedef Image< PriorPixelType, ImageDimension > PriorImageType;

  typedef Statistics::ImageToWeightedHistogramFilter< ImageType, MembershipsImageType > WeightedHistogramType;
  typedef typename WeightedHistogramType::HistogramMeasurementVectorType MeasurementVectorType;
  /*
   * TODO: Use a more robust membership
   */
  typedef Statistics::EmpiricalDensityMembershipFunction< MeasurementVectorType > EmpiricalDistributionMembershipType;

  typedef MembershipImageFilter< ImageType,
      EmpiricalDistributionMembershipType, TProbabilityPrecision,
      MembershipsVectorImageType > MembershipFilterType;

  void SetPriorVectorImage( const PriorsVectorImageType *image)
    {
    this->ProcessObject::SetNthInput( 1, const_cast< PriorsVectorImageType * >( image ) );
    }

  const PriorsVectorImageType * GetPriorVectorImage()
    {
    return static_cast<const PriorsVectorImageType*>(this->ProcessObject::GetInput(1));
    }


  itkSetMacro(NumberOfIterations, unsigned int);
  itkGetMacro(NumberOfIterations, unsigned int);

  void AddMembershipFunction(EmpiricalDistributionMembershipType * _arg)
  {
    m_MembershipFunctions->InsertElement(m_NumberOfClasses, _arg);
    m_NumberOfClasses = m_MembershipFunctions->Size();
  }
  void ClearMembershipFunctions()
  {
    m_MembershipFunctions->Initialize(); // Clear elements
    m_NumberOfClasses = 0;
  }
  itkSetMacro(PriorBias, float);
  itkGetMacro(PriorBias, float);

protected:

  IterativeBayesianImageFilter();
  ~IterativeBayesianImageFilter(){}

  // does the real work
  virtual void GenerateData();
  // display
  void PrintSelf( std::ostream& os, Indent indent ) const;

private:
  typedef Functor::Equal<ClassificationPixelType,ClassificationPixelType,ClassificationPixelType> ClassSelectorFunctor;
  typedef BinaryFunctorImageFilter<ClassifierOutputImageType,ClassifierOutputImageType, ClassifierOutputImageType, ClassSelectorFunctor> ClassSelectorFilter;
  typedef MaskImageFilter<ClassifierOutputImageType,PriorImageType,MembershipsImageType> MaskPriorFilter;
  typedef itk::VectorIndexSelectionCastImageFilter< PriorsVectorImageType, PriorImageType > PriorSelectorImageFilterType;

  typedef typename EmpiricalDistributionMembershipType::Pointer MembershipFunctionPointer;

  typedef VectorContainer< unsigned int, MembershipFunctionPointer > MembershipFunctionContainerType;
  typedef typename MembershipFunctionContainerType::Pointer MembershipFunctionContainerPointer;


  IterativeBayesianImageFilter(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented

  unsigned int m_NumberOfIterations;
  MembershipFunctionContainerPointer m_MembershipFunctions;
  unsigned int m_NumberOfClasses;
  float m_PriorBias;

}; // end of class

} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkIterativeBayesianImageFilter.hxx"
#endif

#endif

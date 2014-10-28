/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */

#ifndef __itkAffineTransformCalculator_h
#define __itkAffineTransformCalculator_h


#include "itkImageToImageFilter.h"

#include "itkDataObjectDecorator.h"

#include "itkRescaleIntensityImageFilter.h"

#include "itkMultiResolutionImageRegistrationMethod.h"
#include "itkResampleImageFilter.h"
#include "itkLinearInterpolateImageFunction.h"
#include "itkCenteredTransformInitializer.h"
#include "itkVersorRigid3DTransform.h"
#include "itkAffineTransform.h"
#include "itkVersorRigid3DTransformOptimizer.h"
#include "itkRegularStepGradientDescentOptimizer.h"
#include "itkMattesMutualInformationImageToImageMetric.h"

namespace itk
{

/** \class AffineTransformCalculator
 * \brief composite ITK filter for calculating affine transform
 */

template <class TFixedImageType, class TMovingImageType=TFixedImageType>
class ITK_EXPORT
AffineTransformCalculator : public ProcessObject
{
public:
  itkStaticConstMacro(SpaceDimension, unsigned int,
                      TMovingImageType::ImageDimension);

  // standard class typedefs
  typedef AffineTransformCalculator                       Self;
  typedef ProcessObject                                   Superclass;
  typedef SmartPointer<Self>                              Pointer;
  typedef SmartPointer<const Self>                        ConstPointer;

  // method for creation through the object factory
  itkNewMacro(Self);

  // run-time type information (and related methods)
  itkTypeMacro(AffineTransformCalculator, ProcessObject);

  typedef itk::AffineTransform<double,SpaceDimension> TransformType;


  // image and label templates
  typedef TFixedImageType                        FixedImageType;
  typedef typename FixedImageType::Pointer       FixedImagePointer;
  typedef typename FixedImageType::ConstPointer  FixedImageConstPointer;

  typedef TMovingImageType                       MovingImageType;
  typedef typename MovingImageType::Pointer      MovingImagePointer;
  typedef typename MovingImageType::ConstPointer MovingImageConstPointer;

  typedef DataObjectDecorator< TransformType > TransformObjectType;

  TransformObjectType * GetTransformationOutput();
  const TransformObjectType * GetTransformationOutput() const;


  void SetMovingImage(const TMovingImageType *image)
    {
    this->ProcessObject::SetNthInput( 0, const_cast< MovingImageType * >( image ) );
    }
  void SetFixedImage(const TFixedImageType *image)
    {
    this->ProcessObject::SetNthInput( 1, const_cast< FixedImageType * >( image ) );
    }

  virtual const TransformType * GetAffineTransform () const
    {
    return this->GetTransformationOutput()->Get();
    }

  itkGetConstMacro(IntraRegistration, bool);
  itkSetMacro(IntraRegistration, bool);
  itkBooleanMacro(IntraRegistration);
protected:

  AffineTransformCalculator();
  ~AffineTransformCalculator(){}

  void PrintSelf( std::ostream& os, Indent indent ) const;
  // does the real work
  virtual void GenerateData();
  virtual void VerifyInputInformation() {}

private:

  const MovingImageType * GetMovingImage()
    {
    return static_cast<const MovingImageType*>(this->ProcessObject::GetInput(0));
    }

  const FixedImageType * GetFixedImage()
    {
    return static_cast<const FixedImageType*>(this->ProcessObject::GetInput(1));
    }


  AffineTransformCalculator(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented

  typedef itk::VersorRigid3DTransform<double> RigidTransformType;

  typename RigidTransformType::Pointer m_RigidTransform;
  typename TransformType::Pointer m_AffineTransform;

  FixedImagePointer  m_FixedImage;
  MovingImagePointer m_MovingImage;

  bool m_IntraRegistration;
  void RescaleImages();
  void DownsampleImage();
  void RigidRegistration();
  void AffineRegistration();

}; // end of class

} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkAffineTransformCalculator.hxx"
#endif

#endif

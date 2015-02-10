/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */

#ifndef __itkAffineTransformCalculator_h
#define __itkAffineTransformCalculator_h

#include "itkImageToImageFilter.h"
#include "itkDataObjectDecorator.h"

#include "itkAffineTransform.h"
#include "itkImageMaskSpatialObject.h"
#include "itkBinaryThresholdImageFilter.h"

#include "itkCenteredTransformInitializer.h"
#include "itkMultiResolutionImageRegistrationMethod.h"
#include "itkMattesMutualInformationImageToImageMetric.h"
#include "itkRegularStepGradientDescentOptimizer.h"
#include "itkRecursiveMultiResolutionPyramidImageFilter.h"
#include "itkImage.h"
#include "itkCastImageFilter.h"

#include "itkCommand.h"

namespace itk
{

/** \class AffineTransformCalculator
 * \brief composite ITK filter for calculating affine transform
 */
template< class TFixedImageType, class TMovingImageType = TFixedImageType >
class AffineTransformCalculator: public ProcessObject
{
public:
  // standard class typedefs
  typedef AffineTransformCalculator Self;
  typedef ProcessObject Superclass;
  typedef SmartPointer< Self > Pointer;
  typedef SmartPointer< const Self > ConstPointer;

  // method for creation through the object factory
  itkNewMacro (Self);

  // run-time type information (and related methods)
  itkTypeMacro(AffineTransformCalculator, ProcessObject);

  // image and label templates
  typedef TFixedImageType FixedImageType;
  typedef typename FixedImageType::Pointer FixedImagePointer;
  typedef typename FixedImageType::ConstPointer FixedImageConstPointer;

  typedef TMovingImageType MovingImageType;
  typedef typename MovingImageType::Pointer MovingImagePointer;
  typedef typename MovingImageType::ConstPointer MovingImageConstPointer;

  itkStaticConstMacro(ImageDimension, unsigned int, FixedImageType::ImageDimension);

  typedef float InternalPixelType;
  typedef Image< InternalPixelType, ImageDimension > InternalImageType;

  typedef unsigned char MaskPixelType;
  typedef Image< MaskPixelType, ImageDimension > MaskImageType;

  typedef ImageMaskSpatialObject< ImageDimension > MaskType;

  typedef AffineTransform< double, ImageDimension > TransformType;

  typedef RegularStepGradientDescentOptimizer OptimizerType;
  typedef LinearInterpolateImageFunction< InternalImageType, double > InterpolatorType;
  typedef MattesMutualInformationImageToImageMetric< InternalImageType,
      InternalImageType > MetricType;

  typedef OptimizerType::ScalesType OptimizerScalesType;

  typedef MultiResolutionImageRegistrationMethod< InternalImageType,
      InternalImageType > RegistrationType;

  typedef DataObjectDecorator< TransformType > TransformObjectType;

  TransformObjectType * GetTransformationOutput();
  const TransformObjectType * GetTransformationOutput() const;

  void SetMovingImage(const TMovingImageType *image)
  {
    this->ProcessObject::SetNthInput(0, const_cast< MovingImageType * >(image));
  }
  void SetFixedImage(const TFixedImageType *image)
  {
    this->ProcessObject::SetNthInput(1, const_cast< FixedImageType * >(image));
  }

  virtual const TransformType * GetAffineTransform() const
  {
    return this->GetTransformationOutput()->Get();
  }

  itkGetConstMacro(IntraRegistration, bool);
  itkSetMacro(IntraRegistration, bool);
  itkBooleanMacro (IntraRegistration);

protected:

  AffineTransformCalculator();
  ~AffineTransformCalculator()
  {
  }

  void PrintSelf(std::ostream& os, Indent indent) const;
  // does the real work
  virtual void GenerateData();
  virtual void VerifyInputInformation()
  {
  }

private:

  class RegistrationInterfaceCommand: public Command
  {
  public:
    typedef RegistrationInterfaceCommand Self;
    typedef itk::Command Superclass;
    typedef itk::SmartPointer< Self > Pointer;
    itkNewMacro (Self);

  protected:
    RegistrationInterfaceCommand()
    {
    }
    ;

  public:
    typedef RegistrationType * RegistrationPointer;
    typedef OptimizerType * OptimizerPointer;
    void Execute(itk::Object * object, const itk::EventObject & event)
    {
      if (!(itk::IterationEvent().CheckEvent(&event)))
      {
        return;
      }
      RegistrationPointer registration =
          dynamic_cast< RegistrationPointer >(object);
      OptimizerPointer optimizer =
          dynamic_cast< OptimizerPointer >(registration->GetOptimizer());

      std::cout << "-------------------------------------" << std::endl;
      std::cout << "MultiResolution Level : " << registration->GetCurrentLevel()
      << std::endl;
      std::cout << std::endl;

      if (registration->GetCurrentLevel() == 0)
      {
        optimizer->SetMaximumStepLength(16.00);
        optimizer->SetMinimumStepLength(0.01);
      }
      else
      {
        optimizer->SetMaximumStepLength(
            optimizer->GetMaximumStepLength() / 4.0);
        optimizer->SetMinimumStepLength(
            optimizer->GetMinimumStepLength() / 10.0);
      }
    }
    void Execute(const itk::Object *, const itk::EventObject &)
    {
      return;
    }
  };

  const MovingImageType * GetMovingImage()
  {
    return static_cast< const MovingImageType* >(this->ProcessObject::GetInput(
        0));
  }

  const FixedImageType * GetFixedImage()
  {
    return static_cast< const FixedImageType* >(this->ProcessObject::GetInput(1));
  }

  AffineTransformCalculator(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented

  typename TransformType::Pointer m_AffineTransform;

  FixedImagePointer m_FixedImage;
  MovingImagePointer m_MovingImage;

  unsigned int m_NumberOfBins;
  unsigned int m_NumberOfSamples;

  double m_TransitionRelativeScale; // 1.0 / 1e7

  unsigned int m_NumberOfIterations;
  double m_RelaxationFactor;

  unsigned int m_NumberOfLevels;
  bool m_IntraRegistration;
};
// end of class

}// end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkAffineTransformCalculator.hxx"
#endif

#endif

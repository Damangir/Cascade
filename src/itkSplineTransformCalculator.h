/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */

#ifndef __itkSplineTransformCalculator_h
#define __itkSplineTransformCalculator_h

#include "itkDataObjectDecorator.h"

#include "itkImageRegistrationMethodv4.h"
#include "itkMattesMutualInformationImageToImageMetricv4.h"

#include "itkAffineTransform.h"
#include "itkBSplineTransform.h"
#include "itkCompositeTransform.h"

#include "itkLBFGSBOptimizer.h"

#include "itkBSplineResampleImageFunction.h"
#include "itkBSplineDecompositionImageFilter.h"
#include "itkResampleImageFilter.h"

#include "itkCastImageFilter.h"

#include "itkImageMaskSpatialObject.h"

#include "itkTimeProbesCollectorBase.h"
#include "itkMemoryProbesCollectorBase.h"

namespace itk
{

/** \class SplineTransformCalculator
 * \brief composite ITK filter for automatic global registration
 */
template< class TFixedImageType, class TMovingImageType = TFixedImageType >
class ITK_EXPORT
SplineTransformCalculator: public ProcessObject
{
public:
  itkStaticConstMacro(SpaceDimension, unsigned int,
      TMovingImageType::ImageDimension);
  itkStaticConstMacro(SplineOrder, unsigned int, 3);

  // standard class typedefs
  typedef SplineTransformCalculator Self;
  typedef ProcessObject Superclass;
  typedef SmartPointer< Self > Pointer;
  typedef SmartPointer< const Self > ConstPointer;

  // method for creation through the object factory
  itkNewMacro(Self)
  ;

  // run-time type information (and related methods)
  itkTypeMacro(SplineTransformCalculator, ProcessObject)
  ;

  // image and label templates
  typedef TFixedImageType FixedImageType;
  typedef typename FixedImageType::Pointer FixedImagePointer;
  typedef typename FixedImageType::ConstPointer FixedImageConstPointer;

  typedef TMovingImageType MovingImageType;
  typedef typename MovingImageType::Pointer MovingImagePointer;
  typedef typename MovingImageType::ConstPointer MovingImageConstPointer;

  typedef double CoordinateRepType;

  /*
   * Transformation definitions
   */
  typedef BSplineTransform< CoordinateRepType, SpaceDimension, SplineOrder > BSplineTransformType;
  typedef CompositeTransform< CoordinateRepType, SpaceDimension > CompositeTransformType;
  typedef typename CompositeTransformType::TransformType TransformType;
  typedef typename BSplineTransformType::ParametersType ParametersType;
  // We need object decorator to pass the transformation as output
  typedef DataObjectDecorator< TransformType > TransformObjectType;

  /*
   * Registration definition
   */
  typedef MattesMutualInformationImageToImageMetricv4< FixedImageType,
      MovingImageType > MetricType;

  typedef ImageRegistrationMethodv4< FixedImageType, MovingImageType,
      BSplineTransformType > BSplineRegistrationType;
  typedef LBFGSBOptimizer LBFGSBOptimizerType;
  typedef GradientDescentOptimizerv4 OptimizerType;
  typedef LinearInterpolateImageFunction< MovingImageType, CoordinateRepType > LinearInterpolatorType;

  /*
   * Internal Filters
   */
  typedef ImageMaskSpatialObject< SpaceDimension > SpatialObjectMaskType;
  typedef typename SpatialObjectMaskType::ImageType MaskImageType;
  typedef CastImageFilter< FixedImageType, MaskImageType > FixedImageToMaskImageType;

  TransformObjectType * GetTransformationOutput();
  const TransformObjectType * GetTransformationOutput() const;

  typedef ProcessObject::DataObjectPointerArraySizeType DataObjectPointerArraySizeType;
  using Superclass::MakeOutput;
  virtual typename DataObject::Pointer MakeOutput(
      DataObjectPointerArraySizeType)
  {
    return TransformObjectType::New().GetPointer();
  }

  void SetMovingImage(const MovingImageType *image)
  {
    this->ProcessObject::SetNthInput(0, const_cast< MovingImageType * >(image));
  }
  const MovingImageType * GetMovingImage()
  {
    return static_cast< const MovingImageType* >(this->ProcessObject::GetInput(
        0));
  }

  void SetFixedImage(const FixedImageType *image)
  {
    this->ProcessObject::SetNthInput(1, const_cast< FixedImageType * >(image));
  }
  const FixedImageType * GetFixedImage()
  {
    return static_cast< const FixedImageType* >(this->ProcessObject::GetInput(1));
  }

  void SetFixedImageMask(const MaskImageType *image)
  {
    this->ProcessObject::SetNthInput(2, const_cast< MaskImageType * >(image));
  }
  const MaskImageType * GetFixedImageMask()
  {
    return static_cast< const MaskImageType* >(this->ProcessObject::GetInput(2));
  }

  void AddInitialTransform(TransformType *tfm)
  {
    m_Transform->AddTransform(tfm);
    this->Modified();
  }

  /*
   * Getters
   */
  itkGetConstMacro(UseExplicitPDFDerivatives,bool)
  itkGetConstMacro(UseCachingOfBSplineWeights,bool)

  itkGetConstMacro(NumberOfIterations, SizeValueType)

  itkGetObjectMacro(Transform, CompositeTransformType)
  /*
   * Setters
   */
  itkSetMacro(UseExplicitPDFDerivatives,bool)
  itkSetMacro(UseCachingOfBSplineWeights,bool)

  itkSetMacro(NumberOfIterations, SizeValueType)

protected:
  SplineTransformCalculator();
  virtual ~SplineTransformCalculator()
  {
  }
  virtual void GenerateData();
  void PrintSelf(std::ostream& os, Indent indent) const;

private:
  SplineTransformCalculator(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented

  bool m_UseExplicitPDFDerivatives;
  bool m_UseCachingOfBSplineWeights;

  SizeValueType m_NumberOfIterations;

  typename CompositeTransformType::Pointer m_Transform;

  typename SpatialObjectMaskType::Pointer m_FixedImageMaskSpatialObject;

};
// end of class

}
// end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkSplineTransformCalculator.hxx"
#endif

#endif

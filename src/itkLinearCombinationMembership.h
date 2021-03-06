/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */
#ifndef __itkLinearCombinationMembership_h
#define __itkLinearCombinationMembership_h

#include "itkMatrix.h"
#include "itkMembershipFunctionBase.h"

namespace itk
{
namespace Statistics
{
template< typename TMeasurementVector >
class LinearCombinationMembership: public MembershipFunctionBase<
    TMeasurementVector >
{
public:
  /** Standard class typedefs */

  typedef LinearCombinationMembership Self;
  typedef MembershipFunctionBase< TMeasurementVector > Superclass;
  typedef SmartPointer< Self > Pointer;
  typedef SmartPointer< const Self > ConstPointer;

  /** Standard macros */
  itkTypeMacro(LinearCombinationMembership, MembershipFunctionBase)
  ;itkNewMacro(Self)
  ;

  /** SmartPointer class for superclass */
  typedef Superclass MembershipFunctionType;
  typedef typename Superclass::Pointer MembershipFunctionPointerType;

  typedef std::pair<MembershipFunctionPointerType, double> WeightedComponentType;
  typedef std::vector< WeightedComponentType > DistributionType;

  /** Typedef alias for the measurement vectors */
  typedef TMeasurementVector MeasurementVectorType;

  /** Length of each measurement vector */
  typedef typename Superclass::MeasurementVectorSizeType MeasurementVectorSizeType;

  /** Type of the mean vector. RealType on a vector-type is the same
   * vector-type but with a real element type.  */
  typedef typename itk::NumericTraits< MeasurementVectorType >::RealType MeasurementVectorRealType;
  typedef MeasurementVectorRealType MeanVectorType;

  /** Evaluate the probability density of a measurement vector. */
  double Evaluate(const MeasurementVectorType & measurement) const;
  double NthEvaluate(const size_t n, const MeasurementVectorType & measurement) const;

  void AddMembership(WeightedComponentType mem);
  void AddMembership(Superclass* mem, double w);
  void Reset();

protected:
  LinearCombinationMembership(void);
  virtual ~LinearCombinationMembership(void)
  {
  }
  void PrintSelf(std::ostream & os, Indent indent) const;

private:
  LinearCombinationMembership(const Self &);   //purposely not implemented
  void operator=(const Self &); //purposely not implemented

  DistributionType m_Distribution;
};
} // end of namespace Statistics
} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkLinearCombinationMembership.hxx"
#endif

#endif

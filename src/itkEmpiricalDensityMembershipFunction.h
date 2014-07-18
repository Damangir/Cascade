/*=========================================================================
 *
 *  Copyright Insight Software Consortium
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/
#ifndef __itkEmpiricalDensityMembershipFunction_h
#define __itkEmpiricalDensityMembershipFunction_h

#include "itkMembershipFunctionBase.h"
#include "itkMeasurementVectorTraits.h"

namespace itk
{
namespace Statistics
{
/** \class EmpiricalDensityMembershipFunction
 * \brief EmpiricalDensityMembershipFunction models class membership
 * using an empirical density metric.
 *
 */
template< typename TVector >
class EmpiricalDensityMembershipFunction:
  public MembershipFunctionBase< TVector >
{
public:
  /** Standard class typedefs */
  typedef EmpiricalDensityMembershipFunction   Self;
  typedef MembershipFunctionBase< TVector >    Superclass;
  typedef SmartPointer< Self >                 Pointer;
  typedef SmartPointer< const Self >           ConstPointer;

  /** Strandard macros */
  itkTypeMacro(EmpiricalDensityMembershipFunction,
               MembershipFunctionBase);
  itkNewMacro(Self);

  /** SmartPointer class for superclass */
  typedef typename Superclass::Pointer MembershipFunctionPointer;

  /** Typedef alias for the measurement vectors */
  typedef TVector MeasurementVectorType;

  /** Typedef to represent the length of measurement vectors */
  typedef typename Superclass::MeasurementVectorSizeType
  MeasurementVectorSizeType;

  /**  Set the length of each measurement vector. */
  virtual void SetMeasurementVectorSize(MeasurementVectorSizeType);

  /** Type of the Distribution to use to use */
  typedef typename MeasurementVectorTraitsTypes<MeasurementVectorType>::ValueType MeasurementValueType;
  typedef Histogram< MeasurementValueType >         DistributionType;
  typedef typename DistributionType::Pointer        DistributionPointer;
  typedef typename DistributionType::IndexType      DistributionIndexType;

  /** Set the Distribution to be used when calling the Evaluate() method */
  itkSetObjectMacro(Distribution, DistributionType);

  /** Get the Distribution used by the MembershipFunction */
  itkGetConstObjectMacro(Distribution, DistributionType);

  /**
   * Method to get probability of an instance. The return value is the
   * value of the density function, not probability. */
  double Evaluate(const MeasurementVectorType & measurement) const;

protected:
  EmpiricalDensityMembershipFunction(void);
  virtual ~EmpiricalDensityMembershipFunction(void) {}
  void PrintSelf(std::ostream & os, Indent indent) const;

private:
  EmpiricalDensityMembershipFunction(const Self &);   //purposely not implemented
  void operator=(const Self &); //purposely not implemented

  DistributionPointer m_Distribution;
};
} // end of namespace Statistics
} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkEmpiricalDensityMembershipFunction.hxx"
#endif

#endif

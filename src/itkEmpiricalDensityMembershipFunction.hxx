/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */

#ifndef __itkEmpiricalDensityMembershipFunction_hxx
#define __itkEmpiricalDensityMembershipFunction_hxx

#include "itkEmpiricalDensityMembershipFunction.h"
#include "itkEuclideanDistanceMetric.h"

namespace itk
{
namespace Statistics
{
template< typename TVector >
EmpiricalDensityMembershipFunction< TVector >
::EmpiricalDensityMembershipFunction()
{
  m_Distribution = 0;
}

template< typename TVector >
void
EmpiricalDensityMembershipFunction< TVector >
::SetMeasurementVectorSize(MeasurementVectorSizeType s)
{
  this->Superclass::SetMeasurementVectorSize(s);
  m_Distribution->SetMeasurementVectorSize(s);
}

template< typename TVector >
double
EmpiricalDensityMembershipFunction< TVector >
::Evaluate(const MeasurementVectorType & measurement) const
{
  itkAssertOrThrowMacro(m_Distribution.IsNotNull(), "You should set distribution before evaluation.")
  DistributionIndexType index;
  m_Distribution->GetIndex( measurement, index );
  return double(m_Distribution->GetFrequency(index))/double(m_Distribution->GetTotalFrequency());
}

template< typename TVector >
void
EmpiricalDensityMembershipFunction< TVector >
::PrintSelf(std::ostream & os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);

  os << indent << "Distribution: " << m_Distribution.GetPointer() << std::endl;
}
} // end namespace Statistics
} // end of namespace itk
#endif

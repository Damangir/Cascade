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

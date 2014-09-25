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
#ifndef __itkLinearCombinationMembership_hxx
#define __itkLinearCombinationMembership_hxx

#include "itkLinearCombinationMembership.h"

namespace itk
{
namespace Statistics
{
template< typename TMeasurementVector >
LinearCombinationMembership< TMeasurementVector >::LinearCombinationMembership()
{
}

template< typename TMeasurementVector >
void LinearCombinationMembership< TMeasurementVector >::PrintSelf(
    std::ostream & os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);
}

template< typename TMeasurementVector >
void LinearCombinationMembership< TMeasurementVector >::AddMembership(
    WeightedComponentType mem)
{
  m_Distribution.push_back(mem);
  this->Modified();
}

template< typename TMeasurementVector >
void LinearCombinationMembership< TMeasurementVector >::Reset()
{
  m_Distribution.erase(m_Distribution.begin(), m_Distribution.end());
  this->Modified();
}

template< typename TMeasurementVector >
void LinearCombinationMembership< TMeasurementVector >::AddMembership(
    Superclass* mem, double w)
{
  m_Distribution.push_back(WeightedComponentType(mem, w));
  this->Modified();
}
template< typename TMeasurementVector >
inline double LinearCombinationMembership< TMeasurementVector >::NthEvaluate(
    const size_t n, const MeasurementVectorType & measurement) const
{
  return m_Distribution[n].first->Evaluate(measurement)
      * m_Distribution[n].second;
}

template< typename TMeasurementVector >
inline double LinearCombinationMembership< TMeasurementVector >::Evaluate(
    const MeasurementVectorType & measurement) const
{
  double eval = 0;
  for (size_t i = 0; i < m_Distribution.size(); i++)
  {
    eval += NthEvaluate(i, measurement);
  }
  eval = eval > 0 ? eval : 0;
  return eval;
}

} // end namespace Statistics
} // end of namespace itk

#endif

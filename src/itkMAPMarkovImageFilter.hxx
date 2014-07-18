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

#ifndef __itkMAPMarkovImageFilter_hxx
#define __itkMAPMarkovImageFilter_hxx

#include "itkMAPMarkovImageFilter.h"

namespace itk
{

template< class TImage, class TClassificationImage, class TProbabilityPrecision >
MAPMarkovImageFilter< TImage, TClassificationImage, TProbabilityPrecision >::MAPMarkovImageFilter()
{
  m_NumberOfIterations = 1;
  m_PriorBias = 0;
}

template< class TImage, class TClassificationImage, class TProbabilityPrecision >
void MAPMarkovImageFilter< TImage, TClassificationImage, TProbabilityPrecision >::GenerateData()
{
  const unsigned int nClass =
      this->GetPriorVectorImage()->GetNumberOfComponentsPerPixel();

  typename NormalizeProbabilitiesFilterType::Pointer normalizePriori =
      NormalizeProbabilitiesFilterType::New();
  typename NormalizeProbabilitiesFilterType::Pointer normalizePosteriori =
      NormalizeProbabilitiesFilterType::New();

  typename FreqBayesFilterType::Pointer freqBayesFilter =
      FreqBayesFilterType::New();
  typename MaxIndexFilterType::Pointer maximumPosteriori =
      MaxIndexFilterType::New();
  typename GibbsMarkovEnergyFilterType::Pointer gibbsMarkov =
      GibbsMarkovEnergyFilterType::New();
  typename FreqBayesFilterType::Pointer gibbsMarkovBayesFilter =
      FreqBayesFilterType::New();

  /*
   * General pipelne setup
   */

  normalizePriori->SetBias(m_PriorBias);
  normalizePriori->SetInput(this->GetPriorVectorImage());

  freqBayesFilter->SetInput1(this->GetMembershipVectorImage());
  freqBayesFilter->SetInput2(normalizePriori->GetOutput());

  normalizePosteriori->SetBias(0);
  normalizePosteriori->SetInput(freqBayesFilter->GetOutput());

  maximumPosteriori->SetInput(normalizePosteriori->GetOutput());

  maximumPosteriori->Update();

  for (unsigned int i = 0; i < nClass; i++)
  {
    gibbsMarkov->AddClass(i);
  }

  for (unsigned int i = 0; i < m_NumberOfIterations; i++)
  {
    typename ClassifierOutputImageType::Pointer prevClassification =
        maximumPosteriori->GetOutput();
    prevClassification->DisconnectPipeline();

    gibbsMarkov->SetInput(prevClassification);
    gibbsMarkovBayesFilter->SetInput1(freqBayesFilter->GetOutput());
    gibbsMarkovBayesFilter->SetInput2(gibbsMarkov->GetOutput());
    maximumPosteriori->SetInput(gibbsMarkovBayesFilter->GetOutput());
    maximumPosteriori->Modified();
    maximumPosteriori->Update();
  }

  /*
   * Put output of gibbsMarkovBayesFilter to the output as well
   */
  this->GraftOutput(maximumPosteriori->GetOutput());
}

template< class TImage, class TClassificationImage, class TProbabilityPrecision >
void MAPMarkovImageFilter< TImage, TClassificationImage, TProbabilityPrecision >::PrintSelf(
    std::ostream& os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);

  os << indent << "Iterations: " << m_NumberOfIterations<< std::endl;
  os << indent << "Prior bias: " << m_PriorBias<< std::endl;
}

} // end namespace itk

#endif

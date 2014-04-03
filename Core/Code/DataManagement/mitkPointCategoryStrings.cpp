/*===================================================================

Copyright (c) National Institutes of Health

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

===================================================================*/

#include "mitkPointCategoryStrings.h"
#include <mitkCommon.h>
#include <mitkVector.h>

/***************       CONSTRUCTOR      ***************/
mitk::mitkPointCategoryStrings::mitkPointCategoryStrings()
{
  // ********************WARNING********************************* //
  // THESE STRINGS CAN NOT INCLUDE XML formatting characters such as
  // '>', '<', '\' or '/'
  // ********************WARNING********************************* //
  m_PointSpecificationTypeStrings.clear();
	//MITK point specifications
  m_PointSpecificationTypeStrings.push_back(std::string("PTUNDEFINED"));
	m_PointSpecificationTypeStrings.push_back(std::string("PTSTART"));
	m_PointSpecificationTypeStrings.push_back(std::string("PTCORNER"));
	m_PointSpecificationTypeStrings.push_back(std::string("PTEDGE"));
	m_PointSpecificationTypeStrings.push_back(std::string("PTEND"));
	//detection true/false
	m_PointSpecificationTypeStrings.push_back(std::string("True lymp node"));
	m_PointSpecificationTypeStrings.push_back(std::string("False lymp node"));
	//size
	m_PointSpecificationTypeStrings.push_back(std::string("Size: short axis less 1cm"));
	m_PointSpecificationTypeStrings.push_back(std::string("Size: short axis geq. 1cm"));
	//density
	m_PointSpecificationTypeStrings.push_back(std::string("Density: less 25HU"));
	m_PointSpecificationTypeStrings.push_back(std::string("Density: geq. 25HU"));
	//Morphology
	m_PointSpecificationTypeStrings.push_back(std::string("Morphology: normal"));
	m_PointSpecificationTypeStrings.push_back(std::string("Morphology: dysmorphic"));
	m_PointSpecificationTypeStrings.push_back(std::string("Morphology: indeterminate"));
	//border
	m_PointSpecificationTypeStrings.push_back(std::string("Border: smooth and distinct"));
	m_PointSpecificationTypeStrings.push_back(std::string("Border: irregular and-or indistinct"));
	m_PointSpecificationTypeStrings.push_back(std::string("Border: indeterminate"));
	//hilum
	m_PointSpecificationTypeStrings.push_back(std::string("Hilum: present"));
	m_PointSpecificationTypeStrings.push_back(std::string("Hilum: not present"));
	m_PointSpecificationTypeStrings.push_back(std::string("Hilum: indeterminate"));
	//calcification
	m_PointSpecificationTypeStrings.push_back(std::string("Calcification: not present"));
	m_PointSpecificationTypeStrings.push_back(std::string("Calcification: less 50%"));
	m_PointSpecificationTypeStrings.push_back(std::string("Calcification: geq. 50%"));
	m_PointSpecificationTypeStrings.push_back(std::string("Calcification: indeterminate"));
	//proximity to adjacent structures
	m_PointSpecificationTypeStrings.push_back(std::string("Proximity to adjacent structures: distinct from other structures"));
	m_PointSpecificationTypeStrings.push_back(std::string("Proximity to adjacent structures: conglomerated to other structures"));
	m_PointSpecificationTypeStrings.push_back(std::string("Proximity to adjacent structures: indeterminate"));
	//orientation in the coronal or sagittal plane
	m_PointSpecificationTypeStrings.push_back(std::string("Orientation in the coronal or sagittal plane: long axis parallel to spine"));
	m_PointSpecificationTypeStrings.push_back(std::string("Orientation in the coronal or sagittal plane: long axis perpendicular to the long axis of the spine"));
	m_PointSpecificationTypeStrings.push_back(std::string("Orientation in the coronal or sagittal plane: indeterminate (lymph node is spherical or long axis is oriented almost exactly 45 degrees to the long axis of the spine)"));
}

std::vector<std::string> mitk::mitkPointCategoryStrings::getPointCategoryStrings()
{
  this->checkIfInitialized();

	return m_PointSpecificationTypeStrings;
}

unsigned int mitk::mitkPointCategoryStrings::getPointCategoryIndex( std::string category )
{
  this->checkIfInitialized();

  for (unsigned int i=0; i<m_PointSpecificationTypeStrings.size(); i++)
  {
    if (m_PointSpecificationTypeStrings[i].compare(category) == 0)
    {
      return i;
    }
  }

  return mitk::PTUNDEFINED;
}

std::string mitk::mitkPointCategoryStrings::getPointCategoryString( unsigned int index )
{
  this->checkIfInitialized();

  return m_PointSpecificationTypeStrings[index];
}

void mitk::mitkPointCategoryStrings::checkIfInitialized()
{
  if (m_PointSpecificationTypeStrings.empty())
  {
    mitkThrow() << "mitkPointCategoryStrings::getPointCategoryString: m_PointSpecificationTypeStrings size is 0 or not initialised.";
  }
}
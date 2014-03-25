/*===================================================================

Copyright (c) National Institutes of Health

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

===================================================================*/

#ifndef MITK_POINTCATEGORYSTRINGS_H
#define MITK_POINTCATEGORYSTRINGS_H

#include <MitkExports.h>

#include <string.h>
#include <vector>

/**
* \brief A Class for point category strings
*
*/

namespace mitk {

class MITK_CORE_EXPORT mitkPointCategoryStrings
{  
private:
  std::vector<std::string> m_PointSpecificationTypeStrings;	

public:

  /***************       CONSTRUCTOR      ***************/
  /**
  * @brief Constructor
  */
  mitkPointCategoryStrings();

  /***************       public functions      ***************/
  /**
  * @brief getPointCategoryStrings()
  */
  std::vector<std::string> getPointCategoryStrings();

  /**
  * @brief getPointCategoryIndex()
  */
  unsigned int getPointCategoryIndex( std::string category );

  /**
  * @brief getPointCategoryString()
  */
  std::string getPointCategoryString( unsigned int index );

  /**
  * @brief checkIfInitialized()
  */
  void checkIfInitialized();
};

} // namespace mitk

#endif /* MITK_POINTCATEGORYSTRINGS_H */

// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef BSR_INDEX_H
#define BSR_INDEX_H

#include <string>

namespace msl {

	class BsrIndex {

	public:

		BsrIndex();
		BsrIndex(int id, int rowIndex, int columnIndex);

		int getColumnIndex() const;
		int getId() const;
		int getRowIndex() const;
		
		void setColumnIndex(int columnIndex);
		void setId(int id);
		void setRowIndex(int rowIndex);

		std::string toString() const;

	private:

		int id;
		int rowIndex;
		int columnIndex;

	};

}

#endif

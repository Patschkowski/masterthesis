// **********************************************************************************************************
// *																										*
// * This Software is published under the MIT License.														*
// * For more information and the license text, please refer to the file 'License' located in the zip file. *
// *																										*
// * Copyright (c) 2001-2011 Steffen Ernsting, Philipp Ciechanowicz, Michael Poldner and Herbert Kuchen     *                 
// *        		{s.ernsting|ciechanowicz|poldner|kuchen}@uni-muenster.de              					*
// *																										*	
// **********************************************************************************************************

#ifndef CONVERSION_H
#define CONVERSION_H

namespace msl {

	// liefert true, wenn U oeffentlich von T erbt oder wenn T und U den gleichen Typ besitzen
	#define MSL_IS_SUPERCLASS(T, U) (Conversion<const U*, const T*>::exists && !Conversion<const T*, const void*>::sameType)

	/* MSL_Conversion erkennt zur Compilezeit, ob sich T nach U konvertieren laesst.
	   exists = true, wenn sich T nach U konvertieren laesst, sonst false
	*/
	template<class T, class U>
	class Conversion {

	private:

		// sizeof(Small) = 1
		typedef char Small;
		
		// sizeof(Big) > 1
		class Big {
			char dummy[2];
		};

		// Compiler waehlt diese Funktion, wenn er eine Umwandlung von T nach U findet
		static Small Test(U);
		// sonst nimmer er diese
		static Big Test(...);
		// Erzeugt ein Objekt vom Typ T, selbst wenn der Konstruktor als private deklariert wurde
		static T MakeT();

	public:

		enum {
			exists = sizeof(Test(MakeT())) == sizeof(Small)
		};

		enum {
			sameType = false
		};

	};

	/* Überladung von MSL_Conversion, um den Fall T = U zu erkennen
	*/
	template<class T>
	class Conversion<T, T> {

	public:

		enum {
			exists = true, sameType = true
		};

	};

	/* MSL_Int2Type erzeugt aus ganzzahligen Konstanten einen eigenen Typ. Wird
	   benoetigt, damit der Compiler zur Compilezeit die korrekte MSL_Send Methode
	   auswaehlen kann.
	*/
	template<int v>
	struct Int2Type {

		enum {
			value = v
		};

	};

}

#endif

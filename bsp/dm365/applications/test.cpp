/*
 * File      : test.cpp
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006, RT-Thread Development Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Change Logs:
 * Date           Author		Notes
 * 2011-01-13     weety		first version
 */


/**
 * @addtogroup dm365
 */
/*@{*/

#include <rtthread.h>
//#include <rtdevice.h>
#include <stdio.h>

#include <iostream>  
#include <string>  
#include <bitset>  
#include <typeinfo>  
#include <vector>  
#include <stdexcept>  
  
using namespace std;  

#define NO_IOSTREAM 1
  
//◊‘∂®“Â≈‰÷√∆˜£¨vector∑÷≈‰ø’º‰ π”√  
template<class _Ty>  
class stingyallocator : public allocator<_Ty>  
{  
public:  
   template <class U>  
    struct rebind {  
          typedef stingyallocator<U> other;  
    };  
  
   size_t max_size( ) const  
   {  
         return 10;  
   };  
};  
  
int cxx_exp_main(void)  
{  
    //¬ﬂº≠¥ÌŒÛ£∫out_of_range  
    try {  
      string str( "Micro" );  
      string rstr( "soft" );  
      str.append( rstr, 5, 3 );  
	#ifdef NO_IOSTREAM
	  printf("%s\n", str.c_str());
	#else
      cout << str << endl;  
    #endif
   }  
   catch ( exception &e ) {  
   	#ifdef NO_IOSTREAM
   	  printf("Caught:%s\n", e.what( ));
	  printf("Type: %s\n", typeid( e ).name( ));
	#else
      cerr << "Caught: " << e.what( ) << endl;  
      cerr << "Type: " << typeid( e ).name( ) << endl << endl;  
	#endif
   };  
  
   //¬ﬂº≠¥ÌŒÛ£∫length_error  
   try  
   {  
      vector<int, stingyallocator< int > > myv;  
      for ( int i = 0; i < 11; i++ )  
        myv.push_back( i );  
   }  
   catch ( exception &e )  
   {  
   #ifdef NO_IOSTREAM
   	  printf("Caught:%s\n", e.what( ));
	  printf("Type: %s\n", typeid( e ).name( ));
	#else
      cerr << "Caught " << e.what( ) << endl;  
      cerr << "Type " << typeid( e ).name( ) << endl << endl; 
	#endif
   };  
  
   //¬ﬂº≠¥ÌŒÛ£∫invalid_argument  
   try  
   {  
      bitset< 32 > bitset( string( "11001010101100001b100101010110000") );  
   }  
   catch ( exception &e )  
   {  
   #ifdef NO_IOSTREAM
   	  printf("Caught:%s\n", e.what( ));
	  printf("Type: %s\n", typeid( e ).name( ));
	#else
      cerr << "Caught " << e.what( ) << endl;  
      cerr << "Type " << typeid( e ).name( ) << endl << endl;  
	#endif
   };  
  
   //¬ﬂº≠¥ÌŒÛ£∫domain_error  
   try  
   {  
      throw domain_error( "Your domain is in error!" );  
   }  
   catch (exception &e)  
   {  
   #ifdef NO_IOSTREAM
   	  printf("Caught:%s\n", e.what( ));
	  printf("Type: %s\n", typeid( e ).name( ));
	#else
      cerr << "Caught: " << e.what( ) << endl;  
      cerr << "Type: " << typeid(e).name( ) << endl << endl;  
	#endif
   };  
  
   //‘À–– ±¥ÌŒÛ£∫range_error  
    try  
   {  
      throw range_error( "The range is in error!" );  
   }  
   catch (exception &e)  
   {  
   #ifdef NO_IOSTREAM
   	  printf("Caught:%s\n", e.what( ));
	  printf("Type: %s\n", typeid( e ).name( ));
	#else
      cerr << "Caught: " << e.what( ) << endl;  
      cerr << "Type: " << typeid( e ).name( ) << endl << endl << endl;  
	#endif
   };  
  
   //‘À–– ±¥ÌŒÛ£∫underflow_error  
   try  
   {  
      throw underflow_error( "The number's a bit small, captain!" );  
   }  
   catch ( exception &e ) {  
   	#ifdef NO_IOSTREAM
   	  printf("Caught:%s\n", e.what( ));
	  printf("Type: %s\n", typeid( e ).name( ));
	#else
      cerr << "Caught: " << e.what( ) << endl;  
      cerr << "Type: " << typeid( e ).name( ) << endl << endl;  
	#endif
   };  
  
    //‘À–– ±¥ÌŒÛ£∫overflow_error  
    try  
    {  
        bitset< 33 > bitset;  
        bitset[32] = 1;  
        bitset[0] = 1;  
        unsigned long x = bitset.to_ulong( );  
    }  
    catch(exception &e)  
    {  
    #ifdef NO_IOSTREAM
   	  printf("Caught:%s\n", e.what( ));
	  printf("Type: %s\n", typeid( e ).name( ));
	#else
        cerr << "Caught " << e.what() << endl;  
        cerr << "Type: " << typeid(e).name() << endl << endl;  
    #endif
    }  
  
    return 0;  
}

void *__dso_handle = NULL;


extern "C" 
{
int cxx_excep_test(int argc, char *argv[])
{
	cxx_exp_main();
}

}

/*@}*/

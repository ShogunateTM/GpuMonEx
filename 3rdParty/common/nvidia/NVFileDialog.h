#ifndef _NVFILEDIALOG_H_
#define _NVFILEDIALOG_H_

#pragma warning (disable : 4786)
#include <string>
#pragma warning (disable : 4786)
#include <vector>
#pragma warning (disable : 4786)

 
////////////
// Helper class to assist in loading files
//
// Usage :
//
// Just create a NV*FileDialog object on the stack, and call Open
//
//  NVXFileDialog aDialog;
//
//  std::string theFileName;
//  
//  if ( aDialog.Open( theFileName ) )
//  {
//      // open the filename and read it in
//  }
//
//  // That's it !
//
// Use the NVTextureFileDialog for texture files,
//
// or use the NVFileDialog to do arbitrary filters
//

class NVFileDialog
{
	private :

		OPENFILENAME mOpenFileName;

		std::vector< std::string > mFilterNames;
		std::vector< std::string > mFilterStrings;

		std::string mString;

		void Init()
		{
			memset( &mOpenFileName, 0x00, sizeof( mOpenFileName ) );
			mOpenFileName.lStructSize = sizeof( mOpenFileName );

			OSVERSIONINFO osinfo;
			memset( &osinfo, 0x00, sizeof( osinfo ) );
			BOOL bSuccess = ::GetVersionEx( &osinfo );

			if ( osinfo.dwMajorVersion >= 0x0500 )
			{
				mOpenFileName.lStructSize += ( 3 * sizeof( DWORD ) );
			}

			mString.erase( mString.begin(), mString.end() );

			mOpenFileName.Flags = OFN_FILEMUSTEXIST | OFN_LONGNAMES | OFN_SHAREAWARE;

			mOpenFileName.nFilterIndex = 1;

			for ( int i = 0; i < mFilterNames.size(); ++i )
			{
				mString += mFilterNames[ i ];
				mString += TCHAR(0x00);
				mString += mFilterStrings[ i ];
				mString += TCHAR(0x00);
			}

			// Last element must be double terminated
			mString += TCHAR(0x00);

			mOpenFileName.lpstrFilter = mString.c_str();
		}

	public :

	~NVFileDialog(){;}

	NVFileDialog()
	{
		mFilterNames.push_back("*.*");
		mFilterStrings.push_back("");
	}

	void SetFilters( const std::vector< std::string >& theFilterNames,
					 const std::vector< std::string >& theFilterStrings )
	{
		assert( mFilterNames.size() == theFilterStrings.size() );

		mFilterNames   = theFilterNames;
		mFilterStrings = theFilterStrings;
	}

	void SetFilter( const std::string& theFilterName )
	{
		mFilterNames.clear();
		mFilterStrings.clear();

		mFilterNames.push_back( theFilterName );
		mFilterStrings.push_back( theFilterName );
	}

	virtual bool Open( std::string& theResult )
	{
		Init();

		theResult.resize(1024);
		theResult[0] = 0;
		mOpenFileName.lpstrFile  = &theResult[ 0 ];
		mOpenFileName.nMaxFile   = 1024;

		BOOL bSuccess = ::GetOpenFileName( &mOpenFileName );

		return ( bSuccess == TRUE );
	}

};

class NVXFileDialog : public NVFileDialog
{
	public :

	NVXFileDialog()
	{
		SetFilter("*.x");
	}

};


class NVTextureFileDialog : public NVFileDialog
{
	public :

	NVTextureFileDialog()
	{
		SetFilter("*.bmp;*.tga;*.dds");
	}

};

#endif  _NVFILEDIALOG_H_


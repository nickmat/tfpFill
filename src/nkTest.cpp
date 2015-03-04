/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Name:        nkRefDocuments.cpp
 * Project:     tfpnick: Private utility to create Matthews TFP database
 * Purpose:     Read and process Reference Document files
 * Author:      Nick Matthews
 * Modified by:
 * Created:     23rd September 2011
 * RCS-ID:      $Id$
 * Copyright:   Copyright (c) 2011, Nick Matthews.
 * Licence:     GNU GPLv3
 *
 *  tfpnick is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  tfpnick is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with tfpnick.  If not, see <http://www.gnu.org/licenses/>.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

*/

#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <wx/filename.h>
#include <wx/dir.h>

#include <rec/recDb.h>

#include "nkMain.h"

int DoTest( const wxString& fname )
{
    recInitialize();

    if( !recDb::OpenDb( fname ) ) {
        wxPrintf( "Unable to open [%s]\n", fname );
        return EXIT_FAILURE;
    }
    Filenames fns;
    ProcessRefFile( "C:\\Users\\Nick\\Projects\\Family\\Web\\rd01", "rd00393.htm", fns );

    recUninitialize();


#if 0
    recDb::SetDb( new wxSQLite3Database() );
    if( !recDb::OpenDb( fname ) ) {
        wxPrintf( "Unable to open [%s]\n", fname );
        return EXIT_FAILURE;
    }
    wxPrintf( "Input file [%s] opened\n", fname );
    // Look for missing ref numbers
    wxSQLite3ResultSet result = recReference::GetTitleList();

    int last = result.GetInt( 0 );
    int ref, cnt=0;
    wxPrintf( "First ref = %d\n", last );
    --last;
    while( result.NextRow() ) {
        ref = result.GetInt( 0 );
        while( last+1 < ref ){
            wxPrintf( "Missing %d\n", last+1 );
            cnt++;
            last++;
        }
        last = ref;
    }
    wxPrintf( "Missing ref = %d\n", cnt );
#endif
    return EXIT_SUCCESS;
}

// End of nkTest.cpp file 

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Name:        src/fiCommon.cpp
 * Project:     fill: Private utility to create Matthews TFP database
 * Purpose:     Copy Common Data into database.
 * Author:      Nick Matthews
 * Website:     http://thefamilypack.org
 * Created:     10th August 2022
 * Copyright:   Copyright (c) 2022, Nick Matthews.
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

#include "fiCommon.h"

#include <rec/recDb.h>

#include <wx/filename.h>

void TransferCommonData( const wxString& CommonData )
{
    wxPrintf( " Done\nTransfer Common UK Sources" );
    wxFileName extfname( CommonData );
    wxString extdb = extfname.GetName();
    bool ret = recDb::OpenExternalDb( CommonData, extdb );
    recIdVec refIDs = recReference::GetReferenceIDs( extdb );
    if( ret ) {
        for( idt refID : refIDs ) {
            recReference::Transfer( refID, extdb, "Main", 0 );
            wxPrintf( "." );
        }
    }
}

// End of src/fiMedia.cpp file

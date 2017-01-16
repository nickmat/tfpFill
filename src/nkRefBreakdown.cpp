/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Name:        nkRefBreakdown.cpp
 * Project:     tfpnick: Private utility to create Matthews TFP database
 * Purpose:     Read a Reference breakdown file and create Reference Entities.
 * Author:      Nick Matthews
 * Website:     http://thefamilypack.org
 * Created:     13th October 2011
 * Copyright:   Copyright (c) 2011 ~ 2017, Nick Matthews.
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

#include <rec/recDb.h>

#include "fiRefMarkup.h"

//###########################################################################
//##===================[ Process RD files with markup ]====================##
//###########################################################################

void ProcessMarkupRef( idt refID, wxXmlNode* root )
{
    const wxString savepoint = recDb::GetSavepointStr();
    recDb::Savepoint( savepoint );

    fiRefMarkup markup( refID, root );
    bool ok = markup.create_records();
    if( ok ) {
        recDb::ReleaseSavepoint( savepoint );
    } else {
        recDb::Rollback( savepoint );
    }
}

// End of nkRefBreakdown.cpp file

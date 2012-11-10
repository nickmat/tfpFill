/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Name:        nkRefBreakdown.cpp
 * Project:     tfpnick: Private utility to create Matthews TFP database
 * Purpose:     Read a Reference breakdown file and create Reference Entities.
 * Author:      Nick Matthews
 * Modified by:
 * Created:     13th October 2011
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

#include <wx/textfile.h>
#include <wx/tokenzr.h>

#include <rec/recDb.h>

#include "nkMain.h"

idt CreateDate( idt refID, const wxString& line, int* pseq )
{
    if( refID == 0 ) return 0;
    recDate date;
    date.SetDefaults();
    date.SetDate( line );
    date.Save();
    recReferenceEntity::Create( refID, recReferenceEntity::TYPE_Date, date.f_id, pseq );
    return date.f_id;
}

idt CreateMainEvent( idt refID, idt dateID, idt placeID, const wxString& line, int* pseq )
{
    if( refID == 0 ) return 0;
    wxStringTokenizer tk( line, ":", wxTOKEN_RET_EMPTY );
    wxString typeStr = tk.GetNextToken();
    wxString title = tk.GetNextToken();

    recEventType::EType type;
    if( typeStr == "Marriage" ) {
        type = recEventType::ET_Marriage;
    } else if( typeStr == "Birth" ) {
        type = recEventType::ET_Birth;
    } else if( typeStr == "Death" ) {
        type = recEventType::ET_Death;
    } else {
        type = recEventType::ET_Unstated;
    }

    recEvent ev(0);
    ev.f_title = title;
    ev.f_type_id = type;
    ev.f_date1_id = dateID;
    ev.f_place_id = placeID;
    ev.Save();
    recReferenceEntity::Create( refID, recReferenceEntity::TYPE_Event, ev.f_id, pseq );

    return ev.f_id;
}

idt CreatePersona( idt refID, const wxString& line )
{
    if( refID == 0 ) return 0;
    wxStringTokenizer tk( line, ":", wxTOKEN_RET_EMPTY );
    wxString sexStr = tk.GetNextToken();
    wxString indNum = tk.GetNextToken();

    Sex sex;
    int ch = sexStr[0];
    switch( ch )
    {
    case 'M': sex = SEX_Male; break;
    case 'F': sex = SEX_Female; break;
    case 'U': sex = SEX_Unknown; break;
    default: sex = SEX_Unstated;
    }
    recPersona pa(0);
    pa.f_sex = sex;
    pa.f_ref_id = refID;
    pa.Save();

    idt indID;
    if( !indNum.IsEmpty() ) { 
        indID = recGetID( indNum.Mid( 1 ) );
        if( indID ) {
            recLinkPersona lp(0);
            lp.f_ind_per_id = recIndividual::GetPersona( indID );
            lp.f_ref_per_id = pa.f_id;
            lp.f_conf = 0.999;
            lp.Save();
        }
    }
    return pa.f_id;
}

void CreateName( idt refID, idt perID, const wxString& line, int* pseq )
{
    if( refID == 0 || perID == 0 ) return;
    wxStringTokenizer tk( line, ":", wxTOKEN_RET_EMPTY );
    wxString styleStr = tk.GetNextToken();
    wxString nameStr = tk.GetNextToken();

    recNameStyle::Style style = recNameStyle::NS_Default;
    int nameSeq = 1;
    if( styleStr == "Maiden" ) {
        style = recNameStyle::NS_Birth;
        nameSeq = 2;
    } else if( styleStr == "Married" ) {
        style = recNameStyle::NS_Married;
        nameSeq = 3;
    }

    recName name(0);
    name.f_per_id = perID;
    name.f_style_id = style;
    name.f_sequence = nameSeq;
    name.Save();
    name.AddNameParts( nameStr );
    recReferenceEntity::Create( refID, recReferenceEntity::TYPE_Name, name.f_id, pseq );
}

void AddEventRole( idt evID, idt perID, const wxString& line, idt dateID )
{
    if( evID == 0 || perID == 0 ) return;

    recEventTypeRole::Role role;
    if( line == "Groom" ) {
        role = recEventTypeRole::ROLE_Marriage_Groom;
    } else if( line == "Bride" ) {
        role = recEventTypeRole::ROLE_Marriage_Bride;
    } else if( line == "FofGroom" ) {
        role = recEventTypeRole::ROLE_Marriage_FatherOfGroom;
    } else if( line == "FofBride" ) {
        role = recEventTypeRole::ROLE_Marriage_FatherOfBride;
    } else if( line == "Witness" ) {
        role = recEventTypeRole::ROLE_Marriage_Witness;
    } else if( line == "Officiate" ) {
        role = recEventTypeRole::ROLE_Marriage_Officiate;
    } else if( line == "Died" ) {
        role = recEventTypeRole::ROLE_Death_Died;
    } else if( line == "Born" ) {
        role = recEventTypeRole::ROLE_Birth_Born;
    } else {
        role = recEventTypeRole::ROLE_Unstated;
    }

    recEventPersona ep(0);
    ep.f_event_id = evID;
    ep.f_per_id = perID;
    ep.f_role_id = role;
    ep.f_per_seq = recEvent::GetLastPerSeqNumber( evID ) + 1;
    ep.Save();
}
#if 0
void CreateAttr( idt refID, idt perID, const wxString& line, idt dateID, int* pseq )
{
    if( refID == 0 || perID == 0 ) return;
    wxStringTokenizer tk( line, ":", wxTOKEN_RET_EMPTY );
    wxString typeStr = tk.GetNextToken();
    wxString value = tk.GetNextToken();

    recAttributeType::AType type = recAttributeType::ATYPE_Unstated;
    if( typeStr == "Occ" ) {
        type = recAttributeType::ATYPE_Occupation;
    }

    recAttribute at(0);
    at.f_per_id = perID;
    at.f_type_id = type;
    at.f_val = value;
    at.f_sequence = recDate::GetDatePoint( dateID );
    at.Save();
    recReferenceEntity::Create( refID, recReferenceEntity::TYPE_Attribute, at.f_id, pseq );
}
#endif
void CreateSubEvent( idt refID, idt perID, idt baseID, const wxString& line, int* pseq )
{
    if( refID == 0 || perID == 0 ) return;
    wxStringTokenizer tk( line, ":", wxTOKEN_RET_EMPTY );
    wxString typeStr = tk.GetNextToken();
    wxString dateStr = tk.GetNextToken();
    wxString placeStr = tk.GetNextToken();

    recEventType::EType type = recEventType::ET_Unstated;
    wxString title;
    recDate::DatePoint datePt = recDate::DATE_POINT_Mid;
    recEventTypeRole::Role role = recEventTypeRole::ROLE_Unstated;
    if( typeStr == "Birth" ) {
        type = recEventType::ET_Birth;
        title = wxString::Format( "Birth of %s", recPersona::GetNameStr( perID ) );
        datePt = recDate::DATE_POINT_Beg;
        role = recEventTypeRole::ROLE_Birth_Born;
    } 
    
    idt dateID = 0;
    if( !dateStr.IsEmpty() ) {
        CalendarUnit unit = CALENDAR_UNIT_Unstated;
        recDate date(0);
        date.SetDefaults();
        switch( (int) dateStr[0] )
        {
        case 'y': unit = CALENDAR_UNIT_Year;  break;
        case 'm': unit = CALENDAR_UNIT_Month; break;
        case 'w': unit = CALENDAR_UNIT_Week;  break;
        case 'd': unit = CALENDAR_UNIT_Day;   break;
        }
        if( unit == CALENDAR_UNIT_Unstated ) {
            // Assume it's a normal date
            date.SetDate( dateStr );
        } else {
            long age;
            recRelativeDate rel;
            rel.SetDefaults();
            if( !dateStr.Mid( 1 ).ToLong( &age ) ) {
                age = 0;
            }
            rel.f_val = age;
            rel.f_unit = unit;
            rel.f_base_id = baseID;
            rel.f_type = recRelativeDate::TYPE_AgeRoundDown;
            rel.Save();
            date.f_rel_id = rel.f_id;
            date.Update();
        }
        date.Save();
        dateID = date.f_id;
    }

    idt placeID = 0;
    if( !placeStr.IsEmpty() ) {
        placeID = CreatePlace( placeStr, refID, pseq );
    }

    recEvent ev(0);
    ev.f_title = title;
    ev.f_type_id = type;
    ev.f_date1_id = dateID;
    ev.f_place_id = placeID;
    ev.f_date_pt = recDate::GetDatePoint( dateID, datePt );
    ev.Save();
    idt evID = ev.f_id;
    recReferenceEntity::Create( refID, recReferenceEntity::TYPE_Event, evID, pseq );
    
    recEventPersona ep(0);
    ep.f_event_id = evID;
    ep.f_per_id = perID;
    ep.f_role_id = role;
    ep.f_per_seq = 1;
    ep.Save();
}


bool InputRefBreakdownFile( const wxString& refFile )
{
    wxTextFile rf( refFile );
    if( rf.Open() ) {
        idt refID, dateID, placeID, evID, perID;
        int seq;
        for ( wxString line = rf.GetFirstLine() ; !rf.Eof() ; line = rf.GetNextLine() ) {
            if( line.Left( 2 ) == "R:" ) {
                refID = recGetID( line.Mid( 3 ) );
                dateID = placeID = evID = perID = 0;
                seq = 0;
            } else if( line.Left( 2 ) == "D:" ) {
                dateID = CreateDate( refID, line.Mid( 2 ), &seq );
            } else if( line.Left( 2 ) == "P:" ) {
                placeID = CreatePlace( line.Mid( 2 ), refID, &seq );
            } else if( line.Left( 2 ) == "E:" ) {
                if( evID == 0 ) {
                    evID = CreateMainEvent( refID, dateID, placeID, line.Mid( 2 ), &seq );
                } else {
                    CreateSubEvent( refID, perID, dateID, line.Mid( 2 ), &seq );
                }
            } else if( line.Left( 3 ) == "Pa:" ) {
                perID = CreatePersona( refID, line.Mid( 3 ) );
            } else if( line.Left( 2 ) == "N:" ) {
                CreateName( refID, perID, line.Mid( 2 ), &seq );
            } else if( line.Left( 3 ) == "EP:" ) {
                AddEventRole( evID, perID, line.Mid( 3 ), dateID );
//            } else if( line.Left( 2 ) == "A:" ) {
//                CreateAttr( refID, perID, line.Mid( 2 ), dateID, &seq );
            }
        }
    }
    return true;
}


// End of nkRefBreakdown.cpp file 

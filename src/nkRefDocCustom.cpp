/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Name:        nkRefDocuments.cpp
 * Project:     tfpnick: Private utility to create Matthews TFP database
 * Purpose:     Read and process Reference Document files
 * Author:      Nick Matthews
 * Website:     http://thefamilypack.org
 * Created:     23rd September 2011
 * Copyright:   Copyright (c) 2011..2022, Nick Matthews.
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

#include <wx/dir.h>
#include <wx/textfile.h>
#include <wx/tokenzr.h>

#include <rec/recDb.h>

#include "nkMain.h"
#include "xml2.h"

typedef std::vector< wxFileName > Filenames;

enum RecordType {
    RT_UNKNOWN,
    RT_Date,
    RT_Place,
    RT_Individual,
    RT_Reference,
    RT_MAX
};

wxString MakeIntoLink( const wxString& text, RecordType type, idt id )
{
    // Remove any end spaces to tail
    wxString tail;
    wxString::const_iterator it = text.end();
    while( it != text.begin() && *--it == ' ' ) {
        tail << ' ';
    }
    if( it == text.begin() ) {
        return text; // It's all spaces
    }
    wxString body = text.substr( 0, text.size() - tail.size() );

    // Construct result
    wxString result;
    it = body.begin();
    while( it != body.end() && *it == ' ' ) {
        result << *it++; // Skip spaces 
    }
    result << "<a href='";
    switch( type )
    {
    case RT_UNKNOWN:
        return text;
    case RT_Date:
        result << "tfpi:D";
        break;
    case RT_Place:
        result << "tfpi:P";
        break;
    case RT_Individual:
        result << "tfp:I";
        break;
    case RT_Reference:
        result << "tfp:R";
        break;
    default:
        wxASSERT( false ); // Shouldn't be here
        return text;
    }
    result << id << "'>";
    while( it != body.end() ) {
        result << *it++;
    }
    result << "</a>" << tail;

    return result;
}

wxString GetDateStrQuarterPlus( long year, long quarter )
{
    switch( quarter ) 
    {
    case 1:
        return wxString::Format( "1 Dec %ld ~ 31 Mar %ld", year-1, year );
    case 2:
        return wxString::Format( "1 Mar %ld ~ 30 Jun %ld", year, year );
    case 3:
        return wxString::Format( "1 Jun %ld ~ 30 Sep %ld", year, year );
    case 4:
        return wxString::Format( "1 Sep %ld ~ 31 Dec %ld", year, year );
    }
    return wxEmptyString;
}

wxString GetDateStrQuarter( long year, long quarter )
{
    switch( quarter ) 
    {
    case 1:
        return wxString::Format( "1 Jan %ld ~ 31 Mar %ld", year, year );
    case 2:
        return wxString::Format( "1 Apr %ld ~ 30 Jun %ld", year, year );
    case 3:
        return wxString::Format( "1 Jul %ld ~ 30 Sep %ld", year, year );
    case 4:
        return wxString::Format( "1 Oct %ld ~ 31 Dec %ld", year, year );
    }
    return wxEmptyString;
}

wxString MonthsStr[] =
{
    "Dec", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

wxString GetDateStrMonth( long year, long month )
{
    wxASSERT( month > 0 && month <= 12 );
    return wxString::Format( "%s %ld", MonthsStr[month], year );
}

wxString GetDateStrMonthPlus( long year, long month )
{
    wxASSERT( month > 0 && month <= 12 );
    return wxString::Format( "%s %ld ~ %s %ld",
        MonthsStr[month-1], (month == 1) ? year-1 : year, 
        MonthsStr[month], year
    );
}

wxString GetDateStrDay( long year, long month, long day )
{
    if( day == 0 ) {
        if( month == 0 ) {
            return wxString::Format( "%ld", year );
        }
        wxASSERT( month > 0 && month <= 12 );
        return wxString::Format( "%s %ld", MonthsStr[month], year );
    }
    wxASSERT( month > 0 && month <= 12 );
    return wxString::Format( "%ld %s %ld", day, MonthsStr[month], year );
}

idt GetIdFromHref( const wxString& href )
{
    // href="../ps01/ps01_459.htm"
    long dir, file;
    if( !href.Mid( 11, 2 ).ToLong( &dir ) ) return 0;
    if( !href.Mid( 19, 3 ).ToLong( &file ) ) return 0;
    if( dir < 1 ) return 0;
    return ( dir - 1 ) * 500 + file;
}

bool TidyStr( wxString* str )
{
    while( !str->IsEmpty() && !wxIsalnum( str->Last() ) ) {
        str->Truncate( str->length() - 1 );
    }
    return !str->IsEmpty();
}

struct C161 
{ 
    int distL;
    int mothB;
    int mothL;
    int indStart;
};

enum RecSource {
    RECSOURCE_none, RECSOURCE_FreeBMD, RECSOURCE_FindMyPast, RECSOURCE_Ancestry
};

wxString FindSource( const wxString& line, size_t* pos )
{
    wxString rs = "\n\n<b>Source</b>: ";
    *pos = line.find( "[A]" );
    if( *pos != wxString::npos ) {
        return rs + "Ancestry.co.uk";
    }
    *pos = line.find( "[O]" );
    if( *pos != wxString::npos ) {
        return rs + "FindMyPast.co.uk";
    }
    *pos = line.find( "[F]" );
    if( *pos != wxString::npos ) {
        return rs + "FreeBMD.co.uk";
    }
    return "";
}

void Process161File( wxFileName& fn )
{
    C161 c161[] = {
        { 14, 68, 12, 115 },
        { 13, 70, 11, 115 },
        { 12, 77, 10, 121 }
    };
    wxTextFile file( fn.GetFullPath() );
    file.Open();
    long year, quarter, month;
    long rDatePt, bDatePt;
    wxString str, placeStr, heading;
    idt indID, perID, rDateID, bDateID, placeID, perMotherID, rEveID, bEveID;
    int block = -1;
    recReference ref;
    int seq;
    for( wxString line = file.GetFirstLine() ; !file.Eof() ; line = file.GetNextLine() ) {
        if( line.IsEmpty() ) continue;
        wxChar ch = line[0];
        if( ch == '[' ) break;
        if( ch == ' ' || ch == '<' ) continue;
        if( line.Mid( 0, 4 ) == "Year" ) {
            heading = "<pre><b>\n" + line + "\n\n";
            block++;
            if( block > 2 ) break;
            continue;
        }
        if( !wxIsdigit( ch ) ) continue;
        if( block < 0 ) continue;
        // Create a new reference from this line
        ref.Clear();
        ref.Save();
        ref.FSetUserRef( "RD161" );
        seq = 0;
        str = line.Mid( 21, 22 ) + line.Mid( 8, 13 );
        size_t start = line.find( ">P", c161[block].indStart );
        line.Mid( start+2, 4 ).ToLongLong( &indID ); 
        perID = CreatePersona( ref.f_id, indID, str, SEX_Unstated, &seq );
        
        line.Mid( 0, 4 ).ToLong( &year );
        line.Mid( 5, 1 ).ToLong( &quarter );
        line.Mid( 5, 2 ).ToLong( &month );
        if( block < 1 ) {
            str = GetDateStrQuarter( year, quarter );
        } else {
            str = GetDateStrMonth( year, month );
        }
        rDateID = CreateDate( str, ref.f_id, &seq );

        placeStr = line.Mid( 43, c161[block].distL );
        str = line.Mid( 43, c161[block].distL ).Trim() + " (RD)";
        placeID = CreatePlace( str, ref.f_id, &seq );

        rEveID = CreateRegBirthEvent( ref.f_id, perID, rDateID, placeID );

        if( block < 1 ) {
            str = GetDateStrQuarterPlus( year, quarter );
        } else {
            str = GetDateStrMonthPlus( year, month );
        }
        bDateID = CreateDate( str, ref.f_id, &seq );
        bEveID = CreateBirthEvent( ref.f_id, perID, bDateID, placeID );

        str = line.Mid( c161[block].mothB, c161[block].mothL ).Trim();
        if( !str.empty() ) {
            idt indMotherID = 0;
            recFamilyVec fams = recIndividual::GetParentList( indID );
            if( !fams.empty() ) {
                indMotherID = fams[0].FGetWifeID();
            }
            perMotherID = CreatePersona( ref.f_id, indMotherID, "? "+str, SEX_Female, &seq, "Mother" );
            rDatePt = recDate::GetDatePoint( rDateID, recDate::DatePoint::beg );
            AddPersonaToEventa( rEveID, perMotherID, 
                recEventTypeRole::ROLE_RegBirth_Parent );
            bDatePt = recDate::GetDatePoint( bDateID, recDate::DatePoint::beg );
            AddPersonaToEventa( bEveID, perMotherID, 
                recEventTypeRole::ROLE_Birth_Mother );
            CreateRelationship( perMotherID, "Mother", perID, ref.f_id, &seq );
        }

        size_t pos = wxString::npos;
        wxString rs = FindSource( line, &pos );
        size_t pos1 = line.find( "../or" );
        size_t pos2;
        if( pos1 != wxString::npos ) {
            pos2 = line.find( "</span>", pos1 ) + 7;
            while( line.length() > pos2 && line.at( pos2 ) == ' ' ) {
                pos2++;
            }
            line = line.substr( 0, pos1-22 ) + line.substr( pos2 );
        } else if( pos != wxString::npos ) {
            pos2 = pos+3;
            while( line.length() > pos2 && line.at( pos2 ) == ' ' ) {
                pos2++;
            }
            line = line.substr( 0, pos ) + line.substr( pos2 );
        }

        pos1 = line.find( "../ps" );
        wxASSERT( pos1 != wxString::npos );
        pos2 = line.find( "</a>", pos1 ) + 4;
        while( line.length() > pos2 && line.at( pos2 ) == ' ' ) {
            pos2++;
        }
        line = line.substr( 0, pos1-9 ) + line.substr( pos2 );

        pos1 = line.find( "../rd" );
        if( pos1 != wxString::npos ) {
            idt noteRefID;
            line.Mid( pos1+10, 5 ).ToLongLong( &noteRefID );
            wxString str = "R" + recGetIDStr( noteRefID );
            wxString refStr = MakeIntoLink( str, RT_Reference, noteRefID );
            pos2 = line.find( "</a>", pos1 );
            line = line.substr( 0, pos1-9 ) + refStr + line.substr( pos2+4 );
        }

        line.Trim();
        ref.f_title << year << " GRO Birth Index for " << recPersona::GetNameStr( perID );
        ref.f_statement 
            << "<!-- HTML -->\n" << heading 
            << MakeIntoLink( line.substr( 0, 8 ), RT_Date, rDateID )
            << MakeIntoLink( line.substr( 8, 12 ), RT_Individual, indID )
            << line.substr( 20, 23 ) // Forename
            << MakeIntoLink( placeStr, RT_Place, placeID )
            << line.substr( 43+c161[block].distL )
            << rs
            << "\n\n</pre>\n"
        ;
        ref.Save();
    }
}

void Process162File( wxFileName& fn )
{
    C161 c162[] = {
        { 14, 0, 0, 102 },
        { 14, 69, 14, 118 },
        { 14, 74, 14, 123 },
        { 14, 79, 10, 123 }
    };
    wxTextFile file( fn.GetFullPath() );
    file.Open();
    long year, quarter, month;
    long datePt;
    wxString str, heading;
    idt indID, perID, dateID, placeID, perSpouseID, eveID;
    int block = -1;
    recReference ref;
    int seq;
    for( wxString line = file.GetFirstLine() ; !file.Eof() ; line = file.GetNextLine() ) {
        if( line.IsEmpty() ) continue;
        wxChar ch = line[0];
        if( ch == '[' ) break;
        if( ch == ' ' || ch == '<' ) continue;
        if( line.Mid( 0, 4 ) == "Year" ) {
            heading = "<pre><b>\n" + line + "\n\n";
            block++;
            if( block > 3 ) break;
            continue;
        }
        if( !wxIsdigit( ch ) ) continue;
        if( block < 0 ) continue;
        // Create a new reference from this line
        ref.Clear();
        ref.Save();
        ref.FSetUserRef( "RD162" );
        seq = 0;
        str = line.Mid( 21, 22 ) + line.Mid( 8, 13 );
        size_t start = line.find( ">P", c162[block].indStart );
        line.Mid( start+2, 4 ).ToLongLong( &indID ); 
        perID = CreatePersona( ref.f_id, indID, str, SEX_Unstated, &seq );
        
        line.Mid( 0, 4 ).ToLong( &year );
        line.Mid( 5, 1 ).ToLong( &quarter );
        line.Mid( 5, 2 ).ToLong( &month );
        if( block < 2 ) {
            str = GetDateStrQuarter( year, quarter );
        } else {
            str = GetDateStrMonth( year, month );
        }
        dateID = CreateDate( str, ref.f_id, &seq );

        str = line.Mid( 43, 14 ).Trim() + " (RD)";
        placeID = CreatePlace( str, ref.f_id, &seq );

        recEventTypeRole::Role role = recEventTypeRole::ROLE_Marriage_Spouse;
        eveID = CreateMarriageEvent( perID, dateID, placeID, role, ref.f_id );

        if( block > 0 ) {
            str = line.Mid( c162[block].mothB, c162[block].mothL ).Trim();
            perSpouseID = CreatePersona( ref.f_id, 0, str, SEX_Unstated, &seq );
            datePt = recDate::GetDatePoint( dateID );
            role = recEventTypeRole::ROLE_Marriage_Spouse;
            AddPersonaToEventa( eveID, perSpouseID, role );
            CreateRelationship( perSpouseID, "Spouse", perID, ref.f_id, &seq );
        }

        ref.f_title << year << " GRO Marriage Index for " << recPersona::GetNameStr( perID );
        ref.f_statement << "<!-- HTML -->\n" << heading << line << "\n\n</pre>\n";
        ref.Save();
    }
}

struct C163 
{ 
    int distB;
    int distL;
    int indStart;
};

void Process163File( wxFileName& fn )
{
    C163 c163[] = {
        { 48, 13, 105 },
        { 56, 12, 112 },
        { 56, 14, 119 },
        { 56, 12, 125 }
    };
    wxTextFile file( fn.GetFullPath() );
    file.Open();
    long year, quarter, month;
    wxString str, heading;
    idt indID, perID, rDateID, dDateID, bDateID, placeID, rEveID, dEveID;
    int block = -1;
    recReference ref;
    int seq;
    for( wxString line = file.GetFirstLine() ; !file.Eof() ; line = file.GetNextLine() ) {
        if( line.IsEmpty() ) continue;
        wxChar ch = line[0];
        if( ch == '[' ) break;
        if( ch == ' ' || ch == '<' ) continue;
        if( line.Mid( 0, 4 ) == "Year" ) {
            heading = "<pre><b>\n" + line + "\n\n";
            block++;
            if( block > 3 ) break;
            continue;
        }
        if( !wxIsdigit( ch ) ) continue;
        if( block < 0 ) continue;
        // Create a new reference from this line
        ref.Clear();
        ref.Save();
        ref.FSetUserRef( "RD163" );
        seq = 0;
        str = line.Mid( 21, 22 ) + " " + line.Mid( 8, 13 );
        size_t start = line.find( ">P", c163[block].indStart );
        line.Mid( start+2, 4 ).ToLongLong( &indID ); 
        perID = CreatePersona( ref.f_id, indID, str, SEX_Unstated, &seq );
        
        line.Mid( 0, 4 ).ToLong( &year );
        line.Mid( 5, 1 ).ToLong( &quarter );
        line.Mid( 5, 2 ).ToLong( &month );
        if( block < 2 ) {
            str = GetDateStrQuarter( year, quarter );
        } else {
            str = GetDateStrMonth( year, month );
        }
        rDateID = CreateDate( str, ref.f_id, &seq );

        str = line.Mid( c163[block].distB, c163[block].distL ).Trim() + " (RD)";
        placeID = CreatePlace( str, ref.f_id, &seq );

        rEveID = CreateRegDeathEvent( ref.f_id, perID, rDateID, placeID );

        if( block < 2 ) {
            str = GetDateStrQuarterPlus( year, quarter );
        } else {
            str = GetDateStrMonthPlus( year, month );
        }
        dDateID = CreateDate( str, ref.f_id, &seq );
        dEveID = CreateDeathEventa( ref.f_id, perID, dDateID, placeID );
        LinkOrCreateEventFromEventa( dEveID );

        if( block == 0 ) {  // This block may state age
            str = line.Mid( 44, 2 );
            if( str != "  " ) {
                long age;
                str.ToLong( &age );
                bDateID = CreateDateFromAge( age, dDateID, ref.f_id, &seq );
                CreateBirthEvent( ref.f_id, perID, bDateID, 0 );
            }
        }

        if( block > 0 ) {  // These blocks give Birth Date
            long y, m, d;
            if( !line.Mid( 44, 4 ).ToLong( &y ) ) y = 0;
            if( !line.Mid( 49, 2 ).ToLong( &m ) ) m = 0;
            if( !line.Mid( 52, 2 ).ToLong( &d ) ) d = 0;
            str = GetDateStrDay( y, m, d );
            bDateID = CreateDate( str, ref.f_id, &seq );
            CreateBirthEvent( ref.f_id, perID, bDateID, 0 );
        }

        ref.f_title << year << " GRO Death Index for " << recPersona::GetNameStr( perID );
        ref.f_statement << "<!-- HTML -->\n" << heading << line << "\n\n</pre>\n";
        ref.Save();
    }
}

void Process1051File( wxFileName& fn )
{
    wxTextFile file( fn.GetFullPath() );
    file.Open();
    bool start = false, entry = false;
    wxString str, heading;
    idt indID, ind1stID, perID, per1stID, dateID, bDateID, placeID, rPlaceID,
        cEveID, cEve1stID, bEveID, rEveID;
    long datePt, bDatePt;
    wxString bDateStr;
    idt famID, parentID, parentNameID, perParentID;
    idt fatherID, fatherNameID, perFatherID;
    idt motherID, motherNameID, motherMaidenID, perMotherID;
    recReference ref;
    int seq;
    wxString rec_loc, address;
    for( wxString line = file.GetFirstLine() ; !file.Eof() ; line = file.GetNextLine() ) {
        if( !start && line.StartsWith( "<table" ) ) {
            rec_loc = line;
            while( !file.Eof() && !line.StartsWith( "</table>" ) ) {
                line = file.GetNextLine();
                rec_loc << "\n" << line;
            }
            address = xmlReadRecLoc( rec_loc );
        }
        if( !start && line.size() > 1 && wxIsdigit( line[1] ) ) {
            start = true;
        }
        if( !start ) continue;
        if( line.size() < 2 || !wxIsdigit( line[1] ) ) continue;
        if( line == "</pre>" ) break;
        // Create a new reference from this line
        ref.Clear();
        ref.Save();
        ref.FSetUserRef( "RD1051" );
        seq = 0;

        dateID = CreateDate( line.Mid( 0, 11 ), ref.f_id, &seq );
        datePt = recDate::GetDatePoint( dateID, recDate::DatePoint::beg );
        if( !address.IsEmpty() ) {
            placeID = CreatePlace( address, ref.f_id, &seq );
        }

        ref.f_statement = "<!-- HTML -->\n" + rec_loc + "<pre>\n\n";
        ref.f_statement << line;
        str = line.Mid( 13 );
        for(;;) {
            line = file.GetNextLine();
            ref.f_statement << "\n" << line;
            if( line.IsEmpty() ) break;
            str << line.Mid( 12 );
        }
        ref.f_statement << "\n</pre>\n";

        wxStringTokenizer tokenizer( str, " <>", wxTOKEN_STRTOK );
        wxString token, name, name1st, parent, father, mother;
        wxString maiden, residence, occupation;
        Sex sex = SEX_Unstated, parentSex = SEX_Unstated;
        famID = parentID = fatherID = motherID = 0;
        indID = ind1stID = perID = per1stID = 0;
        bEveID = 0;
        int state = 0, substate = 0;
        bDateStr = wxEmptyString;
        bool singleParent = true;
        while ( tokenizer.HasMoreTokens() )
        {
            token = tokenizer.GetNextToken();

            switch( state )
            {
            case 0:
                if( token.Mid( 0, 5 ) == "href=" ) {
                    indID = GetIdFromHref( token );
                    break;
                }
                if( token == "b" ) {
                    state = 1;
                }
                break;
            case 1:
                if( token == "/b" ) {
                    state = 2;
                    substate = 0;
                    break;
                }
                if( name.IsEmpty() ) {
                    name = token;
                } else {
                    name << " " << token;
                }
                break;
            case 2:
                switch( substate )
                {
                case 1:
                    bDateStr = token;
                    substate = 2;
                    continue;
                case 2:
                    bDateStr << " " << token << " ";
                    substate = 3;
                    continue;
                case 3:
                    if( wxIsdigit( token[0] ) ) {
                        bDateStr << token;
                        substate = 0;
                        continue;
                    } else {
                        bDateStr << recDate::GetYear( dateID );
                        // Fall thru
                    }
                    substate = 0;
                }
                if( token.Mid( 0, 4 ) == "born" ) {
                    substate = 1;
                    break;
                }
                if( token.Mid( 0, 3 ) == "dau" ) {
                    sex = SEX_Female;
                    break;
                }
                if( token.Mid( 0, 3 ) == "son" ) {
                    sex = SEX_Male;
                    break;
                }
                if( token == "&amp;" || token == "and" ) {
                    name1st = name;
                    ind1stID = indID;
                    name = wxEmptyString;
                    indID = 0;
                    state = 0;
                    break;
                }
                if( token.Mid( 0, 11 ) == "href=\"../wc" ) {
                    famID = GetIdFromHref( token );
                    break;
                }
                if( token.Mid( 0, 11 ) == "href=\"../ps" ) {
                    parentID = GetIdFromHref( token );
                    break;
                }
                if( token == "b" ) {
                    state = 3;
                }
                break;
            case 3:
                if( token == "/b" ) {
                    state = 35;
                    break;
                }
                if( token == "&amp;" || token == "and" ) {
                    state = 4;
                    break;
                }
                if( name.IsEmpty() ) {
                    parent = token;
                } else {
                    parent << " " << token;
                }
                break;
            case 35:
                if( token == "&amp;" || token == "and" ) {
                    state = 4;
                    break;
                }
                if( token == "nee" ) {
                    state = 6;
                    break;
                }
                if( token == "of" ) {
                    state = 7;
                    break;
                }
                break;
            case 4:
                if( token == "/b" || token == "/a" ) {
                    state = 5;
                    break;
                }
                if( token == "&amp;" || token == "b" ) break;
                if( mother.IsEmpty() ) {
                    father = parent;
                    parent = wxEmptyString;
                    mother = token;
                } else {
                    mother << " " << token;
                }
                break;
            case 5:
                if( token == "nee" ) {
                    state = 6;
                    break;
                }
                if( token == "of" ) {
                    state = 7;
                    break;
                }
                if( token == "!--O--" ) {
                    state = 8;
                    break;
                }
                break;
            case 6:
                if( token == "b" ) break;
                maiden = token;
                state = 5;
                break;
            case 7:
                if( token == "!--O--" ) {
                    state = 8;
                    break;
                }
                if( token == "!--N--" ) {
                    state = -1;
                    break;
                }
                if( residence.IsEmpty() ) {
                    residence = token;
                    break;
                }
                residence << " " << token;
                break;
            case 8:
                if( token == "!--N--" ) {
                    state = -1;
                    break;
                }
                if( occupation.IsEmpty() ) {
                    occupation = token;
                    break;
                }
                occupation << " " << token;
                break;
           }

        }
        recFamily fam(famID);
        motherID = fam.f_wife_id;
        fatherID = fam.f_husb_id;
        if( parentID && parentSex == SEX_Female ) {
            if( !parent.IsEmpty() ) {
                mother = parent;
                parent = wxEmptyString;
            }
            motherID = parentID;
            parentID = 0;
        }
        parentNameID = motherNameID = motherMaidenID = fatherNameID = 0;
        perParentID = perMotherID = perFatherID = 0;
        wxString surname;
        if( !parent.IsEmpty() ) {
            parentNameID = CreateName( parent );
            surname = recName::GetNamePartStr( parentNameID, NAME_TYPE_Surname );
        } else if( !mother.IsEmpty() ) {
            motherNameID = CreateName( mother, recNameStyle::NS_Married );
            surname = recName::GetNamePartStr( motherNameID, NAME_TYPE_Surname );
            father << " " << surname;
            fatherNameID = CreateName( father );
            if( !maiden.IsEmpty() ) {
                mother = recName::GetNamePartStr( motherNameID, NAME_TYPE_Given_name )
                    + " " + maiden;
                motherMaidenID = CreateName( mother );
            }
        } else if( !father.IsEmpty() ) {
            fatherNameID = CreateName( father );
            surname = recName::GetNamePartStr( fatherNameID, NAME_TYPE_Surname );
        }
        name << " " << surname;

        if( !name1st.IsEmpty() ) {
            name1st << " " << surname;
            per1stID = CreatePersona( ref.f_id, ind1stID, name1st, sex, &seq );
            cEve1stID = CreateChrisEventa( ref.f_id, per1stID, dateID, placeID );
        } else {
            cEve1stID = 0;
        }
        perID = CreatePersona( ref.f_id, indID, name, sex, &seq ); 
        cEveID = CreateChrisEventa( ref.f_id, perID, dateID, placeID );

        if( !bDateStr.IsEmpty() ) {
            // We have a birth event
            bDateID = CreateDate( bDateStr, ref.f_id, &seq );
            bEveID = CreateBirthEvent( ref.f_id, perID, bDateID, 0 );
        }

        if( parentNameID ) {
            recReferenceEntity::Create( ref.f_id, recReferenceEntity::TYPE_Name, parentNameID, &seq );
            perParentID = CreatePersona( ref.f_id, parentID, parentNameID, SEX_Unstated );
            AddPersonaToEventa( cEveID, perParentID, recEventTypeRole::ROLE_Baptism_Parent );
        }
        if( perParentID ) {
            CreateRelationship( perParentID, "Parent", perID, ref.f_id, &seq );
        }

        if( fatherNameID ) {
            recReferenceEntity::Create( ref.f_id, recReferenceEntity::TYPE_Name, fatherNameID, &seq );
            perFatherID = CreatePersona( ref.f_id, fatherID, fatherNameID, SEX_Male );
            if( per1stID ) {
                AddPersonaToEventa( cEve1stID, perFatherID, recEventTypeRole::ROLE_Baptism_Parent );
            }
            AddPersonaToEventa( cEveID, perFatherID, recEventTypeRole::ROLE_Baptism_Parent );
        }
        if( perFatherID ) {
            if( per1stID ) {
                CreateRelationship( perFatherID, "Father", per1stID, ref.f_id, &seq );
            }
            CreateRelationship( perFatherID, "Father", perID, ref.f_id, &seq );
        }

        if( motherNameID ) {
            if( motherMaidenID ) {
                recReferenceEntity::Create( ref.f_id, recReferenceEntity::TYPE_Name, motherMaidenID, &seq );
                recReferenceEntity::Create( ref.f_id, recReferenceEntity::TYPE_Name, motherNameID, &seq );
                perMotherID = CreatePersona( ref.f_id, motherID, motherMaidenID, SEX_Female );
                rAddNameToPersona( perMotherID, motherNameID );
            } else {
                recReferenceEntity::Create( ref.f_id, recReferenceEntity::TYPE_Name, motherNameID, &seq );
                perMotherID = CreatePersona( ref.f_id, motherID, motherNameID, SEX_Female );
            }
            if( per1stID ) {
                AddPersonaToEventa( cEve1stID, perMotherID, recEventTypeRole::ROLE_Baptism_Parent );
            }
            AddPersonaToEventa( cEveID, perMotherID, recEventTypeRole::ROLE_Baptism_Parent );
        }

        if( perMotherID ) {
            if( per1stID ) {
                CreateRelationship( perMotherID, "Mother", per1stID, ref.f_id, &seq );
            }
            CreateRelationship( perMotherID, "Mother", perID, ref.f_id, &seq );
            if( perFatherID ) {
                CreateRelationship( perFatherID, "Husband", perMotherID, ref.f_id, &seq );
            }
        }

        if( bEveID ) {
            bDatePt = recDate::GetDatePoint( bDateID, recDate::DatePoint::beg );
            if( perMotherID ) {
                AddPersonaToEventa( bEveID, perMotherID, recEventTypeRole::ROLE_Birth_Mother );
            }
        }

        if( TidyStr( &residence ) ) {
            rPlaceID = CreatePlace( residence, ref.f_id, &seq );
            rEveID = CreateResidenceEventa( dateID, rPlaceID, ref.f_id );
            if( perParentID ) {
                AddPersonaToEventa( rEveID, perParentID, recEventTypeRole::ROLE_Residence_Family );
            }
            if( perFatherID ) {
                AddPersonaToEventa( rEveID, perFatherID, recEventTypeRole::ROLE_Residence_Family );
            }
            if( perMotherID ) {
                AddPersonaToEventa( rEveID, perMotherID, recEventTypeRole::ROLE_Residence_Family );
            }
            if( per1stID ) {
                AddPersonaToEventa( rEveID, per1stID, recEventTypeRole::ROLE_Residence_Family );
            }
            AddPersonaToEventa( rEveID, perID, recEventTypeRole::ROLE_Residence_Family );
        }

//        if( !occupation.IsEmpty() && perFatherID ) {
//            int aseq = 0;
//            CreateOccupation( perFatherID, occupation, &aseq, ref.f_id, &seq );
//        }

        ref.f_title << recDate::GetStr( dateID ) << " Baptism of ";
        if( per1stID ) {
            ref.f_title << recPersona::GetNameStr( per1stID ) << " and ";
        }
        ref.f_title << recPersona::GetNameStr( perID );
        ref.Save();
        LinkOrCreateEventFromEventa( cEve1stID );
        LinkOrCreateEventFromEventa( cEveID );
    }
}

void ProcessCustomFile( wxFileName& fn )
{
    long ref;
    fn.GetName().Mid( 2 ).ToLong( &ref );
    switch( ref )
    {
    case 161:
        Process161File( fn );
        break;
    case 162:
        Process162File( fn );
        break;
    case 163:
        Process163File( fn );
        break;
    case 1051:
        Process1051File( fn );
        break;
    }
}

// End of nkRefDocuments.cpp file 

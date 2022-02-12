/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Name:        fiRefMarkup.cpp
 * Project:     tfp_fill: Private utility to create Matthews TFP database
 * Purpose:     fiRefMarkup Class implimentation.
 * Author:      Nick Matthews
 * Website:     http://thefamilypack.org
 * Created:     22nd February 2015
 * Copyright:   Copyright (c) 2015 ~ 2022, Nick Matthews.
 * Licence:     GNU GPLv3
 *
 *  tfp_fill is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  tfp_fill is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with tfp_fill.  If not, see <http://www.gnu.org/licenses/>.
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

#include "fiRefMarkup.h"

#include "nkMain.h"
#include "xml2.h"

#include <rec/recDb.h>

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

bool Date_IsOverlap( idt date1ID, idt date2ID )
{
    recDate d1( date1ID );
    recDate d2( date2ID );
    unsigned flags = d1.GetCompareFlags( d2 );
    return flags == recDate::CF_Overlap;
}

void Event_CreateRolesFromEventa( idt eID, idt eaID )
{
    recEventaPersonaVec epas = recEventa::GetEventaPersonas( eaID );
    for( size_t i = 0 ; i < epas.size() ; i++ ) {
        recIdVec indIDs = recPersona::GetIndividualIDs( epas[i].FGetPerID() );
        for( size_t j = 0 ; j < indIDs.size() ; j++ ) {
            recIndividualEvent ie;
            if( ie.Find( indIDs[j], eID, epas[i].FGetRoleID() ) ) {
                continue;
            }
            ie.FSetIndID( indIDs[j] );
            ie.FSetEventID( eID );
            ie.FSetRoleID( epas[i].FGetRoleID() );
            ie.FSetNote( epas[i].FGetNote() );
            ie.FSetIndSeq( recIndividual::GetMaxEventSeqNumber( indIDs[j] ) + 1 );
            ie.Save();
        }
    }
}

idt Event_CreateFromEventa( const recEventa& ea )
{
    idt eaID = ea.FGetID();
    idt typeID = ea.FGetTypeID();
    recEvent e(0);
    e.FSetTitle( ea.FGetTitle() );
    e.FSetTypeID( typeID );
    e.FSetDate1ID( ea.FGetDate1ID() );
    e.FSetDate2ID( ea.FGetDate2ID() );
    e.FSetPlaceID( ea.FGetPlaceID() );
    e.FSetNote( ea.FGetNote() );
    e.FSetDatePt( ea.FGetDatePt() );

    recEventTypeGrp grp = recEventType::GetGroup( typeID );
    idt eID;
    if( grp == recEventTypeGrp::personal ) {
        //idt existingEventID = recIndividual::GetPersonalEvent( ?? we dont have the individual!!!

        e.Save();
        eID = e.FGetID();
        Event_CreateRolesFromEventa( eID, eaID );

        e.FSetID( 0 );
        e.FSetHigherID( eID );
        e.Save();  // <<======<<<< If personal and existing, don't create another!
        eID = e.FGetID();
    } else {
        e.Save();
        eID = e.FGetID();
    }
    Event_CreateRolesFromEventa( eID, eaID );

    recEventEventa eea(0);
    eea.FSetEventID( eID );
    eea.FSetEventaID( eaID );
    eea.FSetConf( 0.999 );
    eea.Save();

    return eID;
}

// Compares Eventa and Event to determine if they refer
// to the same event.
bool Eventa_IsEventMatch( const recEventa& ea, idt eventID )
{
    recEvent eve(eventID);
    if( eve.FGetID() == 0 ) {
        return false;
    }
    recEventTypeGrp et = ea.GetTypeGroup();
    if( et != eve.GetTypeGroup() ) {
        return false;
    }
    switch( et )
    {
    case recEventTypeGrp::birth:
    case recEventTypeGrp::death:
        return true; // Only one Event per Individual for these.
    case recEventTypeGrp::fam_union:
    case recEventTypeGrp::fam_other:
        // TODO: Test for identical individuals.
        // Return false if not
        // Fall thru if they are.
    default:
        if( ! Date_IsOverlap( ea.FGetDate1ID(), eve.FGetDate1ID() ) ) {
            return false;
        }
        // TODO: Test for equivalient place
        // Assuming ok for now.
        break;
    }
    return true;
}


// We may move this to the recEventa class.
// If the persona linked to the eventa have Individual links
// then check to see if a corresponding Event exists,
// if not, create one. Then create the EventEventa link.
idt Eventa_CreateEventLink( const recEventa& ea )
{
    recCheckIdVec ces = ea.FindCheckedLinkedEvents();
    if( ces.size() ) {
        for( size_t i = 0 ; i < ces.size() ; i++ ) {
            if( ces[i].GetFirstID() != 0 ) {
                continue;
            }
            idt eID = ces[i].GetSecondID();
            if( Eventa_IsEventMatch( ea, eID ) ) {
                recEventEventa::Create( eID, ea.FGetID() );
                Event_CreateRolesFromEventa( eID, ea.FGetID() );
            } else {
                Event_CreateFromEventa( ea );
            }
        }
    } else {
        return Event_CreateFromEventa( ea );
    }
    return 0;
}

idt Name_CreateDuplicateName( idt nameID, idt indID, idt perID ) 
{
    recName name( nameID );
    name.FSetID( 0 );
    name.FSetIndID( indID );
    name.FSetPerID( perID );
    name.Save();

    recNamePartVec parts = recName::GetParts( nameID );
    for( size_t i = 0 ; i < parts.size() ; i++ ) {
        recNamePart part( parts[i].FGetID() );
        part.FSetID( 0 );
        part.FSetNameID( name.FGetID() );
        part.Save();
    }
    return name.FGetID();
}

idt Individual_CreateFromPersona( idt perID, idt indID )
{
    recPersona per(perID);
    if( per.FGetID() == 0 ) {
        return 0;
    }
    idt nameID = per.GetNameID( perID );
    nameID = Name_CreateDuplicateName( nameID, indID, 0 );
    if( nameID == 0 ) {
        return 0;
    }

    recIndividual ind(0);
    ind.FSetID( indID );
    ind.FSetSex( per.FGetSex() );
    // We don't copy the note field since this would
    // normally refer to the Reference Document.
    ind.FSetName( recName::GetNameStr( nameID ) );
    ind.FSetSurname( recName::GetSurname( nameID ) );
    ind.Save();
    return ind.FGetID();
}

bool fiRefMarkup::create_records()
{
    wxXmlNode* body = NULL;
    wxString data;
    wxXmlNode* child = m_root->GetChildren();
    while( child ) {
        if( child->GetType() == wxXML_ELEMENT_NODE &&
            child->GetName() == "body" )
        {
            body = child;
        }
        if( child->GetType() == wxXML_COMMENT_NODE &&
            child->GetContent().substr( 0, 7 ) == "[-tfp-]" )
        {
            data = child->GetContent().substr( 7 );
        }
        child = child->GetNext();
    }
    if( body == NULL ) {
        return false;
    }

    StringVec statements = parse_statements( data );
    for( size_t i = 0 ; i < statements.size() ; i++ ) {
        if( statements[i].compare( 0, 2, "L-" ) != 0 ) {
            // Only interested in parsing local records (for now).
            continue;
        }
        wxString localStr, recordStr;
        idt localID = 0, recordID = 0;
        size_t pos = statements[i].find( ':' );
        if( pos == wxString::npos || pos < 3 ) {
            continue;
        }
        recordStr =  statements[i].substr( pos + 1 );
        localStr = statements[i].substr( 2, pos - 2 );
        if( localStr.compare( 0, 2, "Pa" ) == 0 ) {
            recordID = create_persona_rec( recordStr );
            if( recordID ) {
                m_cur_persona = recordID;
            }
        } else if( localStr.compare( 0, 2, "IP" ) == 0 ) {
            idt indID = find_id( recordStr );
            create_ind_per_rec( indID, m_cur_persona );
        } else if( localStr.compare( 0, 1, "N" ) == 0 ) {
            recordID = create_name_rec( m_cur_persona, recordStr );
        } else if( localStr.compare( 0, 1, "D" ) == 0 ) {
            recordID = create_date_rec( recordStr );
            if( recordID ) {
                m_cur_date = recordID;
            }
        } else if( localStr.compare( 0, 1, "P" ) == 0 ) {
            recordID = create_place_rec( recordStr );
            if( recordID ) {
                m_cur_place = recordID;
            }
        } else if( localStr.compare( 0, 2, "Ea" ) == 0 ) {
            recordID = create_eventa_rec( recordStr );
        } else if( localStr.compare( 0, 2, "Ro" ) == 0 ) {
            recordID = create_role_rec( recordStr );
        } else if( localStr.compare( 0, 2, "EP" ) == 0 ) {
            recordID = create_eventa_per_rec( recordStr );
        } else if( localStr.compare( 0, 2, "EE" ) == 0 ) {
            recordID = create_event_eventa_rec( recordStr );
        }

        if( recordID != 0 ) {
            m_localIDs[localStr] = recordID;
        }
    }
    recIndividual::CreateMissingFamilies();
    create_reference_rec();

    return true;
}

// Splits into statements that end with the ';' character.
// Removes line comments that start with "//". Leaves the new line.
// Passes over " quoted sections.
StringVec fiRefMarkup::parse_statements( const wxString& str ) const
{
    StringVec result;
    wxString statement;
    bool inquote = false;
    bool comment = false;
    for( wxString::const_iterator it = str.begin() ; it != str.end() ; it++ ) {
        if( comment ) {
            if( *it == '\n' ) {
                comment = false;
                statement += ' ';
            }
            continue;
        }
        if( *it == '/' ) {
            if( it+1 != str.end() && *(it+1) == '/' ) {
                comment = true;
                continue;
            }
        } else if( *it == '"' ) {
            inquote = !inquote;
        }
        if( !inquote && *it == ';' ) {
            result.push_back( statement.Trim( false ) );
            statement.clear();
        } else {
            if( *it == '\n' || *it == '\t' ) {
                statement += ' ';
            } else {
                statement += *it;
            }
        }
    }
    return result;
}

wxString fiRefMarkup::read_text( const wxString& input, wxString* tail ) const
{
    wxString str = input; // Work on copy because input may equal tail.
    wxString text;
    wxString::const_iterator it = str.begin();
    if( str.compare( 0, 1, "\"" ) == 0 ) {
        it++;
        while( it != str.end() ) {
            if( *it == '"' ) {
                it++;
                if( it != str.end() && *it == ',' ) {
                    it++;
                }
                break;
            }
            text += *it++;
        }
    } else {
        while( it != str.end() ) {
            if( *it == ',' ) {
                it++;
                break;
            }
            text += *it++;
        }
    }
    if( tail ) {
        tail->clear();
        while( it != str.end() ) {
            *tail += *it++;
        }
    }
    return text;
}

Sex fiRefMarkup::read_sex( const wxString& input, wxString* tail ) const
{
    wxString str = input;
    Sex sex = Sex::unstated;
    wxString::const_iterator it = str.begin();
    if( it != str.end() ) {
        if( *it == 'M' ) {
            sex = Sex::male;
        } else if( *it == 'F' ) {
            sex = Sex::female;
        } else if( *it == 'U' ) {
            sex = Sex::unknown;
        }
        if( sex != Sex::unstated ) {
            it++;
        }
        if( it != str.end() ) {
            if( *it == ',' ) {
                it++;
            }
        }
    }
    if( tail ) {
        tail->clear();
        while( it != str.end() ) {
            *tail += *it++;
        }
    }
    return sex;
}

idt fiRefMarkup::read_int( const wxString& input, wxString* tail ) const
{
    wxString str = input; // Work on copy because input may equal tail.
    wxString num;
    wxString::const_iterator it = str.begin();
    while( it != str.end() ) {
        if( *it == ',' ) {
            it++;
            break;
        }
        num += *it++;
    }
    if( tail ) {
        tail->clear();
        while( it != str.end() ) {
            *tail += *it++;
        }
    }
    return recGetID( num );
}

bool fiRefMarkup::read_bool( const wxString& input, wxString* tail ) const
{
    wxString str = input; // Work on copy because input may equal tail.
    wxString b;
    wxString::const_iterator it = str.begin();
    while( it != str.end() ) {
        if( *it == ',' ) {
            it++;
            break;
        }
        b += *it++;
    }
    if( tail ) {
        tail->clear();
        while( it != str.end() ) {
            *tail += *it++;
        }
    }
    if( b.size() && b != "0" && b.Lower().compare( "true" ) != 0 ) {
        return true;
    }
    return false;
}

idt fiRefMarkup::find_id( const wxString& recStr ) const
{
    if( recStr.size() <= 3 ) {
        return 0;
    }
    wxString::const_iterator it = recStr.begin();
    if( *it == 'L' ) {
        StringIdMap::const_iterator lit;
        lit = m_localIDs.find( recStr.substr( 2 ) );
        if( lit == m_localIDs.end() ) {
            return 0;
        }
        return lit->second;
    }
    if( *it != 'D' ) {
        return 0;
    }
    bool in_num = false;
    wxString numStr;
    it += 3;
    while( it != recStr.end() ) {
        if( in_num ) {
            if( !isdigit( *it ) ) {
                break;
            }
            numStr += *it;
        } else if( isdigit( *it ) || *it == '-' ) {
            in_num = true;
            numStr = *it;
        }
        it++;
    }
    return recGetID( numStr );
}

void fiRefMarkup::markup_node( wxXmlNode* refNode )
{
    wxXmlNode* child = refNode->GetChildren();
    while( child ) {
        if( child->GetType() == wxXML_ELEMENT_NODE ) {
            wxString hrefStr;
            if( child->GetName() == "a" || child->GetName() == "span" ) {
                wxString rIdStr = child->GetAttribute( "id" );
                if( rIdStr.size() ) {
                    hrefStr = convert_local_id( rIdStr );
                    if( hrefStr.size() ) {
                        child->SetName( "a" );
                        // Remove existing attributes.
                        wxXmlAttribute *a, *a2;
                        for( a = child->GetAttributes() ; a ; a = a2 ) {
                            a2 = a->GetNext();
                            delete a;
                        }
                        wxXmlAttribute* a_href = new wxXmlAttribute( "href", hrefStr );
                        child->SetAttributes( a_href );
                    }
                }
            }
            markup_node( child );
        }
        child = child->GetNext();
    }
}

wxString fiRefMarkup::convert_local_id( const wxString& localStr ) const
{
    idt hrefID = 0;
    wxString prefix;
    if( localStr.size() > 2 ) {
        wxString s = localStr.substr( 2 );
        StringIdMap::const_iterator it = m_localIDs.find( s );
        if( it != m_localIDs.end() ) {
            hrefID = it->second;
            wxString::const_iterator it = s.begin();
            for( ; it != s.end() && !wxIsdigit( *it ) ; it++ ) {
                prefix += *it;
            }
        }
    }
    if( prefix.size() == 0 ) {
        return "";
    }
    wxString url;
    if( prefix == "Ea" ) {
        url = "tfp:";
    } else {
        url = "tfpi:";
    }
    return url + prefix + recGetStr( hrefID );
}

idt fiRefMarkup::create_persona_rec( const wxString& str )
{
    wxString tail;
    Sex sex = read_sex( str, &tail );
    wxString note;
    if( tail.size() && tail.compare( 0, 1, "\"" ) == 0 ) {
        note = read_text( tail, NULL );
    }

    recPersona per(0);
    per.FSetRefID( m_referenceID );
    per.FSetSex( sex );
    per.FSetNote( note );
    per.Save();

    return per.FGetID();
}

idt fiRefMarkup::create_name_rec( idt perID, const wxString& str )
{
    if( str.compare( 0, 1, "\"" ) != 0 ) {
        return 0;
    }
    wxString tail;
    wxString name_str = read_text( str, &tail );

    idt styleID = recGetID( tail );
    // Alternative to using style id number
    if( tail == "Married" ) {
        styleID = (idt) recNameStyle::NS_Married;
    } else if( tail == "Birth" ) {
        styleID = (idt) recNameStyle::NS_Birth;
    } else if( tail == "Alias" ) {
        styleID = (idt) recNameStyle::NS_Alias;
    }

    recName name(0);
    name.FSetPerID( perID );
    name.FSetTypeID( styleID );
    name.SetNextSequence();
    name.Save();
    name.AddNameParts( name_str );

    recReferenceEntity::Create( m_referenceID, recReferenceEntity::TYPE_Name, name.FGetID() );

    return name.FGetID();
}

idt fiRefMarkup::create_ind_per_rec( idt indID, idt perID )
{
    if( indID <= 0 ) {
        return 0;
    }
    if( !recIndividual::Exists( indID ) ) {
        // Create Individual from from Persona
        indID = Individual_CreateFromPersona( perID, indID );
    }
    recIndividualPersona ip(0);
    ip.FSetIndID( indID );
    ip.FSetPerID( perID );
    ip.FSetConf( 0.99 );
    ip.Save();
    return ip.FGetID();
}

idt fiRefMarkup::create_date_rec( const wxString& str )
{
    wxString dateStr = read_text( str, NULL );

    idt dateID = recDate::Create( dateStr );
    recReferenceEntity::Create( m_referenceID, recReferenceEntity::TYPE_Date, dateID );
    return dateID;
}

idt fiRefMarkup::create_place_rec( const wxString& str )
{
    wxString tail;
    wxString placeStr = read_text( str, NULL );

    idt placeID = recPlace::Create( placeStr );
    recReferenceEntity::Create( m_referenceID, recReferenceEntity::TYPE_Place, placeID );
    return placeID;
}

idt fiRefMarkup::create_eventa_rec( const wxString& str )
{
    wxString tail;
    wxString head = read_text( str, &tail );

    wxString etypeStr, perName;
    idt etypeID = 0, perID = 0;
    if( head.compare( 0, 4, "D-ET" ) == 0 ) {
        etypeStr = head.substr( 4 );
        etypeID = recGetID( etypeStr );
    }
    wxString perStr = read_text( tail, NULL );
    if( perStr.compare( 0, 4, "L-Pa" ) == 0 ) {
        perID = find_id( perStr );
        perName = recName::GetDefaultNameStr( 0, perID );
    }

    recEventa ea(0);
    ea.FSetRefID( m_referenceID );
    ea.FSetTypeID( etypeID );
    ea.FSetDate1ID( m_cur_date );
    ea.FSetPlaceID( m_cur_place );
    ea.SetAutoTitle( perName );
    ea.UpdateDatePoint();
    ea.Save();
    m_cur_eventa = ea.FGetID();

    create_eventa_per_rec( tail );

    return m_cur_eventa;
}

idt fiRefMarkup::create_eventa_per_rec( const wxString& str )
{
    wxString roleStr;
    wxString perStr = read_text( str, &roleStr );
    idt perID = 0, roleID = 0;
    if( perStr.compare( 0, 4, "L-Pa" ) == 0 ) {
        perID = find_id( perStr );
    }
    if( roleStr.compare( 0, 4, "D-Ro" ) == 0 ) {
        roleID = recGetID( roleStr.substr( 4 ) );
    } else {
        roleID = m_cur_role;
    }

    recEventaPersona ep(0);
    ep.FSetEventaID( m_cur_eventa );
    ep.FSetPerID( perID );
    ep.FSetRoleID( roleID );
    ep.FSetPerSeq( recEventa::GetLastPerSeqNumber( m_cur_eventa ) + 1 );
    ep.Save();

    return ep.FGetID();
}

idt fiRefMarkup::create_event_eventa_rec( const wxString& str )
{
    // Ignore str for now.
    recEventa ea( m_cur_eventa );
    return Eventa_CreateEventLink( ea );
}

idt fiRefMarkup::create_role_rec( const wxString& str )
{
    wxString tail;
    wxString head = read_text( str, &tail );

    wxString etypeStr;
    idt etypeID = 0;
    if( head.compare( 0, 4, "D-ET" ) == 0 ) {
        etypeStr = head.substr( 4 );
        etypeID = recGetID( etypeStr );
        if( etypeID == 0 ) {
            return 0;
        }
    }
    recEventTypeRole::Prime prime = recEventTypeRole::Prime( read_int( tail, &tail ) );
    bool official = read_bool( tail, &tail );
    wxString value = read_text( tail, NULL );

    m_cur_role = recEventTypeRole::FindOrCreate( value, etypeID, prime, official );
    return m_cur_role;
}

void fiRefMarkup::create_reference_rec()
{
    wxXmlNode* child = m_root->GetChildren();
    wxXmlNode* refNode = NULL;
    wxString title;

    while( child ) {
        if( child->GetName() == "body" ) {
            child = child->GetChildren();
            continue;
        } else if( child->GetName() == "h1" ) {
            title = xmlGetAllContent( child );
        } else if( child->GetName() == "div" ) {
            wxString idAttr = child->GetAttribute( "id" );
            if( idAttr != "topmenu" && refNode == NULL ) {
                // We should be looking at reference text
                refNode = child;
                break;
            }
        }
        child = child->GetNext();
    }
    if( refNode ) {
        markup_node( refNode );
        wxString refStr = "<!-- HTML -->\n" + xmlGetSource( refNode );
        recReference ref(m_referenceID);
        ref.f_id = m_referenceID;
        ref.f_title = title;
        ref.f_statement = refStr;
        ref.Save();
    }
}

// End of fiRefMarkup.cpp file

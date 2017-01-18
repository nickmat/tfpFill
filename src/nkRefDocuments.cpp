/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Name:        nkRefDocuments.cpp
 * Project:     tfpnick: Private utility to create Matthews TFP database
 * Purpose:     Read and process Reference Document files
 * Author:      Nick Matthews
 * Website:     http://thefamilypack.org
 * Created:     23rd September 2011
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

#include <wx/dir.h>
#include <wx/textfile.h>

#include <rec/recDb.h>

#include "nkMain.h"
#include "xml2.h"


idt g_1841CensusDateID;
idt g_1851CensusDateID;
idt g_1861CensusDateID;
idt g_1871CensusDateID;
idt g_1881CensusDateID;
idt g_1891CensusDateID;
idt g_1901CensusDateID;
idt g_1911CensusDateID;

void CreateSourceGlobals()
{
    g_1841CensusDateID = recDate::Create( "6 Jun 1841" );
    g_1851CensusDateID = recDate::Create( "31 Mar 1851" );
    g_1861CensusDateID = recDate::Create( "8 Apr 1861" );
    g_1871CensusDateID = recDate::Create( "3 Apr 1871" );
    g_1881CensusDateID = recDate::Create( "4 Apr 1881" );
    g_1891CensusDateID = recDate::Create( "6 Apr 1891" );
    g_1901CensusDateID = recDate::Create( "1 Apr 1901" );
    g_1911CensusDateID = recDate::Create( "3 Apr 1911" );
}

void ListIndividuals( wxXmlNode* node, recIdVec& list, wxArrayString& names )
{
    while( node ) {
        if( node->GetType() == wxXML_ELEMENT_NODE ) {
            if( node->GetName() == "a" ) {
                wxXmlAttribute* attr = node->GetAttributes();
                while( attr ) {
                    if( attr->GetName() == "href" ) {
                        wxString newHref;
                        idt id;
                        if( DecodeHref( attr->GetValue(), &id, &newHref ) ) {
                            attr->SetValue( newHref );
                            list.push_back( id );
                            names.push_back( xmlGetAllContent( node ) );
                        }
                        attr = NULL;
                    } else {
                        attr = attr->GetNext();
                    }
                }
            } else {
                ListIndividuals( node->GetChildren(), list, names );
            }
        }
       node = node->GetNext();
    }
}

// Create date from a node holding an age value and
// place a anchor link around it pointing to the new date record.
idt CreateDateFromAge( wxXmlNode* node, idt baseID, idt refID, int* pseq )
{
    long age;
    wxString ageStr = xmlGetAllContent( node );
    if( !ageStr.ToLong( &age ) ) {
        return 0;
    }
    idt dateID = CreateDateFromAge( age, baseID, refID, pseq );

    xmlCreateLink( node, "tfpi:"+recDate::GetIdStr( dateID ) );

    return dateID;
}

// Create place record from a node holding an address value and
// place a anchor link around it pointing to the new place record.
idt CreatePlace( wxXmlNode* node, idt refID, int* pseq )
{
    wxString address = xmlGetAllContent( node );
    if( address.IsEmpty() ) {
        return 0;
    }
    idt placeID = CreatePlace( address, refID, pseq );

    xmlCreateLink( node, "tfpi:"+recPlace::GetIdStr( placeID ) );

    return placeID;
}

wxString GetCensusAddress( wxXmlNode* table )
{
    wxString address;
    wxString part;
    wxXmlNode* row;
    wxXmlNode* data;
    // Get address, 1901 files have been structured differently from others
    // census. We will make the address by combining the "Address", "Civil
    // Parish" and "Administrative County" fields.
    // TODO: Institutions (ie R559) have different layout for address
    row = xmlGetFirstChild( table, "tr" );
    data = xmlGetFirstChild( row, "td" );
    data = xmlGetNext( data, "td" );
    if( xmlGetAllContent( data ) == "Signature:" ) {
        row = xmlGetNext( row, "tr" );        // 1911 format has extra row
        data = xmlGetFirstChild( row, "td" );
        data = xmlGetNext( data, "td" );
    }
    data = xmlGetNext( data, "td" );
    address = xmlGetAllContent( data ); // "Address"
    row = xmlGetNext( row, "tr" );
    data = xmlGetFirstChild( row, "td" );
    data = xmlGetNext( data, "td" );
    data = xmlGetNext( data, "td" );
    part = xmlGetAllContent( data );   // "Civil Parish"
    address = CreateCommaList( address, part );
    row = xmlGetNext( row, "tr" );
    row = xmlGetNext( row, "tr" );
    data = xmlGetFirstChild( row, "td" );
    data = xmlGetNext( data, "td" );
    data = xmlGetNext( data, "td" );
    data = xmlGetNext( data, "td" );
    data = xmlGetNext( data, "td" );
    data = xmlGetNext( data, "td" );
    part = xmlGetAllContent( data );  // "Administrative County"
    return CreateCommaList( address, part );
}

void Process1841CensusIndividuals(
    wxXmlNode* row, idt refID, idt eventID, idt dateID, const wxString& address, int* pseq )
{
    wxXmlNode* data;
    idt indID, ageID, bplaceID = 0, occID;
    bool samecounty;
    wxString samecountyStr;
    wxString name;
    wxString county = address.AfterLast( ' ' );
    Sex sex;
    wxString occ;
    recEventaPersona ep(0);
    ep.f_eventa_id = eventID;
    ep.f_role_id = recEventTypeRole::ROLE_Census_Listed;
    int personaSeq = 0;
    while( row ) {
        data = xmlGetFirstChild( row, "td" );  // In name column.
        indID = GetIndividualAnchor( data, &name );
        if( indID && !name.IsEmpty() ) {
            data = xmlGetNext( data, "td" );  // Age column.
            ageID = CreateDateFromAge( data, dateID, refID, pseq );
            data = xmlGetNext( data, "td" );  // In Sex column.
            sex = GetSexFromStr( xmlGetAllContent( data ) );

            idt perID = CreatePersona( refID, indID, name, sex, pseq );
            // Add persona to Census Event
            ep.f_id = 0;
            ep.f_per_id = perID;
            ep.f_per_seq = ++personaSeq;
            ep.Save();
            data = xmlGetNext( data, "td" );  // Occupation column.
            occ = xmlGetAllContent( data );
            occID = CreateOccupation( occ, refID, perID, dateID, pseq );
            if( occID ) {
                recEventa::CreatePersonalEvent( occID );
                xmlCreateLink( data, "tfp:"+recEventa::GetIdStr( occID ) );
            }
            data = xmlGetNext( data, "td" );  // Same county column.
            samecountyStr = xmlGetAllContent( data );
            samecountyStr.LowerCase();
            if( samecountyStr.Mid( 0, 1 ) == "y" ) {
                if( bplaceID == 0 ) {
                    bplaceID = CreatePlace( county, refID, pseq );
                }
                samecounty = true;
                xmlCreateLink( data, "tfpi:"+recPlace::GetIdStr( bplaceID ) );
            } else {
                samecounty = false;
            }
            if( ageID || samecounty ) {
                CreateBirthEvent( refID, perID, ageID, bplaceID, pseq );
            }
        }
        row = xmlGetNext( row, "tr" );
    }
}

void ProcessCensusIndividuals(
    wxXmlNode* row, idt refID, idt cen_eaID, idt dateID, idt placeID, int* pseq )
{
    wxXmlNode* data;
    wxXmlNode* condNode;
    wxXmlNode* span;
    idt indID, bplaceID, ageID, attID;
    int attSeq = 0;
    wxString name;
    idt headPerID = 0, spousePerID = 0;
    recIdVec childIDs;
    wxString relStr;
    wxString condStr;
    Sex sex;
    wxString occ;
    recEventaPersona cen_ep(0);
    cen_ep.f_eventa_id = cen_eaID;
    cen_ep.f_role_id = recEventTypeRole::ROLE_Census_Listed;
    int personaSeq = 0;

    idt res_eaID = CreateResidenceEventa( dateID, placeID, refID, pseq );

    while( row ) {
        data = xmlGetFirstChild( row, "td" );  // In name column.
        indID = GetIndividualAnchor( data, &name );
        if( indID && !name.IsEmpty() ) {
            data = xmlGetNext( data, "td" );  // In Relation column.
            relStr = xmlGetAllContent( data );
            if( dateID == g_1911CensusDateID ) {  // 1911 has different order
                data = xmlGetNext( data, "td" );  // In Age column.
                ageID = CreateDateFromAge( data, dateID, refID, pseq );
                data = xmlGetNext( data, "td" );  // In Sex column.
                sex = GetSexFromStr( xmlGetAllContent( data ) );
                data = xmlGetNext( data, "td" );  // In Marriage column.
                condStr = xmlGetAllContent( data );
                condNode = data;
            } else {
                data = xmlGetNext( data, "td" );  // In Marriage column.
                condStr = xmlGetAllContent( data );
                condNode = data;
                data = xmlGetNext( data, "td" );  // In Age column.
                ageID = CreateDateFromAge( data, dateID, refID, pseq );
                data = xmlGetNext( data, "td" );  // In Sex column.
                sex = GetSexFromStr( xmlGetAllContent( data ) );
            }

            idt perID = CreatePersona( refID, indID, name, sex, pseq );
            // Add persona to Census Event
            cen_ep.FSetID( 0 );
            cen_ep.FSetPerID( perID );
            cen_ep.FSetPerSeq( ++personaSeq );
            cen_ep.FSetNote( relStr );
            cen_ep.Save();
            attSeq = 0;

            if( headPerID == 0 || relStr == "Head" ) {
                headPerID = perID;
            } else if( spousePerID == 0 && headPerID != 0 && relStr == "Wife" ) {
                spousePerID = perID;
            } else if( headPerID != 0 && ( relStr == "Daur" || relStr == "Son" ) ) {
                childIDs.push_back( perID );
            }

            recEventTypeRole::Role resRoleID;
            if( relStr == "Visitor" ) {
                resRoleID = recEventTypeRole::ROLE_Residence_Visitor;
            } else if( relStr == "Boarder" ) {
                resRoleID = recEventTypeRole::ROLE_Residence_Boarder;
            } else if( relStr == "Lodger" ) {
                resRoleID = recEventTypeRole::ROLE_Residence_Lodger;
            } else {
                resRoleID = recEventTypeRole::ROLE_Residence_Family;
            }
            AddPersonaToEventa( res_eaID, perID, resRoleID );

            data = xmlGetNext( data, "td" );  // In Birthplace column.
            bplaceID = CreatePlace( data, refID, pseq );

            attID = CreateCondition( GetConditionStr( sex, condStr ), refID, perID, dateID, pseq );
            if( attID ) {
                recEventa::CreatePersonalEvent( attID );
                xmlCreateLink( condNode, "tfp:"+recEventa::GetIdStr( attID ) );
            }

            bplaceID = 0;
            if( data ) {
                bplaceID = CreatePlace( data, refID, pseq );
            }
            if( ageID || bplaceID ) {
                CreateBirthEvent( refID, perID, ageID, bplaceID, pseq );
            }
            // See if there is an Occupation.
            row = xmlGetNext( row, "tr" );
            data = xmlGetFirstChild( row, "td" );
            data = xmlGetNext( data, "td" );
            if( data ) {
                span = xmlGetFirstChild( data, "span" );
                span = xmlGetNext( span, "span" );
                occ = xmlGetAllContent( span );
                attID = CreateOccupation( occ, refID, perID, dateID, pseq );
                if( attID ) {
                    recEventa::CreatePersonalEvent( attID );
                    xmlCreateLink( span, "tfp:"+recEventa::GetIdStr( attID ) );
                }
            }
        }
        row = xmlGetNext( row, "tr" );
    }
    if( headPerID != 0 && ( spousePerID != 0 || !childIDs.empty() ) ) {
        // We have a family.
        idt eaID = CreateFamilyRelEventa( refID, headPerID, dateID, placeID, pseq );
        if( spousePerID ) {
            AddPersonaToEventa( eaID, spousePerID, recEventTypeRole::ROLE_Family_Wife );
        }
        for( size_t i = 0 ; i < childIDs.size() ; i++ ) {
            AddPersonaToEventa( eaID, childIDs[i], recEventTypeRole::ROLE_Family_Child );
        }
        recEventa::CreateFamilyLink( eaID );
    }
    recEventa::CreatePersonalEvent( res_eaID );
//    LinkOrCreateEventFromEventa( cen_eaID );
    CreateEventFromEventa( cen_eaID );
}


void Create1901UkCensus( idt refID, wxXmlNode* refNode, const wxString& title )
{
    int refSeq = 0;
    wxString address;
    wxString part;
    wxXmlNode* row;
    wxXmlNode* data;
    wxXmlNode* addrNode;
    wxXmlNode* table = xmlGetFirstChild( refNode, "table" );
    // Get address, 1901 files have been structured differently from others
    // census. We will make the address by combining the "Address", "Civil
    // Parish" and "Administrative County" fields.
    // TODO: Institutions (ie R559) have different layout for address
    row = xmlGetFirstChild( table, "tr" );
    data = xmlGetFirstChild( row, "td" );
    data = xmlGetNext( data, "td" );
    data = xmlGetNext( data, "td" );
    address = xmlGetAllContent( data ); // "Address"
    addrNode = data;
    wxString addrStr = address;
    row = xmlGetNext( row, "tr" );
    data = xmlGetFirstChild( row, "td" );
    data = xmlGetNext( data, "td" );
    data = xmlGetNext( data, "td" );
    part = xmlGetAllContent( data );   // "Civil Parish"
    address = CreateCommaList( address, part );
    row = xmlGetNext( row, "tr" );
    row = xmlGetNext( row, "tr" );
    data = xmlGetFirstChild( row, "td" );
    data = xmlGetNext( data, "td" );
    data = xmlGetNext( data, "td" );
    data = xmlGetNext( data, "td" );
    data = xmlGetNext( data, "td" );
    data = xmlGetNext( data, "td" );
    part = xmlGetAllContent( data );  // "Administrative County"
    address = CreateCommaList( address, part );
    // Carry on.
    table = xmlGetNext( table, "table" );
    table = xmlGetNext( table, "table" );
    row = xmlGetFirstChild( table, "tr" );
    row = xmlGetNext( row, "tr" );
    if( !row ) return;

    // We are now comitted to creating the records
    idt placeID = CreatePlace( address, refID, &refSeq );
    if( addrStr.size() ) {
        xmlCreateLink( addrNode, "tfpi:P"+recGetStr( placeID ) );
    }
    idt eventID = CreateCensusEvent( title, g_1901CensusDateID, placeID, refID, &refSeq );
    ProcessCensusIndividuals( row, refID, eventID, g_1901CensusDateID, placeID, &refSeq );
}

void CreateUkCensus( idt refID, idt dateID, wxXmlNode* refNode, const wxString& title )
{
    int refSeq = 0;
    wxXmlNode* row;
    wxXmlNode* table = xmlGetFirstChild( refNode, "table" );
    wxString address = GetCensusAddress( table );
    // Get to first person
    table = xmlGetNext( table, "table" );
    row = xmlGetFirstChild( table, "tr" );
    row = xmlGetNext( row, "tr" );
    if( !row ) return;
    // We are now comitted to creating the records
    idt placeID = CreatePlace( address, refID, &refSeq );
    idt eventID = CreateCensusEvent( title, dateID, placeID, refID, &refSeq );
    if( dateID == g_1841CensusDateID ) {
        Process1841CensusIndividuals( row, refID, eventID, dateID, address, &refSeq );
    } else {
        ProcessCensusIndividuals( row, refID, eventID, dateID, placeID, &refSeq );
    }
}

void CreateIgiBaptism( idt refID, wxXmlNode* refNode )
{
    int refSeq = 0;
    wxXmlNode* table;
    wxXmlNode* row;
    wxXmlNode* cell;
    wxXmlNode* table2;
    wxXmlNode* row2;
    wxXmlNode* cell2;

    // Read in all data
    table = xmlGetFirstChild( refNode, "center" );
    table = xmlGetFirstChild( table, "table" );
    row = xmlGetFirstChild( table, "tr" );

    row = xmlGetNext( row, "tr" );
    cell = xmlGetFirstChild( row, "td" );
    wxString name;
    idt indID = GetIndividualAnchor( cell, &name );

    row = xmlGetNext( row, "tr" );
    cell = xmlGetFirstChild( row, "td" );
    cell = xmlGetNext( cell, "td" );
    Sex sex = GetSexFromStr( xmlGetAllContent( cell ) );
    idt perID = CreatePersona( refID, indID, name, sex, &refSeq );

    row = xmlGetNext( row, "tr" );
    row = xmlGetNext( row, "tr" );
    row = xmlGetNext( row, "tr" );
    row = xmlGetNext( row, "tr" );
    cell = xmlGetFirstChild( row, "td" );
    cell = xmlGetNext( cell, "td" );
    table2 = xmlGetFirstChild( cell, "table" );
    row2 = xmlGetFirstChild( table2, "tr" );
    cell2 = xmlGetFirstChild( row2, "td" );
    wxXmlNode* birthEventCell = cell2;
    cell2 = xmlGetNext( cell2, "td" );
    wxString birthStr = xmlGetAllContent( cell2 );
    wxXmlNode* birthDateCell = cell2;

    row2 = xmlGetNext( row2, "tr" );
    cell2 = xmlGetFirstChild( row2, "td" );
    wxXmlNode* chrisEventCell = cell2;
    cell2 = xmlGetNext( cell2, "td" );
    wxString chrisStr = xmlGetAllContent( cell2 );
    wxXmlNode* chrisCell = cell2;

    row2 = xmlGetNext( row2, "tr" );
    cell2 = xmlGetFirstChild( row2, "td" );
    wxXmlNode* deathEventCell = cell2;
    cell2 = xmlGetNext( cell2, "td" );
    wxString deathStr = xmlGetAllContent( cell2 );
    wxXmlNode* deathCell = cell2;

    row2 = xmlGetNext( row2, "tr" );
    cell2 = xmlGetFirstChild( row2, "td" );
    wxXmlNode* burialEventCell = cell2;
    cell2 = xmlGetNext( cell2, "td" );
    wxString burialStr = xmlGetAllContent( cell2 );
    wxXmlNode* burialCell = cell2;

    row = xmlGetNext( row, "tr" );
    row = xmlGetNext( row, "tr" );
    row = xmlGetNext( row, "tr" );
    cell = xmlGetFirstChild( row, "td" );
    cell = xmlGetNext( cell, "td" );
    table2 = xmlGetFirstChild( cell, "table" );
    row2 = xmlGetFirstChild( table2, "tr" );
    cell2 = xmlGetFirstChild( row2, "td" );
    cell2 = xmlGetNext( cell2, "td" );
    idt fatherIndID = GetIndividualAnchor( cell2, &name );
    idt fatherPerID = 0;
    if( !name.IsEmpty() ) {
        fatherPerID = CreatePersona( refID, fatherIndID, name, SEX_Male, &refSeq );
    }

    row2 = xmlGetNext( row2, "tr" );
    cell2 = xmlGetFirstChild( row2, "td" );
    cell2 = xmlGetNext( cell2, "td" );
    idt motherIndID = GetIndividualAnchor( cell2, &name );
    idt motherPerID = 0;
    if( !name.IsEmpty() ) {
//        idt nameID =  recPersona::GetNameID( fatherPerID );
//        nameID = GetWifeName( name, nameID, refID, &refSeq );
//        motherPerID = CreatePersona( refID, motherIndID, nameID, SEX_Female );
        // Assume name is just given name so add '?' for birth surname
        motherPerID = CreatePersona( refID, motherIndID, name+" ?", SEX_Female, &refSeq );
    }

    idt dateID;
    idt placeID;
    idt eaID;
    idt refDateID = 0;
    if( !birthStr.IsEmpty() ) {
        dateID = CreateDate( birthStr, refID, &refSeq );
        placeID = 0;
        eaID = CreateBirthEvent( refID, perID, dateID, placeID, &refSeq );
        AddPersonaToEventa( eaID, motherPerID, recEventTypeRole::ROLE_Birth_Mother );
        LinkOrCreateEventFromEventa( eaID );
        xmlCreateLink( birthDateCell, "tfpi:"+recDate::GetIdStr( dateID ) );
        xmlCreateLink( birthEventCell, 0, 5, "tfp:"+recEventa::GetIdStr( eaID ) );
    }
    if( !chrisStr.IsEmpty() ) {
        wxString chrisDateStr, chrisPlaceStr;
        int pos = chrisStr.Find( "  " );
        if( pos != wxNOT_FOUND ) {
            chrisDateStr = chrisStr.Mid( 0, pos );
            chrisPlaceStr = chrisStr.Mid( pos+2 );
        } else {
            chrisDateStr = chrisStr;
        }
        refDateID = dateID = CreateDate( chrisDateStr, refID, &refSeq );
        placeID = CreatePlace( chrisPlaceStr, refID, &refSeq );
        eaID = CreateChrisEventa( refID, perID, dateID, placeID, &refSeq );
        AddPersonaToEventa( eaID, fatherPerID, recEventTypeRole::ROLE_Baptism_Parent );
        AddPersonaToEventa( eaID, motherPerID, recEventTypeRole::ROLE_Baptism_Parent );
        LinkOrCreateEventFromEventa( eaID );
        wxXmlNode* link = xmlCreateLink( chrisCell, 0, pos, "tfpi:"+recDate::GetIdStr( dateID ) );
        xmlCreateLink( link->GetNext(), 2, -1, "tfpi:"+recPlace::GetIdStr( placeID ) );
        xmlCreateLink( chrisEventCell, 0, 11, "tfp:"+recEventa::GetIdStr( eaID ) );
    }
    if( !deathStr.IsEmpty() ) {
        dateID = CreateDate( deathStr, refID, &refSeq );
        placeID = 0;
        eaID = CreateDeathEventa( refID, perID, dateID, placeID, &refSeq );
        LinkOrCreateEventFromEventa( eaID );
        xmlCreateLink( deathCell, "tfpi:"+recDate::GetIdStr( dateID ) );
        xmlCreateLink( deathEventCell, 0, 5, "tfp:"+recEventa::GetIdStr( eaID ) );
    }
    if( !burialStr.IsEmpty() ) {
        dateID = CreateDate( burialStr, refID, &refSeq );
        placeID = 0;
        eaID = CreateBurialEventa( refID, perID, dateID, placeID, &refSeq );
        LinkOrCreateEventFromEventa( eaID );
        xmlCreateLink( burialCell, "tfpi:"+recDate::GetIdStr( dateID ) );
        xmlCreateLink( burialEventCell, 0, 6, "tfp:"+recEventa::GetIdStr( eaID ) );
    }
    if( fatherPerID ) {
        eaID = CreateFamilyRelEventa( refID, fatherPerID, refDateID, 0, &refSeq );
        AddPersonaToEventa( eaID, motherPerID, recEventTypeRole::ROLE_Family_Wife );
    } else if( motherPerID ) {
        eaID = CreateFamilyRelEventa( refID, motherPerID, refDateID, 0, &refSeq );
    } else {
        eaID = 0;
    }
    if( eaID ) {
        AddPersonaToEventa( eaID, perID, recEventTypeRole::ROLE_Family_Child );
        recEventa::CreateFamilyLink( eaID );
    }
}

enum IntRefReturn {
    INTREF_Done, INTREF_Custom
};

IntRefReturn InterpretRef( idt refID, const wxString& h1Class, const wxString& title, wxXmlNode* refNode )
{
    recIdVec indList;
    wxArrayString names;
    ListIndividuals( refNode, indList, names );
    wxString refFormat = refNode->GetAttribute( "id" );
    wxString refClass = refNode->GetAttribute( "class" );

    if( refClass == "custom" ) {
        return INTREF_Custom;
    }

    wxString refStr = "<!-- HTML -->\n" + xmlGetSource( refNode );
    recReference ref(0);
    ref.FSetID( refID );
    ref.FSetTitle( title );
    ref.FSetStatement( refStr );
    ref.FSetUserRef( "RD"+recGetStr( refID ) );
    ref.Save();

    if( h1Class == "igi-chr" ) {
        CreateIgiBaptism( refID, refNode );
    } else if( refFormat == "census-tab" ) {
        long year;
        title.ToLong( &year );
        if( year == 1841 ) {
            CreateUkCensus( refID, g_1841CensusDateID, refNode, title );
        } else if( year == 1851 ) {
            CreateUkCensus( refID, g_1851CensusDateID, refNode, title );
        } else if( year == 1861 ) {
            CreateUkCensus( refID, g_1861CensusDateID, refNode, title );
        } else if( year == 1871 ) {
            CreateUkCensus( refID, g_1871CensusDateID, refNode, title );
        } else if( year == 1881 ) {
            CreateUkCensus( refID, g_1881CensusDateID, refNode, title );
        } else if( year == 1891 ) {
            CreateUkCensus( refID, g_1891CensusDateID, refNode, title );
        } else if( year == 1901 ) {
            Create1901UkCensus( refID, refNode, title );
        } else if( year == 1911 ) {
            CreateUkCensus( refID, g_1911CensusDateID, refNode, title );
        }
    } else {
        AddPersonas( refID, indList, names );
    }
    return INTREF_Done;
}

// If the reference file has markup (body element has id attribute - see rd00393.htm)
// then is processed by ProcessMarkupRef(...) else is processed by
// InterpretRef(...) or added to custom list.
void ProcessRefFile( const wxString path, idt refID, Filenames& customs )
{
    wxFileName fn( path );
    wxString title;

    wxXmlDocument doc;
    if( !doc.Load( fn.GetFullPath(), "UTF-8", wxXMLDOC_KEEP_WHITESPACE_NODES ) ) {
        wxPrintf( "\nRef ("ID") filename: [%s]\n\n", refID, fn.GetFullPath() );
        return;
    }

    wxXmlNode* root = doc.GetRoot();
    wxXmlNode* child = root->GetChildren();
    wxXmlNode* refNode = NULL;
    wxString idAttr;
    wxString h1Class;
    while( child ) {
        if (child->GetName() == "body") {
            idAttr = child->GetAttribute( "id" );
            if( idAttr.size() ) {
                ProcessMarkupRef( refID, root );
                return;
            }
            child = child->GetChildren();
            continue;
        } else if (child->GetName() == "h1") {
            h1Class = child->GetAttribute( "class" );
            title = xmlGetAllContent( child );
        } else if( child->GetName() == "div" ) {
            idAttr = child->GetAttribute( "id" );
            if( idAttr != "topmenu" && refNode == NULL ) {
                // We should be looking at reference text
                refNode = child;
                break;
            }
        }
        child = child->GetNext();
    }
    if( refNode ) {
        IntRefReturn ret;
        ret = InterpretRef( refID, h1Class, title, refNode );
        if( ret == INTREF_Custom ) {
            customs.push_back( fn );
        }
    }
}

bool InputRefFiles( const wxString& refFolder )
{
    CreateSourceGlobals();

    Filenames customs;
    wxString rddirname;

    wxDir dir( refFolder );
    if( !dir.Open( refFolder ) ) {
        return false;
    }
    bool cont = dir.GetFirst( &rddirname, "rd??", wxDIR_DIRS );
    while( cont ) {
        wxPrintf( "." );
        // Process Directory
        wxDir rddir;
        wxString rdfilename;
        if( !rddir.Open( refFolder + "/" + rddirname ) ) {
            return false;
        }
        cont = rddir.GetFirst( &rdfilename, "rd?????.htm", wxDIR_FILES );
        while( cont ) {
            idt refID = recGetID( rdfilename.substr( 2 ) );
            wxString path = refFolder + "/" + rddirname + "/" + rdfilename;
            ProcessRefFile( path, refID, customs );
            cont = rddir.GetNext( &rdfilename );
        }
        cont = dir.GetNext( &rddirname );
    }
    wxPrintf( "custom" );
    for( size_t i = 0 ; i < customs.size() ; i++ ) {
        wxPrintf( "." );
        ProcessCustomFile( customs[i] );
    }

    return true;
}

// End of nkRefDocuments.cpp file

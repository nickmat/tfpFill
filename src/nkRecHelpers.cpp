/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Name:        nkRefDocuments.cpp
 * Project:     tfpnick: Private utility to create Matthews TFP database
 * Purpose:     Read and process Reference Document files
 * Author:      Nick Matthews
 * Website:     http://thefamilypack.org
 * Created:     23rd September 2011
 * Copyright:   Copyright (c) 2011 ~ 2022, Nick Matthews.
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

#include <map>
#include <rec/recDb.h>

#include "nkMain.h"

// Mirrors  tfpExportGedcom
bool ExportGedcom( const wxString& path )
{
    wxFileName fname( path );
    wxFFileOutputStream outfile( fname.GetFullPath() );
    if( !outfile.IsOk() ) return false;
    try {
        recGedExport ged( outfile );
        if( !ged.Export() ) {
            return false;
        }

    } catch( wxSQLite3Exception& e ) {
        recDb::ErrorMessage( e );
        recDb::Rollback();
        return false;
    }
    return true;
}

void UpdateOccupationEvents()
{
    recIdVec eveIDs = recEvent::GetTypeOfIDs( -18 );
    for( idt eveID : eveIDs ) {
        recEvent eve( eveID );
        // TODO: Improve this title.
        eve.FSetTitle( eve.FGetTitle() + " Summary" );
        eve.Save();
        recIndEventVec ies = eve.GetIndividualEvents();
        if( ies.size() != 1 ) {
            wxASSERT( true );
            continue;
        }
        recEventTypeRole etr( ies[0].FGetRoleID() );
        ies[0].FSetNote( etr.FGetName() );
        ies[0].FSetRoleID( recEventTypeRole::ROLE_Occupation_Summary );
        ies[0].Save();
        if( etr.FGetID() > 0 ) {
            etr.Delete();
        }
    }
    // Tidy up loose ends.
    recIdVec etrIDs = recEventTypeRole::IdVec( recDb::Coverage::user );
    for( idt etrID : etrIDs ) {
        recEventTypeRole::DeleteIfOrphaned( etrID );
    }
}

wxString TrimBoth( const wxString& str )
{
    wxString s = str;
    return s.Trim( true ).Trim( false );
}

idt CreateDate( const wxString& date, idt refID, int* pseq )
{
    idt dateID = recDate::Create( date );
    if( refID ) {
        recReferenceEntity::Create( refID, recReferenceEntity::TYPE_Date, dateID, pseq );
    }
    return dateID;
}

idt CreateDateFromAge( long age, idt baseID, idt refID, int* pseq )
{
    if( baseID == 0 ) return 0;
    recDate base(baseID);
    recRelativeDate rel(0);
    rel.f_val = age;
    rel.f_range = 1;
    rel.f_unit = CALENDAR_UNIT_Year;
    rel.f_base_id = baseID;
    rel.f_type = recRelativeDate::TYPE_AgeRoundDown;
    rel.f_scheme = base.f_record_sch;
    rel.Save();
    recDate date;
    date.SetDefaults();
    date.f_rel_id = rel.f_id;
    date.CreateUidChanged();
    rel.CalculateDate( &date );
    date.Save();
    if( refID ) {
        recReferenceEntity::Create( refID, recReferenceEntity::TYPE_Date, date.f_id, pseq );
    }
    return date.f_id;
}

idt CreatePlace( const wxString& address, idt refID, int* pseq )
{
    if( address.IsEmpty() ) return 0;
    recPlace p(0);
    p.CreateUidChanged();
    p.Save();
    recPlacePart pp(0);
    pp.f_place_id = p.f_id;
    pp.f_type_id = recPlacePartType::TYPE_Address;
    pp.f_val = address;
    pp.Save();
    if( refID ) {
        recReferenceEntity::Create( refID, recReferenceEntity::TYPE_Place, p.f_id, pseq );
    }
    return p.f_id;
}

idt CreateCitationPart(
    const wxString& citation, const wxString& partStr, idt cptID, idt citID )
{
    size_t pos1 = citation.find( partStr );
    if( pos1 == wxString::npos ) {
        return 0;
    }
    size_t pos2 = citation.find( '\n', pos1 );
    pos1 += partStr.length();
    size_t len = wxString::npos;
    if( pos2 != wxString::npos ) {
        len = pos2 - pos1;
    }
    wxString str = citation.substr( pos1, len );
    if( str.empty() ) {
        return 0;
    }
    recCitationPart cp( 0 );
    cp.FSetCitID( citID );
    cp.FSetTypeID( cptID );
    cp.FSetValue( str );
    int seq = cp.GetNextCitationSeq( citID );
    cp.FSetCitSeq( seq );
    cp.Save();
    return cp.FGetID();
}

idt CreateCitation( const wxString& citation, idt higher_citID, idt refID )
{
    if( citation.empty() || higher_citID == 0 ) return 0;
    recCitation cit( 0 );
    cit.FSetHigherID( higher_citID );
    cit.FSetRefID( refID );
    int seq = cit.GetNextRefSequence( refID );
    cit.FSetRefSeq( seq );
    cit.CreateUidChanged();
    cit.Save();
    idt citID = cit.FGetID();

    CreateCitationPart( citation, "Piece: ", -5, citID );
    CreateCitationPart( citation, "Folio: ", -7, citID );
    CreateCitationPart( citation, "Page: ", -8, citID );
//    CreateCitationPart( citation, "Schedule: ", -13, citID );

    return citID;
}

idt CreateCensusEvent( const wxString& title, idt dID, idt pID, idt rID )
{
    recEventa ev(0);
    ev.f_title = title;
    ev.FSetRefID( rID );
    ev.f_type_id = recEventType::ET_Census;
    ev.f_date1_id = dID;
    ev.f_place_id = pID;
    ev.f_date_pt = recDate::GetDatePoint( dID );
    ev.CreateUidChanged();
    ev.Save();
    return ev.f_id;
}

idt GetWifeName( const wxString& nameStr, idt husbandNameID, idt refID, int* pseq )
{
    if( nameStr.IsEmpty() ) return 0;
    recName husb(husbandNameID);
    wxString surname = husb.GetSurname();
    recName name(0);
    name.f_style_id = recNameStyle::NS_Married;
    name.Save();
    name.AddNameParts( nameStr );
    if( refID ) {
        recReferenceEntity::Create( refID, recReferenceEntity::TYPE_Name, name.f_id, pseq );
    }
    if( name.GetSurname() == surname ) {
        return name.f_id;
    }
    recNamePartVec parts = name.GetParts();
    size_t i;
    for( i = 0 ; i < parts.size() ; i++ ) {
        parts[i].f_type_id = NAME_TYPE_Given_name;
    }
    recNamePart part(0);
    part.f_name_id = name.f_id;
    part.f_type_id = NAME_TYPE_Surname;
    part.f_val = surname;
    part.f_sequence = i;
    part.Save();
    return name.f_id;
}

idt CreateName( const wxString& name, idt style )
{
    return recName::CreateName( ProperCase( name ), style );
}

idt CreatePerName( const wxString & nameStr, idt perID, idt style )
{
    if ( perID == 0 ) {
        return 0;
    }
    recName name( 0 );
    name.FSetPerID( perID );
    name.FSetTypeID( style );
    name.SetNextSequence();
    name.Save();

    name.AddNameParts( ProperCase( nameStr ) );
    return name.FGetID();
}

idt CreatePersona( idt refID, idt indID, idt nameID, Sex sex )
{
    recPersona per(0);
    per.f_sex = sex;
    per.f_ref_id = refID;
    per.CreateUidChanged();
    per.Save();

    recName name(nameID);
    name.f_per_id = per.f_id;
    name.f_sequence = 1;
    name.Save();

    if( indID && !recIndividual::Exists( indID ) ) {
        CreateIndividual( indID, per.FGetID() );
    }
    if( recIndividual::Exists( indID ) ) {
        recIndividualPersona lp(0);
        lp.f_ind_id = indID;
        lp.f_per_id = per.f_id;
        lp.f_conf = 0.999;
        lp.Save();
    }
    return per.f_id;
}

idt CreatePersona( idt refID, idt indID, const wxString& nameStr, Sex sex, int* pseq, const wxString& note )
{
    recPersona per(0);
    per.FSetSex( sex );
    per.FSetRefID( refID );
    per.FSetNote( note );
    per.CreateUidChanged();
    per.Save();

    recName name(0);
    name.FSetPerID( per.FGetID() );
    name.FSetSequence( 1 );
    name.Save();
    name.AddNameParts( ProperCase( nameStr ) );
    recReferenceEntity::Create( refID, recReferenceEntity::TYPE_Name, name.f_id );

    if( indID && !recIndividual::Exists( indID ) ) {
        CreateIndividual( indID, per.FGetID() );
    }
    if( recIndividual::Exists( indID ) ) {
        recIndividualPersona lp(0);
        lp.FSetIndID( indID );
        lp.FSetPerID( per.FGetID() );
        lp.FSetConf( 0.999 );
        lp.Save();
    }
    return per.FGetID();
}

void AddName( idt refID, idt perID, const wxString& nameStr, int* pseq )
{
    recName name(0);
    name.FSetPerID( perID );
    name.SetNextSequence();
    name.Save();
    name.AddNameParts( ProperCase( nameStr ) );
    recReferenceEntity::Create( refID, recReferenceEntity::TYPE_Name, name.f_id, pseq );
}

void AddPersonas( idt refID, recIdVec& indList, wxArrayString& names )
{
    if( recReference::GetPersonaCount( refID ) > 0 ) return;

    std::map< idt, idt > IndPerMap;
    int seq = 0;
    for( size_t i = 0 ; i < indList.size() ; i++ ) {
        idt indID = indList[i];
        if( IndPerMap.count(indID) > 0 ) {
            AddName( refID, IndPerMap[indID], names[i], &seq );
        } else {
            IndPerMap[indID] = CreatePersona( refID, indID, names[i], Sex::unstated, &seq );
        }
    }
}

idt CreateBirthEvent( idt refID, idt perID, idt dateID, idt placeID )
{
    recEventa eventa(0);
    eventa.FSetTitle( "Birth of " + recPersona::GetNameStr( perID ) );
    eventa.FSetRefID( refID );
    eventa.FSetTypeID( recEventType::ET_Birth );
    eventa.FSetDate1ID( dateID );
    eventa.FSetPlaceID( placeID );
    eventa.UpdateDatePoint();
    eventa.CreateUidChanged();
    eventa.Save();
    idt eaID = eventa.FGetID();

    recEventaPersona ep(0);
    ep.FSetEventaID( eaID );
    ep.FSetPerID( perID );
    ep.FSetRoleID( recEventTypeRole::ROLE_Birth_Born );
    ep.FSetPerSeq( 1 );
    ep.Save();

//    idt eveID = LinkOrCreateEventFromEventa( eventa.FGetID() );
//    assert( eveID != 0 );

    eventa.CreatePersonalEvent();
    return eaID;
}

idt CreateRegBirthEvent( idt refID, idt perID, idt dateID, idt placeID )
{
    recEventa eventa(0);
    eventa.f_title = "Registered birth of " + recPersona::GetNameStr( perID );
    eventa.FSetRefID( refID );
    eventa.f_type_id = recEventType::ET_RegBirth;
    eventa.f_date1_id = dateID;
    eventa.f_place_id = placeID;
    eventa.UpdateDatePoint();
    eventa.CreateUidChanged();
    eventa.Save();

    recEventaPersona ep(0);
    ep.f_eventa_id = eventa.f_id;
    ep.f_per_id = perID;
    ep.f_role_id = recEventTypeRole::ROLE_RegBirth_Born;
    ep.f_per_seq = 1;
    ep.Save();

    idt eveID = LinkOrCreateEventFromEventa( eventa.FGetID() );
//    assert( eveID != 0 );
    return eventa.f_id;
}

idt CreateChrisEventa( idt refID, idt perID, idt dateID, idt placeID )
{
    recEventa eventa(0);
    eventa.f_title = "Baptism of " + recPersona::GetNameStr( perID );
    eventa.FSetRefID( refID );
    eventa.f_type_id = recEventType::ET_Baptism;
    eventa.f_date1_id = dateID;
    eventa.f_place_id = placeID;
    eventa.UpdateDatePoint();
    eventa.CreateUidChanged();
    eventa.Save();

    recEventaPersona ep(0);
    ep.f_eventa_id = eventa.f_id;
    ep.f_per_id = perID;
    ep.f_role_id = recEventTypeRole::ROLE_Baptism_Baptised;
    ep.f_per_seq = 1;
    ep.Save();

//    idt eveID = LinkOrCreateEventFromEventa( eventa.FGetID() );
//    assert( eveID != 0 );
    return eventa.f_id;
}

idt CreateMarriageEvent( idt perID, idt dateID, idt placeID, long role, idt refID )
{
    recEventa eventa(0);
    eventa.f_title = "Marriage of " + recPersona::GetNameStr( perID );
    eventa.FSetRefID( refID );
    eventa.f_type_id = recEventType::ET_Marriage;
    eventa.f_date1_id = dateID;
    eventa.f_place_id = placeID;
    eventa.UpdateDatePoint();
    eventa.CreateUidChanged();
    eventa.Save();

    recEventaPersona ep(0);
    ep.f_eventa_id = eventa.f_id;
    ep.f_per_id = perID;
    ep.f_role_id = role;
    ep.f_per_seq = 1;
    ep.Save();

    idt eveID = LinkOrCreateEventFromEventa( eventa.FGetID() );
//    assert( eveID != 0 );
    return eventa.f_id;
}

idt CreateDeathEventa( idt refID, idt perID, idt dateID, idt placeID )
{
    recEventa eventa(0);
    eventa.f_title = "Death of " + recPersona::GetNameStr( perID );
    eventa.FSetRefID( refID );
    eventa.f_type_id = recEventType::ET_Death;
    eventa.f_date1_id = dateID;
    eventa.f_place_id = placeID;
    eventa.UpdateDatePoint();
    eventa.CreateUidChanged();
    eventa.Save();

    recEventaPersona ep(0);
    ep.f_eventa_id = eventa.f_id;
    ep.f_per_id = perID;
    ep.f_role_id = recEventTypeRole::ROLE_Death_Died;
    ep.f_per_seq = 1;
    ep.Save();

//    idt eveID = LinkOrCreateEventFromEventa( eventa.FGetID() );
//    assert( eveID != 0 );
    return eventa.f_id;
}

idt CreateRegDeathEvent( idt refID, idt perID, idt dateID, idt placeID )
{
    recEventa eventa(0);
    eventa.f_title = "Registered death of " + recPersona::GetNameStr( perID );
    eventa.FSetRefID( refID );
    eventa.f_type_id = recEventType::ET_RegDeath;
    eventa.f_date1_id = dateID;
    eventa.f_place_id = placeID;
    eventa.UpdateDatePoint();
    eventa.CreateUidChanged();
    eventa.Save();

    recEventaPersona ep(0);
    ep.f_eventa_id = eventa.f_id;
    ep.f_per_id = perID;
    ep.f_role_id = recEventTypeRole::ROLE_RegDeath_Died;
    ep.f_per_seq = 1;
    ep.Save();

    idt eveID = LinkOrCreateEventFromEventa( eventa.FGetID() );
//    assert( eveID != 0 );
    return eventa.f_id;
}

idt CreateBurialEventa( idt refID, idt perID, idt dateID, idt placeID )
{
    recEventa ea(0);
    ea.FSetTitle( "Burial of " + recPersona::GetNameStr( perID ) );
    ea.FSetRefID( refID );
    ea.FSetTypeID( recEventType::ET_Burial );
    ea.FSetDate1ID( dateID );
    ea.FSetPlaceID( placeID );
    ea.UpdateDatePoint();
    ea.CreateUidChanged();
    ea.Save();

    recEventaPersona ep(0);
    ep.FSetEventaID( ea.FGetID() );
    ep.FSetPerID( perID );
    ep.FSetRoleID( recEventTypeRole::ROLE_Burial_Deceased );
    ep.FSetPerSeq( 1 );
    ep.Save();

    return ea.FGetID();
}

idt CreateResidenceEventa( idt dateID, idt placeID, idt refID )
{
    recEventa ea( 0 );
    ea.FSetTitle( "Household at " + recPlace::GetAddressStr( placeID ) );
    ea.FSetRefID( refID );
    ea.FSetTypeID( recEventType::ET_Residence );
    ea.FSetDate1ID( dateID );
    ea.FSetPlaceID( placeID );
    ea.UpdateDatePoint();
    ea.CreateUidChanged();
    ea.Save();
    idt eaID = ea.FGetID();

    return eaID;
}

idt CreateMediaEventa( idt refID )
{
    recEventa ea( 0 );
    ea.FSetTitle( recReference::GetTitle( refID ) );
    ea.FSetRefID( refID );
    ea.FSetTypeID( recEventType::ET_Media );
    ea.FSetDate1ID( 0 );
    ea.FSetPlaceID( 0 );
    ea.UpdateDatePoint();
    ea.CreateUidChanged();
    ea.Save();
    idt eaID = ea.FGetID();

    return eaID;
}

idt CreateFamilyRelEventa( idt refID, idt perID, idt dateID, idt placeID )
{
    recEventa e(0);
    e.FSetTitle( "Family of " + recPersona::GetNameStr( perID ) );
    e.FSetRefID( refID );
    e.FSetTypeID( recEventType::ET_Family );
    e.FSetDate1ID( dateID );
    e.FSetPlaceID( placeID );
    e.UpdateDatePoint();
    e.CreateUidChanged();
    e.Save();

    recEventaPersona ep(0);
    ep.FSetEventaID( e.FGetID() );
    ep.FSetPerID( perID );
    recEventTypeRole::Role role;
    switch( recPersona::GetSex( perID ) )
    {
    case Sex::male:
        role = recEventTypeRole::ROLE_Family_Husband;
        break;
    case Sex::female:
        role = recEventTypeRole::ROLE_Family_Wife;
        break;
    default:
        role = recEventTypeRole::ROLE_Family_Partner;
    }
    ep.FSetRoleID( role );
    ep.FSetPerSeq( 1 );
    ep.Save();

    idt eveID = LinkOrCreateEventFromEventa( e.FGetID() );
//    assert( eveID != 0 );
    return e.FGetID();
}

void AddPersonaToEventa( idt eaID, idt perID, idt roleID, const wxString& note )
{
    if( eaID == 0 || perID == 0 ) return;
    recEventaPersona ep(0);
    ep.FSetRoleID( roleID );
    ep.FSetEventaID( eaID );
    ep.FSetPerID( perID );
    ep.FSetNote( note );
    ep.SetNextPerSequence( eaID );
    ep.Save();
}

idt ClassifyOccupation( const wxString& description )
{
    wxString desc = description.Lower();
    if( desc.empty() ) return recEventTypeRole::ROLE_Occ_Dependant;
    if( desc.find( "student" ) != wxString::npos ) return recEventTypeRole::ROLE_Occ_Student;
    if( desc.find( "scholar" ) != wxString::npos ) return recEventTypeRole::ROLE_Occ_Student;
    if( desc.find( "servant" ) != wxString::npos ) return recEventTypeRole::ROLE_Occ_Service;
    if( desc.find( "labo" ) != wxString::npos ) return recEventTypeRole::ROLE_Occ_Labourer;
    return recEventTypeRole::ROLE_Occupation_Other;
}

idt CreateOccupation( const wxString& occ, idt refID, idt perID, idt dateID )
{
    wxString occStr = TrimBoth( occ );
    if( occStr.empty() ) return 0;
    idt roleID = ClassifyOccupation( occStr );

    recEventa ea(0);
    ea.FSetRefID( refID );
    ea.FSetTypeID( recEventType::ET_Occupation );
    ea.FSetDate1ID( dateID );
    ea.UpdateDatePoint();
    ea.CreateUidChanged();
    ea.SetAutoTitle( recPersona::GetNameStr( perID ) );
    ea.Save();
    idt eaID = ea.FGetID();

    recEventaPersona ep(0);
    ep.FSetEventaID( eaID );
    ep.FSetPerID( perID );
    ep.FSetRoleID( roleID );
    ep.FSetNote( occStr );
    ep.FSetPerSeq( 1 );
    ep.Save();
#if 0
    recIdVec indIDs = recPersona::GetIndividualIDs( perID );
    if( indIDs.size() != 1 ) {
        return eaID;
    }
    idt indID = indIDs[0];
    idt heID = recIndividual::FindEvent( indID, recEventTypeRole::Role::ROLE_Occupation_Summary );
    if( heID == 0 ) {
        recEvent eve( 0 );
        eve.FSetTitle( "Occupation Summary of " + recIndividual::GetName( indID ) );
        eve.FSetTypeID( recEventType::EType::ET_Occupation );
        eve.Save();

        heID = eve.FGetID();
        recIndividualEvent ieve( 0 );
        ieve.FSetIndID( indID );
        ieve.FSetEventID( heID );
        ieve.FSetRoleID( recEventTypeRole::Role::ROLE_Occupation_Summary );
        ieve.Save();
    }
    idt eID = recEvent::CreateFromEventa( eaID );

    recIndividualEvent ie( 0 );
    ie.FSetHigherID( heID );
    ie.FSetIndID( indID );
    ie.FSetEventID( eID );
    ie.FSetRoleID( roleID );
    ie.FSetNote( occ );
    ie.FSetIndSeq( 1 );
    ie.Save();

    recEventEventa eea( 0 );
    eea.FSetEventID( eID );
    eea.FSetEventaID( eaID );
    eea.FSetConf( 0.999 );
    eea.Save();
#endif
    recEventa::CreatePersonalEvent( eaID );
    return eaID;
}

idt CreateCondition( const wxString& con, idt refID, idt perID, idt dateID )
{
    if( con.empty() ) return 0;

    idt conID = recEventTypeRole::FindOrCreate( con, recEventType::ET_Condition );

    recEventa er(0);
    er.FSetRefID( refID );
    er.FSetTypeID( recEventType::ET_Condition );
    er.FSetDate1ID( dateID );
    er.UpdateDatePoint();
    er.CreateUidChanged();
    er.SetAutoTitle( recPersona::GetNameStr( perID ) );
    er.Save();
    idt erID = er.FGetID();

    recEventaPersona ep(0);
    ep.f_eventa_id = erID;
    ep.f_per_id = perID;
    ep.f_role_id = conID;
    ep.f_per_seq = 1;
    ep.Save();

    idt eveID = LinkOrCreateEventFromEventa( erID );
//    assert( eveID != 0 );
    return erID;
}
idt CreateRelationship( idt per1ID, const wxString& des, idt per2ID, idt refID, int* pseq )
{
    return 0;
#if 0
    if( per1ID == 0 || per2ID == 0 || des.IsEmpty() ) return 0;
    recRelationship rel(0);
    rel.f_per1_id = per1ID;
    rel.f_per2_id = per2ID;
    rel.f_descrip = des;
    rel.Save();
    if( refID ) {
        recReferenceEntity::Create( refID, recReferenceEntity::TYPE_Relationship, rel.f_id, pseq );
    }
    return rel.f_id;
#endif
}
void rSetPersonaSex( idt perID, Sex sex )
{
    if( perID ) {
        recPersona per(perID);
        per.f_sex = sex;
        per.Save();
    }
}

void rAddNameToPersona( idt perID, idt nameID )
{
    recName name(nameID);
    name.FSetPerID( perID );
    name.SetNextSequence();
    name.Save();
}

idt LinkOrCreateEventFromEventa( idt eaID )
{
    if( eaID == 0 ) {
        return 0;
    }
    idt eID = 0;
    recEventa ea(eaID);
    recEventType et( ea.FGetTypeID() );
    recIdVec eIDs = ea.FindLinkedEventsViaInd();
    if( et.FGetGrp() == recEventTypeGrp::birth ||
        et.FGetGrp() == recEventTypeGrp::death ||
        et.FGetGrp() == recEventTypeGrp::personal
    ) {
        if( eIDs.empty() ) {
            return CreateEventFromEventa( eaID );
        }
        double conf = 0.999 / eIDs.size();
        for( size_t i = 0 ; i < eIDs.size() ; i++ ) {
            recEventEventa::Create( eIDs[i], eaID, conf );
            recEvent::SetDatePeriodToInclude( eIDs[i], ea.FGetDate1ID() );
            recEvent::CreateRolesFromEventa( eIDs[i], eaID );
        }
        eID = eIDs[0];
//    } else if(
//        et.FGetGrp() == recET_GRP_NrBirth ||
//        et.FGetGrp() == recET_GRP_NrDeath
//    ) {
    } else {
        recIdVec matched_eIDs;
        recDate date1(ea.FGetDate1ID()); 
        for( size_t i = 0 ; i < eIDs.size() ; i++ ) {
            recEvent eve(eIDs[i]);
            if( eve.FGetDate1ID() == 0 ) {
                continue;
            }
            recDate existDate1(eve.FGetDate1ID());
            unsigned comp = date1.GetCompareFlags( existDate1 );
            if( comp & recDate::CF_Overlap || comp & recDate::CF_WithinType ) {
                matched_eIDs.push_back( eIDs[i] );
            }
        }
        if( matched_eIDs.empty() ) {
            return CreateEventFromEventa( eaID );
        }
        double conf = 0.999 / matched_eIDs.size();
        for( size_t i = 0 ; i < matched_eIDs.size() ; i++ ) {
            recEventEventa::Create( matched_eIDs[i], eaID, conf );
            recEvent::SetDatePeriodToInclude( matched_eIDs[i], ea.FGetDate1ID() );
            recEvent::CreateRolesFromEventa( matched_eIDs[i], eaID );
        }
        eID = eIDs[0];
//    } else {
//        wxASSERT( false ); // TODO:                           <<=============<<<<<<
//        eID = 0;
    }
    return eID;
}

idt CreateEventFromEventa( idt eaID )
{
    idt eID = recEvent::CreateFromEventa( eaID );
    recEventEventa::Create( eID, eaID );
    recEvent::CreateRolesFromEventa( eID, eaID );
    return eID;
}

bool CreateIndividual( idt indID, idt perID )
{
    if( recIndividual::Exists( indID ) ) {
        return false;
    }
    recIndividual ind(0);
    ind.FSetID( indID );
    Sex sex = recPersona::GetSex( perID );
    ind.FSetSex( sex );
    recFamily fam(0);
    if( sex != Sex::female ) {
        fam.FSetHusbID( indID );
    } else {
        fam.FSetWifeID( indID );
    }
    fam.Save();
    ind.FSetFamID( fam.FGetID() );
    recName::CreateIndNameFromPersona( indID, perID );
    ind.UpdateNames();
    ind.Save();
    return true;
}

Sex GetSexFromStr( const wxString& str )
{
    if( !str.IsEmpty() ) {
        wxChar ch = str[0];
        if( ch == 'M' || ch == 'm' ) return Sex::male;
        if( ch == 'F' || ch == 'f' ) return Sex::female;
    }
    return Sex::unknown;
}

wxString GetConditionStr( Sex sex, const wxString& cond )
{
    wxString str = cond;
    str.Trim();
    if( !str.empty() ) {
        switch( str[0].GetValue() )
        {
        case 'M': case 'm':
            return "Married";
        case 'U': case 'u':
        case 'S': case 's':
            return "Single";
        case 'W': case 'w':
            if( sex == Sex::female ) {
                return "Widow";
            }
            return "Widower";
        case '-':
            return wxEmptyString;
        }
    }
    return str;
}

wxString ProperCase( const wxString& str )
{
    wxString output;
    bool start = true;
    for ( auto ch : str ) {
        if ( ch == ' ' || ch == '.' ) {
            start = true;
            output += ' ';
            continue;
        }
        if ( start ) {
            start = false;
            output += toupper( ch );
        } else {
            output += tolower( ch );
        }
    }
    return output;
}

// End of nkRefDocuments.cpp file

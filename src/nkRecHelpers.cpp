/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Name:        nkRefDocuments.cpp
 * Project:     tfpnick: Private utility to create Matthews TFP database
 * Purpose:     Read and process Reference Document files
 * Author:      Nick Matthews
 * Website:     http://thefamilypack.org
 * Created:     23rd September 2011
 * Copyright:   Copyright (c) 2011 - 2016, Nick Matthews.
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

idt CreateCensusEvent( const wxString& title, idt dID, idt pID, idt rID, int* pseq )
{
    recEventa ev(0);
    ev.f_title = title;
    ev.f_type_id = recEventType::ET_Census;
    ev.f_date1_id = dID;
    ev.f_place_id = pID;
    ev.f_date_pt = recDate::GetDatePoint( dID );
    ev.Save();
    recReferenceEntity::Create( rID, recReferenceEntity::TYPE_Event, ev.f_id, pseq );
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

idt CreateName( const wxString& surname, const wxString& givens )
{
    recName name(0);
    name.Save();
    int seq = name.AddNameParts( givens, NAME_TYPE_Given_name );
    name.AddNamePart( surname, NAME_TYPE_Surname, seq );
    return name.f_id;
}

idt CreatePersona( idt refID, idt indID, idt nameID, Sex sex )
{
    recPersona per(0);
    per.f_sex = sex;
    per.f_ref_id = refID;
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

idt CreatePersona( idt refID, idt indID, const wxString& nameStr, Sex sex, int* pseq )
{
    recPersona per(0);
    per.FSetSex( sex );
    per.FSetRefID( refID );
    per.Save();

    recName name(0);
    name.FSetPerID( per.FGetID() );
    name.FSetSequence( 1 );
    name.Save();
    name.AddNameParts( nameStr );
    recReferenceEntity::Create( refID, recReferenceEntity::TYPE_Name, name.f_id, pseq );

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
    name.AddNameParts( nameStr );
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
            IndPerMap[indID] = CreatePersona( refID, indID, names[i], SEX_Unstated, &seq );
        }
    }
}

idt CreateBirthEvent( idt refID, idt perID, idt dateID, idt placeID, int* pseq )
{
    recEventa eventa(0);
    eventa.FSetTitle( "Birth of " + recPersona::GetNameStr( perID ) );
    eventa.FSetTypeID( recEventType::ET_Birth );
    eventa.FSetDate1ID( dateID );
    eventa.FSetPlaceID( placeID );
    eventa.UpdateDatePoint();
    eventa.Save();
    recReferenceEntity::Create(
        refID, recReferenceEntity::TYPE_Event, eventa.FGetID(), pseq );

    recEventaPersona ep(0);
    ep.FSetEventaID( eventa.FGetID() );
    ep.FSetPerID( perID );
    ep.FSetRoleID( recEventTypeRole::ROLE_Birth_Born );
    ep.FSetPerSeq( 1 );
    ep.Save();

//    idt eveID = LinkOrCreateEventFromEventa( eventa.FGetID() );
//    assert( eveID != 0 );

    return eventa.FGetID();
}

idt CreateRegBirthEvent( idt refID, idt perID, idt dateID, idt placeID, int* pseq )
{
    recEventa eventa(0);
    eventa.f_title = "Registered birth of " + recPersona::GetNameStr( perID );
    eventa.f_type_id = recEventType::ET_RegBirth;
    eventa.f_date1_id = dateID;
    eventa.f_place_id = placeID;
    eventa.UpdateDatePoint();
    eventa.Save();
    recReferenceEntity::Create( refID, recReferenceEntity::TYPE_Event, eventa.f_id, pseq );

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

idt CreateChrisEventa( idt refID, idt perID, idt dateID, idt placeID, int* pseq )
{
    recEventa eventa(0);
    eventa.f_title = "Baptism of " + recPersona::GetNameStr( perID );
    eventa.f_type_id = recEventType::ET_Baptism;
    eventa.f_date1_id = dateID;
    eventa.f_place_id = placeID;
    eventa.UpdateDatePoint();
    eventa.Save();
    recReferenceEntity::Create( refID, recReferenceEntity::TYPE_Event, eventa.f_id, pseq );

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

idt CreateMarriageEvent( idt perID, idt dateID, idt placeID, long role, idt refID, int* pseq )
{
    recEventa eventa(0);
    eventa.f_title = "Marriage of " + recPersona::GetNameStr( perID );
    eventa.f_type_id = recEventType::ET_Marriage;
    eventa.f_date1_id = dateID;
    eventa.f_place_id = placeID;
    eventa.UpdateDatePoint();
    eventa.Save();
    recReferenceEntity::Create( refID, recReferenceEntity::TYPE_Event, eventa.f_id, pseq );

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

idt CreateDeathEventa( idt refID, idt perID, idt dateID, idt placeID, int* pseq )
{
    recEventa eventa(0);
    eventa.f_title = "Death of " + recPersona::GetNameStr( perID );
    eventa.f_type_id = recEventType::ET_Death;
    eventa.f_date1_id = dateID;
    eventa.f_place_id = placeID;
    eventa.UpdateDatePoint();
    eventa.Save();
    recReferenceEntity::Create( refID, recReferenceEntity::TYPE_Event, eventa.f_id, pseq );

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

idt CreateRegDeathEvent( idt refID, idt perID, idt dateID, idt placeID, int* pseq )
{
    recEventa eventa(0);
    eventa.f_title = "Registered death of " + recPersona::GetNameStr( perID );
    eventa.f_type_id = recEventType::ET_RegDeath;
    eventa.f_date1_id = dateID;
    eventa.f_place_id = placeID;
    eventa.UpdateDatePoint();
    eventa.Save();
    recReferenceEntity::Create( refID, recReferenceEntity::TYPE_Event, eventa.f_id, pseq );

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

idt CreateBurialEventa( idt refID, idt perID, idt dateID, idt placeID, int* pseq )
{
    recEventa ea(0);
    ea.FSetTitle( "Burial of " + recPersona::GetNameStr( perID ) );
    ea.FSetTypeID( recEventType::ET_Burial );
    ea.FSetDate1ID( dateID );
    ea.FSetPlaceID( placeID );
    ea.UpdateDatePoint();
    ea.Save();
    recReferenceEntity::Create( refID, recReferenceEntity::TYPE_Event, ea.FGetID(), pseq );

    recEventaPersona ep(0);
    ep.FSetEventaID( ea.FGetID() );
    ep.FSetPerID( perID );
    ep.FSetRoleID( recEventTypeRole::ROLE_Burial_Deceased );
    ep.FSetPerSeq( 1 );
    ep.Save();

    return ea.FGetID();
}

idt CreateResidenceEventa( idt dateID, idt placeID, idt refID, int* pseq )
{
    recEventa ea(0);
    ea.FSetTitle( "Household at " + recPlace::GetAddressStr( placeID ) );
    ea.FSetTypeID( recEventType::ET_Residence );
    ea.FSetDate1ID( dateID );
    ea.FSetPlaceID( placeID );
    ea.UpdateDatePoint();
    ea.Save();
    idt eaID = ea.FGetID();
    recReferenceEntity::Create( refID, recReferenceEntity::TYPE_Event, eaID, pseq );

    return eaID;
}

idt CreateFamilyRelEventa( idt refID, idt perID, idt dateID, idt placeID, int* pseq )
{
    recEventa e(0);
    e.FSetTitle( "Family of " + recPersona::GetNameStr( perID ) );
    e.FSetTypeID( recEventType::ET_Family );
    e.FSetDate1ID( dateID );
    e.FSetPlaceID( placeID );
    e.UpdateDatePoint();
    e.Save();
    recReferenceEntity::Create( refID, recReferenceEntity::TYPE_Event, e.FGetID(), pseq );

    recEventaPersona ep(0);
    ep.FSetEventaID( e.FGetID() );
    ep.FSetPerID( perID );
    recEventTypeRole::Role role;
    switch( recPersona::GetSex( perID ) )
    {
    case SEX_Male:
        role = recEventTypeRole::ROLE_Family_Husband;
        break;
    case SEX_Female:
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

void AddPersonaToEventa( idt eventID, idt perID, idt roleID )
{
    if( eventID == 0 || perID == 0 ) return;
    recEventaPersona ep(0);
    ep.f_eventa_id = eventID;
    ep.f_per_id = perID;
    ep.f_role_id = roleID;
    ep.f_per_seq = recEventa::GetLastPerSeqNumber( eventID ) + 1;
    ep.Save();
}
idt CreateOccupation( const wxString& occ, idt refID, idt perID, idt dateID, int* pseq )
{
    if( occ.empty() ) return 0;
    idt occID = recEventTypeRole::FindOrCreate( occ, recEventType::ET_Occupation );

    recEventa ea(0);
    ea.FSetTypeID( recEventType::ET_Occupation );
    ea.FSetDate1ID( dateID );
    ea.UpdateDatePoint();
    ea.SetAutoTitle( recPersona::GetNameStr( perID ) );
    ea.Save();
    idt eaID = ea.FGetID();
    recReferenceEntity::Create( refID, recReferenceEntity::TYPE_Event, eaID, pseq );

    recEventaPersona ep(0);
    ep.FSetEventaID( eaID );
    ep.FSetPerID( perID );
    ep.FSetRoleID( occID );
    ep.FSetPerSeq( 1 );
    ep.Save();

    idt eveID = LinkOrCreateEventFromEventa( eaID );
//    assert( eveID != 0 );
    return eaID;
}

idt CreateCondition( const wxString& con, idt refID, idt perID, idt dateID, int* pseq )
{
    if( con.empty() ) return 0;

    idt conID = recEventTypeRole::FindOrCreate( con, recEventType::ET_Condition );

    recEventa er(0);
    er.FSetTypeID( recEventType::ET_Condition );
    er.FSetDate1ID( dateID );
    er.UpdateDatePoint();
    er.SetAutoTitle( recPersona::GetNameStr( perID ) );
    er.Save();
    idt erID = er.FGetID();
    recReferenceEntity::Create( refID, recReferenceEntity::TYPE_Event, erID, pseq );

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
    recIdVec eIDs = ea.FindLinkedEvents( recEventa::recEVENT_Link_IndPer );
    if( et.FGetGrp() == recET_GRP_Birth ||
        et.FGetGrp() == recET_GRP_Death ||
        et.FGetGrp() == recET_GRP_Personal
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
    if( sex != SEX_Female ) {
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
        if( ch == 'M' || ch == 'm' ) return SEX_Male;
        if( ch == 'F' || ch == 'f' ) return SEX_Female;
    }
    return SEX_Unknown;
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
            if( sex = SEX_Female ) {
                return "Widow";
            }
            return "Widower";
        case '-':
            return wxEmptyString;
        }
    }
    return str;
}

// End of nkRefDocuments.cpp file

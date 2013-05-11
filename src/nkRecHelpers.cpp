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

#include <map>
#include <rec/recDb.h>

#include "nkMain.h"

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
    recEventRecord ev(0);
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
    recEventRecord event(0);
    event.f_title = "Birth of " + recPersona::GetNameStr( perID );
    event.f_type_id = recEventType::ET_Birth;
    event.f_date1_id = dateID;
    event.f_place_id = placeID;
    event.UpdateDatePoint();
    event.Save();
    recReferenceEntity::Create( refID, recReferenceEntity::TYPE_Event, event.f_id, pseq );

    recEventPersona ep(0);
    ep.f_event_rec_id = event.f_id;
    ep.f_per_id = perID;
    ep.f_role_id = recEventTypeRole::ROLE_Birth_Born;
    ep.f_per_seq = 1;
    ep.Save();

    return event.f_id;
}

idt CreateRegBirthEvent( idt refID, idt perID, idt dateID, idt placeID, int* pseq )
{
    recEventRecord event(0);
    event.f_title = "Registered birth of " + recPersona::GetNameStr( perID );
    event.f_type_id = recEventType::ET_RegBirth;
    event.f_date1_id = dateID;
    event.f_place_id = placeID;
    event.UpdateDatePoint();
    event.Save();
    recReferenceEntity::Create( refID, recReferenceEntity::TYPE_Event, event.f_id, pseq );

    recEventPersona ep(0);
    ep.f_event_rec_id = event.f_id;
    ep.f_per_id = perID;
    ep.f_role_id = recEventTypeRole::ROLE_RegBirth_Born;
    ep.f_per_seq = 1;
    ep.Save();

    return event.f_id;
}

idt CreateChrisEvent( idt refID, idt perID, idt dateID, idt placeID, int* pseq )
{
    recEventRecord event(0);
    event.f_title = "Baptism of " + recPersona::GetNameStr( perID );
    event.f_type_id = recEventType::ET_Baptism;
    event.f_date1_id = dateID;
    event.f_place_id = placeID;
    event.UpdateDatePoint();
    event.Save();
    recReferenceEntity::Create( refID, recReferenceEntity::TYPE_Event, event.f_id, pseq );

    recEventPersona ep(0);
    ep.f_event_rec_id = event.f_id;
    ep.f_per_id = perID;
    ep.f_role_id = recEventTypeRole::ROLE_Baptism_Baptised;
    ep.f_per_seq = 1;
    ep.Save();

    return event.f_id;
}

idt CreateMarriageEvent( idt perID, idt dateID, idt placeID, long role, idt refID, int* pseq )
{
    recEventRecord event(0);
    event.f_title = "Marriage of " + recPersona::GetNameStr( perID );
    event.f_type_id = recEventType::ET_Marriage;
    event.f_date1_id = dateID;
    event.f_place_id = placeID;
    event.UpdateDatePoint();
    event.Save();
    recReferenceEntity::Create( refID, recReferenceEntity::TYPE_Event, event.f_id, pseq );

    recEventPersona ep(0);
    ep.f_event_rec_id = event.f_id;
    ep.f_per_id = perID;
    ep.f_role_id = role;
    ep.f_per_seq = 1;
    ep.Save();

    return event.f_id;
}

idt CreateDeathEvent( idt refID, idt perID, idt dateID, idt placeID, int* pseq )
{
    recEventRecord event(0);
    event.f_title = "Death of " + recPersona::GetNameStr( perID );
    event.f_type_id = recEventType::ET_Death;
    event.f_date1_id = dateID;
    event.f_place_id = placeID;
    event.UpdateDatePoint();
    event.Save();
    recReferenceEntity::Create( refID, recReferenceEntity::TYPE_Event, event.f_id, pseq );

    recEventPersona ep(0);
    ep.f_event_rec_id = event.f_id;
    ep.f_per_id = perID;
    ep.f_role_id = recEventTypeRole::ROLE_Death_Died;
    ep.f_per_seq = 1;
    ep.Save();

    return event.f_id;
}

idt CreateRegDeathEvent( idt refID, idt perID, idt dateID, idt placeID, int* pseq )
{
    recEventRecord event(0);
    event.f_title = "Registered death of " + recPersona::GetNameStr( perID );
    event.f_type_id = recEventType::ET_RegDeath;
    event.f_date1_id = dateID;
    event.f_place_id = placeID;
    event.UpdateDatePoint();
    event.Save();
    recReferenceEntity::Create( refID, recReferenceEntity::TYPE_Event, event.f_id, pseq );

    recEventPersona ep(0);
    ep.f_event_rec_id = event.f_id;
    ep.f_per_id = perID;
    ep.f_role_id = recEventTypeRole::ROLE_RegDeath_Died;
    ep.f_per_seq = 1;
    ep.Save();

    return event.f_id;
}

idt CreateBurialEvent( idt refID, idt perID, idt dateID, idt placeID, int* pseq )
{
    recEventRecord event(0);
    event.f_title = "Burial of " + recPersona::GetNameStr( perID );
    event.f_type_id = recEventType::ET_Burial;
    event.f_date1_id = dateID;
    event.f_place_id = placeID;
    event.UpdateDatePoint();
    event.Save();
    recReferenceEntity::Create( refID, recReferenceEntity::TYPE_Event, event.f_id, pseq );

    recEventPersona ep(0);
    ep.f_event_rec_id = event.f_id;
    ep.f_per_id = perID;
    ep.f_role_id = recEventTypeRole::ROLE_Burial_Deceased;
    ep.f_per_seq = 1;
    ep.Save();

    return event.f_id;
}

idt CreateResidenceEvent( idt dateID, idt placeID, idt refID, int* pseq )
{
    recEventRecord event(0);
    event.f_title = "Residence at " + recPlace::GetAddressStr( placeID );
    event.f_type_id = recEventType::ET_Residence;
    event.f_date1_id = dateID;
    event.f_place_id = placeID;
    event.UpdateDatePoint();
    event.Save();
    recReferenceEntity::Create( refID, recReferenceEntity::TYPE_Event, event.f_id, pseq );

    return event.f_id;
}

void AddPersonaToEvent( idt eventID, idt perID, idt roleID, long datePt )
{
    if( eventID == 0 || perID == 0 ) return;
    recEventPersona ep(0);
    ep.f_event_rec_id = eventID;
    ep.f_per_id = perID;
    ep.f_role_id = roleID;
    ep.f_per_seq = recEventRecord::GetLastPerSeqNumber( eventID ) + 1;
    ep.Save();
}
idt CreateOccupation( const wxString& occ, idt refID, idt perID, idt dateID, int* pseq )
{
    if( occ.empty() ) return 0;
    idt occID = recEventTypeRole::FindOrCreate( occ, recEventType::ET_Occupation );

    recEventRecord event(0);
    event.f_title = "Occupation of " + recPersona::GetNameStr( perID );
    event.f_type_id = recEventType::ET_Occupation;
    event.f_date1_id = dateID;
    event.UpdateDatePoint();
    event.Save();
    recReferenceEntity::Create( refID, recReferenceEntity::TYPE_Event, event.f_id, pseq );

    recEventPersona ep(0);
    ep.f_event_rec_id = event.f_id;
    ep.f_per_id = perID;
    ep.f_role_id = occID;
    ep.f_per_seq = 1;
    ep.Save();

    return event.f_id;
}

idt CreateCondition( const wxString& con, idt refID, idt perID, idt dateID, int* pseq )
{
    if( con.empty() ) return 0;

    idt conID = recEventTypeRole::FindOrCreate( con, recEventType::ET_Condition );

    recEventRecord er(0);
    er.FSetTypeID( recEventType::ET_Condition );
    er.FSetDate1ID( dateID );
//    er.FSetDate2ID( dateID );
    er.UpdateDatePoint();
    er.SetAutoTitle( recPersona::GetNameStr( perID ) );
    er.Save();
    idt erID = er.FGetID();
    recReferenceEntity::Create( refID, recReferenceEntity::TYPE_Event, erID, pseq );

    recEventPersona ep(0);
    ep.f_event_rec_id = erID;
    ep.f_per_id = perID;
    ep.f_role_id = conID;
    ep.f_per_seq = 1;
    ep.Save();

    LinkOrCreateEventFromEventRecord( erID );
    return erID;
}

idt CreateRelationship( idt per1ID, const wxString& des, idt per2ID, idt refID, int* pseq )
{
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

idt LinkOrCreateEventFromEventRecord( idt erID )
{
    idt eID = 0;
    recEventRecord er(erID);
    recEventType et( er.FGetTypeID() );
    recIdVec eIDs = er.FindMatchingEvents( recEventRecord::recEVENT_Link_IndPer );
    if( et.FGetGrp() == recEventType::ETYPE_Grp_Birth ||
        et.FGetGrp() == recEventType::ETYPE_Grp_Death ||
        et.FGetGrp() == recEventType::ETYPE_Grp_Personal
    ) {
        if( eIDs.empty() ) {
            return CreateEventFromEventRecord( erID );
        }
        double conf = 0.999 / eIDs.size();
        for( size_t i = 0 ; i < eIDs.size() ; i++ ) {
            recEventEventRecord::Create( eIDs[i], erID, conf );
            recEvent::SetDatePeriodToInclude( eIDs[i], er.FGetDate1ID() );
        }
        eID = eIDs[0];
    } else {
        wxASSERT( false ); // TODO:                           <<=============<<<<<<
    }
    return eID;
}

idt CreateEventFromEventRecord( idt erID )
{
    recEventRecord er(erID);
    recEvent e(0);
    e.FSetTitle( er.FGetTitle() );
    e.FSetTypeID( er.FGetTypeID() );
    e.FSetDate1ID( recDate::Create( er.FGetDate1ID() ) );
    e.FSetDate2ID( recDate::Create( er.FGetDate2ID() ) );
    e.FSetPlaceID( er.FGetPlaceID() );
    e.FSetNote( er.FGetNote() );
    e.FSetDatePt( er.FGetDatePt() );
    e.Save();
    idt eID = e.FGetID();
    recEventEventRecord::Create( eID, erID );

    recEventPersonaVec eps = er.GetEventPersonas();
    for( size_t i = 0 ; i < eps.size() ; i++ ) {
        recIdVec indIDs = recPersona::GetIndividualIDs( eps[i].FGetPerID() );
        idt roleID = eps[i].FGetRoleID();
        for( size_t j = 0 ; j < indIDs.size() ; j++ ) {
            recIndividualEvent::Create( indIDs[j], eID, roleID );
        }
    }
    return eID;
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

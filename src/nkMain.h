/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Name:        nkMain.h
 * Project:     tfpnick: Private utility to create Matthews TFP database
 * Purpose:     Application main and supporting function header.
 * Author:      Nick Matthews
 * Website:     http://thefamilypack.org
 * Created:     23rd September 2011
 * Copyright:   Copyright (c) 2011 ~ 2019, Nick Matthews.
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

#ifndef NKMAIN_H
#define NKMAIN_H

#include <wx/filename.h>

#include <rec/recDb.h>

class wxXmlNode;
typedef std::vector< wxFileName > Filenames;

struct Media {
    idt ref;
    wxString filename;
    wxString text;
};
typedef std::vector< Media > MediaVec;

/* nkMain.cpp */
extern recEntity DecodeOldHref( const wxString& href );
extern bool DecodeHref( const wxString& href, idt* indID, wxString* indIdStr );
extern wxString CreateCommaList( wxString& first, wxString& second );

/* fiMedia.cpp */
extern bool InputMediaFiles( const wxString& imgFolder );
extern bool OutputMediaDatabase( const wxString& filename, const wxString& refFolder, const MediaVec& media );

/* fiRefMarkup.cpp */
extern void ProcessMarkupRef( idt refID, wxXmlNode* root );

/* nkRefDocuments.cpp */
extern void ProcessRefFile( const wxString path, idt refID, Filenames& customs );
extern bool InputRefFiles( const wxString& refFolder, MediaVec& media );

/* nkRefDocCustom.cpp */
extern void ProcessCustomFile( wxFileName& fn );

/* nkRecHelpers.cpp */
extern bool ExportGedcom( const wxString& path );
extern idt CreateDate( const wxString& date, idt refID, int* pseq = nullptr );
extern idt CreateDateFromAge( long age, idt baseID, idt refID, int* pseq = nullptr );
extern idt CreatePlace( const wxString& address, idt refID, int* pseq = NULL );
extern idt CreateCensusEvent( const wxString& title, idt dID, idt pID, idt rID );
extern idt CreatePersona( idt refID, idt indID, const wxString& nameStr, Sex sex = SEX_Unstated, int* pseq = nullptr, const wxString& note = "" );
extern idt CreatePersona( idt refID, idt indID, idt nameID, Sex sex = SEX_Unstated );
extern idt GetWifeName( const wxString& nameStr, idt husbandNameID, idt refID, int* pseq = nullptr );
extern idt CreateName( const wxString& name, idt style = 0 );
extern idt CreatePerName( const wxString& name, idt perID, idt style = 0 );
extern void AddPersonas( idt refID, recIdVec& indList, wxArrayString& names );
extern idt CreateBirthEvent( idt refID, idt perID, idt dateID, idt placeID );
extern idt CreateRegBirthEvent( idt refID, idt perID, idt dateID, idt placeID );
extern idt CreateChrisEventa( idt refID, idt perID, idt dateID, idt placeID );
extern idt CreateMarriageEvent( idt perID, idt dateID, idt placeID, long role, idt refID );
extern idt CreateDeathEventa( idt refID, idt perID, idt dateID, idt placeID );
extern idt CreateRegDeathEvent( idt refID, idt perID, idt dateID, idt placeID );
extern idt CreateBurialEventa( idt refID, idt perID, idt dateID, idt placeID );
extern idt CreateResidenceEventa( idt dateID, idt placeID, idt refID );
extern idt CreateMediaEventa( idt refID );
extern idt CreateFamilyRelEventa( idt refID, idt perID, idt dateID, idt placeID );
extern void AddPersonaToEventa( idt eaID, idt perID, idt roleID, const wxString& note = "" );
extern idt CreateOccupation( const wxString& occ, idt refID, idt perID, idt dateID );
extern idt CreateCondition( const wxString& con, idt refID, idt perID, idt dateID );
extern idt CreateRelationship( idt per1ID, const wxString& des, idt per2ID, idt refID, int* pseq = nullptr );
extern void rSetPersonaSex( idt perID, Sex sex );
extern void rAddNameToPersona( idt perID, idt nameID );
extern idt LinkOrCreateEventFromEventa( idt eaID );
extern idt CreateEventFromEventa( idt eaID );
extern bool CreateIndividual( idt indID, idt perID );

extern Sex GetSexFromStr( const wxString& str );
extern wxString GetConditionStr( Sex sex, const wxString& cond );

extern wxString ProperCase( const wxString& str );

/* nkXmlHelpers.cpp */
class wxXmlNode;
extern wxString xmlGetSource( wxXmlNode* node );
extern wxString xmlGetAllContent( wxXmlNode* node );
extern wxXmlNode* xmlGetFirstText( wxXmlNode* node );
extern wxXmlNode* xmlGetFirst( wxXmlNode* node, const wxString& tag );
extern wxXmlNode* xmlGetNext( wxXmlNode* node, const wxString& tag );
extern wxXmlNode* xmlGetChild( wxXmlNode* node );
extern wxXmlNode* xmlGetFirstChild( wxXmlNode* node, const wxString& tag );
extern wxXmlNode* xmlGetFirstTag( wxXmlNode* node, const wxString& tag );
extern wxXmlNode* xmlGetNextTag( wxXmlNode* node, const wxString& tag );
extern idt GetIndividualAnchor( wxXmlNode* node, wxString* name, wxXmlNode** aNode );

extern wxString xmlReadRecLoc( const wxString& str );

extern wxXmlNode* xmlCreateLink( wxXmlNode* node, const wxString& href );
extern wxXmlNode* xmlCreateLink( wxXmlNode* node, int beg, int end, const wxString& href );
extern bool xmlChangeLink( wxXmlNode* node, const wxString& href );
extern wxXmlNode* xmlCreateLink( wxXmlNode* node, recEntity entity, idt id );
extern wxXmlNode* xmlCreateLink( wxXmlNode* node, int beg, int end, recEntity entity, idt id );
extern bool xmlChangeLink( wxXmlNode* node, recEntity entity, idt id );

#endif // NKMAIN_H